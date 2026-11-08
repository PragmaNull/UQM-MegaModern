//Copyright Paul Reiche, Fred Ford. 1992-2002

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <fmt/format.h>

#include "core/log/log.h"
#include "core/stl/optional.h"
#include "libs/file/json.h"
#include "libs/file/textfile.h"
#include "libs/graphics/gfx_common.h"
#include "libs/reslib.h"
#include "options.h"
#include "strintrn.h"


static constexpr int c_stringTableVersion {1};

static void dword_convert(uqstl::span<uint32_t> dword_array)
{
	const uint8_t* p {reinterpret_cast<const uint8_t*>(dword_array.data())};

	for (uint32_t& dword : dword_array)
	{
		dword = uqm::makeDword(
			uqm::makeWord(p[3], p[2]),
			uqm::makeWord(p[1], p[0]));
		p += 4;
	}
}

void SetStringTableEntry(uqm::StringTable* strtab, int index, uqstl::string_view value)
{
	strtab->set(static_cast<size_t>(index), value);
}

namespace
{

[[nodiscard]] uqm::JsonText readJsonFile(uqstl::string_view path)
{
	const uqstl::string pathText {path};
	auto text {uqm::readJsonText(uqm::contentDir(), pathText.c_str())};
	if (!text)
	{
		uqm::log::warn("Warning: Can't open '{}'", path);
		return {};
	}
	return {uqstl::move(*text), path, YYJSON_READ_ALLOW_INVALID_UNICODE};
}

[[nodiscard]] uqstl::string joinNumbers(yyjson_val* numbers)
{
	uqstl::string text {};
	size_t index {};
	size_t max {};
	yyjson_val* number {};
	yyjson_arr_foreach(numbers, index, max, number)
	{
		if (index > 0)
		{
			text += ',';
		}
		fmt::format_to(uqstl::back_inserter(text), "{}", yyjson_get_uint(number));
	}
	return text;
}

// The timing of a line as the track player reads it: the times of the pages, and after each ';' the times of a subclip.
[[nodiscard]] uqstl::string timingText(yyjson_val* line)
{
	yyjson_val* const timing {yyjson_obj_get(line, "timing")};
	yyjson_val* const subclipTiming {yyjson_obj_get(line, "subclipTiming")};
	if (timing == nullptr && subclipTiming == nullptr)
	{
		return {};
	}
	uqstl::string text {yyjson_is_arr(timing) ? joinNumbers(timing) : uqstl::string {}};
	size_t index {};
	size_t max {};
	yyjson_val* subclip {};
	yyjson_arr_foreach(subclipTiming, index, max, subclip)
	{
		text += ';';
		text += joinNumbers(subclip);
	}
	return text;
}

[[nodiscard]] uqstl::string joinLines(yyjson_val* lines)
{
	uqstl::string text {};
	size_t index {};
	size_t max {};
	yyjson_val* line {};
	yyjson_arr_foreach(lines, index, max, line)
	{
		if (index > 0)
		{
			text += '\n';
		}
		text += uqstl::string_view {yyjson_get_str(line), yyjson_get_len(line)};
	}
	return text;
}

// Sets an entry to the text and its NUL, or empties it if 'text' is nullopt.
void setEntry(uqm::StringTable* table, int index, const uqstl::optional<uqstl::string>& text)
{
	if (text)
	{
		SetStringTableEntry(table, index, {text->c_str(), text->size() + 1});
	}
}

} // namespace

// Loads a conversation from the fields "text", "clips" and "timing": a text file, a clip directory and a timing file.
// The timing file holds timing for the lines of conversations by the "conversation" name of the text file, and
// replaces the timing in it.
void _GetConversationData(const ResourceArg& arg, RESOURCE_DATA* resdata)
{
	resdata->ptr = nullptr;

	const char* const textField {resourceStringField(arg, "text")};
	if (textField == nullptr)
	{
		uqm::log::warn("Warning: A CONVERSATION resource has no text file.");
		return;
	}
	const char* const clipsField {resourceStringField(arg, "clips")};
	const char* const timingField {resourceStringField(arg, "timing")};
	const uqstl::string_view textPath {textField};
	const uqstl::string_view clipDir {clipsField != nullptr ? clipsField : ""};
	const uqstl::string_view timingPath {timingField != nullptr ? timingField : ""};

	const uqm::JsonText textDoc {readJsonFile(textPath)};
	if (!textDoc)
	{
		return;
	}
	yyjson_val* const root {yyjson_doc_get_root(textDoc.get())};
	yyjson_val* const lines {yyjson_obj_get(root, "lines")};
	const size_t count {yyjson_arr_size(lines)};
	if (count == 0)
	{
		uqm::log::warn("Warning: '{}' has no conversation lines.", textPath);
		return;
	}

	uqm::JsonText timingDoc {};
	yyjson_val* timingLines {};
	if (!timingPath.empty())
	{
		timingDoc = readJsonFile(timingPath);
		const char* const conversation {yyjson_get_str(yyjson_obj_get(root, "conversation"))};
		if (timingDoc && conversation != nullptr)
		{
			timingLines = yyjson_obj_get(yyjson_obj_get(yyjson_doc_get_root(timingDoc.get()), "conversations"), conversation);
		}
	}
	uqm::log::info("\t'{}' -- conversation phrases -- {} lines", textPath, count);

	uqstl::vector<uqstl::optional<uqstl::string>> texts(count);
	uqstl::vector<uqstl::string> names(count);
	uqstl::vector<uqstl::optional<uqstl::string>> clips(count);
	uqstl::vector<uqstl::optional<uqstl::string>> timings(count);
	int flags {uqm::StringTable::c_hasNames};
	size_t index {};
	size_t max {};
	yyjson_val* line {};
	yyjson_arr_foreach(lines, index, max, line)
	{
		const char* const id {yyjson_get_str(yyjson_obj_get(line, "id"))};
		if (id == nullptr)
		{
			uqm::log::warn("Warning: '{}': line {} has no id.", textPath, index);
			return;
		}
		names[index] = id;
		if (yyjson_val* const text {yyjson_obj_get(line, "text")})
		{
			texts[index] = joinLines(text);
		}
		if (const char* const clip {yyjson_get_str(yyjson_obj_get(line, "clip"))}; clip != nullptr && *clip != '\0')
		{
			clips[index] = uqstl::string {clipDir} + clip;
			flags |= uqm::StringTable::c_hasSoundClips;
		}
		yyjson_val* const timingLine {yyjson_obj_get(timingLines, id)};
		if (uqstl::string timing {timingText(timingLine != nullptr ? timingLine : line)}; !timing.empty())
		{
			timings[index] = uqstl::move(timing);
			flags |= uqm::StringTable::c_hasTimestamps;
		}
	}

	auto* const table {
		new uqm::StringTable {count, flags}
	   };
	if (table == nullptr)
	{
		return;
	}
	const int size {static_cast<int>(count)};
	int block {0};
	const auto writeBlock {[&](const auto& entries) {
		for (int i {0}; i < size; ++i)
		{
			setEntry(table, block + i, entries[i]);
		}
		block += size;
	}};
	writeBlock(texts);
	writeBlock(names);
	if (flags & uqm::StringTable::c_hasSoundClips)
	{
		writeBlock(clips);
	}
	if (flags & uqm::StringTable::c_hasTimestamps)
	{
		writeBlock(timings);
	}

	for (int i {0}; i < size; ++i)
	{
		table->indexName(static_cast<size_t>(i));
	}

	size_t timingIndex {};
	size_t timingMax {};
	yyjson_val* timingId {};
	yyjson_val* timingValue {};
	yyjson_obj_foreach(timingLines, timingIndex, timingMax, timingId, timingValue)
	{
		if (!table->find(yyjson_get_str(timingId)))
		{
			uqm::log::warn("Warning: '{}' has timing for '{}', which '{}' does not have.", timingPath, yyjson_get_str(timingId),
						   textPath);
		}
	}
	resdata->ptr = table;
}

// Loads a string table from a JSON file with a "strings" array. Each string has an "id", and can have "text": an array
// of text lines.
void* _GetStringData(const ResourceFile& resource)
{
	auto text {readResourceText(resource)};
	if (!text)
	{
		uqm::log::warn("Warning: Can't read '{}'", resource.path);
		return nullptr;
	}
	const uqm::JsonText doc {uqstl::move(*text), resource.path, YYJSON_READ_ALLOW_INVALID_UNICODE};
	yyjson_val* const strings {uqm::versionedMember(doc, c_stringTableVersion, "strings", YYJSON_TYPE_ARR, resource.path)};
	const size_t count {yyjson_arr_size(strings)};
	if (count == 0)
	{
		uqm::log::warn("Warning: '{}' is not a valid string table.", resource.path);
		return nullptr;
	}

	auto* const table {
		new uqm::StringTable {count, 0}
	   };
	if (table == nullptr)
	{
		return nullptr;
	}
	size_t index {};
	size_t max {};
	yyjson_val* entry {};
	yyjson_arr_foreach(strings, index, max, entry)
	{
		if (yyjson_val* const lines {yyjson_obj_get(entry, "text")})
		{
			setEntry(table, static_cast<int>(index), joinLines(lines));
		}
	}
	return table;
}


void* _GetBinaryTableData(const ResourceFile& resource)
{
	// The data has a length prefix. ~0 means uncompressed data; compressed data is not supported.
	uint32_t compLen {};
	if (!resource.file.readValue(compLen))
	{
		return nullptr;
	}
	if (compLen != ~uint32_t {0})
	{
		uqm::log::warn("LZ-compressed binary data not supported");
		return nullptr;
	}
	const uint32_t length {resource.length - static_cast<uint32_t>(sizeof(uint32_t))};
	uqstl::vector<uint32_t> data((length + 3) / 4, 0);
	if (!resource.file.readExact(uqstl::as_writable_bytes(uqstl::span {data}).first(length)))
	{
		return nullptr;
	}

	uqm::StringTable* lpST {};
	{
		uint32_t* fileData {data.data()};

		dword_convert({fileData, 1}); /* Length */

		lpST = new uqm::StringTable {fileData[0], 0};
		if (lpST)
		{
			int size {};

			size = static_cast<int>(lpST->size());

			dword_convert({fileData + 1, static_cast<size_t>(size) + 1});
			const char* stringptr {reinterpret_cast<const char*>(fileData + 2 + size + fileData[1])};
			for (int i {0}; i < size; i++)
			{
				SetStringTableEntry(lpST, i, {stringptr, fileData[2 + i]});
				stringptr += fileData[2 + i];
			}
		}
	}

	return lpST;
}
