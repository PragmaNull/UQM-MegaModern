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

#include "options.h"
#include "strintrn.h"
#include "libs/graphics/gfx_common.h"
#include "libs/reslib.h"
#include "core/log/log.h"
#include "core/stl/optional.h"
#include "libs/file/textfile.h"
#include <fmt/format.h>
#include <yyjson.h>


#define MAX_STRINGS 2048
#define POOL_SIZE 4096

static void
dword_convert(uint32_t* dword_array, uint16_t num_dwords)
{
	uint8_t* p = (uint8_t*)dword_array;

	do
	{
		*dword_array++ = uqm::makeDword(
			uqm::makeWord(p[3], p[2]),
			uqm::makeWord(p[1], p[0]));
		p += 4;
	} while (--num_dwords);
}

STRING
SetStringTableEntry(STRING_TABLE strtab, int index, const void* value, int len)
{
	STRING str = &strtab->strings[index];

	delete[] str->data;
	str->data = nullptr;
	str->length = 0;
	if (len)
	{
		str->data = new char[len];
		str->length = len;
		memcpy(str->data, value, len);
	}
	return str;
}

static void
copy_strings_to_strtab(STRING_TABLE_DESC* strtab, size_t firstIndex,
					   size_t count, const char* data, const uint32_t* lens)
{
	size_t stringI;
	const char* off = data;

	for (stringI = 0; stringI < count; stringI++)
	{
		SetStringTableEntry(strtab, firstIndex + stringI,
							off, lens[stringI]);
		off += lens[stringI];
	}
}

// Enlarges 'buf' to at least 'minSize' bytes.
static void
ensureBufSize(uqstl::vector<char>& buf, size_t minSize)
{
	if (buf.size() < minSize)
	{
		buf.resize(minSize + POOL_SIZE);
	}
}

namespace
{

using JsonDocPtr = uqstl::unique_ptr<yyjson_doc, decltype(&yyjson_doc_free)>;

[[nodiscard]] JsonDocPtr readJsonFile(uqstl::string_view path)
{
	const uqstl::string pathText {path};
	const auto text {uqm::readTextFile(uqm::contentDir(), pathText.c_str())};
	if (!text)
	{
		uqm::log::warn("Warning: Can't open '{}'", path);
		return {nullptr, &yyjson_doc_free};
	}
	yyjson_read_err error {};
	JsonDocPtr doc {yyjson_read_opts(const_cast<char*>(text->data()), text->size(), YYJSON_READ_ALLOW_INVALID_UNICODE, nullptr, &error),
					&yyjson_doc_free};
	if (!doc)
	{
		uqm::log::warn("Warning: '{}': JSON parse error at byte {}: {}", path, error.pos, error.msg);
	}
	return doc;
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
		text += fmt::format("{}", yyjson_get_uint(number));
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
void setEntry(STRING_TABLE table, int index, const uqstl::optional<uqstl::string>& text)
{
	if (text)
	{
		SetStringTableEntry(table, index, text->c_str(), static_cast<int>(text->size() + 1));
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

	const JsonDocPtr textDoc {readJsonFile(textPath)};
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

	JsonDocPtr timingDoc {nullptr, &yyjson_doc_free};
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
	int flags {HAS_NAMEINDEX};
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
			flags |= HAS_SOUND_CLIPS;
		}
		yyjson_val* const timingLine {yyjson_obj_get(timingLines, id)};
		if (uqstl::string timing {timingText(timingLine != nullptr ? timingLine : line)}; !timing.empty())
		{
			timings[index] = uqstl::move(timing);
			flags |= HAS_TIMESTAMP;
		}
	}

	{
		size_t timingIndex {};
		size_t timingMax {};
		yyjson_val* timingId {};
		yyjson_val* timingValue {};
		yyjson_obj_foreach(timingLines, timingIndex, timingMax, timingId, timingValue)
		{
			if (uqstl::find(names.begin(), names.end(), yyjson_get_str(timingId)) == names.end())
			{
				uqm::log::warn("Warning: '{}' has timing for '{}', which '{}' does not have.", timingPath, yyjson_get_str(timingId),
							   textPath);
			}
		}
	}

	STRING_TABLE table {AllocStringTable(static_cast<int>(count), flags)};
	if (table == nullptr)
	{
		return;
	}
	const int size {static_cast<int>(count)};
	int block {0};
	for (int i {0}; i < size; ++i)
	{
		setEntry(table, block + i, texts[i]);
		setEntry(table, block + size + i, names[i]);
	}
	block += 2 * size;
	if (flags & HAS_SOUND_CLIPS)
	{
		for (int i {0}; i < size; ++i)
		{
			setEntry(table, block + i, clips[i]);
		}
		block += size;
	}
	if (flags & HAS_TIMESTAMP)
	{
		for (int i {0}; i < size; ++i)
		{
			setEntry(table, block + i, timings[i]);
		}
	}

	auto nameIndex {uqstl::make_unique<uqstl::unordered_map<uqstl::string, STRING_TABLE_ENTRY_DESC*>>()};
	for (int i {0}; i < size; ++i)
	{
		(*nameIndex)[names[i]] = &table->strings[size + i];
	}
	table->nameIndex = nameIndex.release();
	resdata->ptr = table;
}

void* _GetStringData(const ResourceFile& resource)
{
	uqm::io::File* const fp {&resource.file};
	const uint32_t length {resource.length};
	void* result;

	int stringI;
	uint32_t opos;
	uint32_t slen[MAX_STRINGS];
	uint32_t StringOffs;
	char CurrentLine[1024];
	uqstl::vector<char> strdata(POOL_SIZE);

	opos = fp->tell();
	stringI = -1;
	StringOffs = 0;
	for (;;)
	{
		int l;

		if (!fp->readLine(CurrentLine))
		{
			// EOF or read error.
			break;
		}

		if (stringI >= MAX_STRINGS - 1)
		{
			// Too many strings.
			break;
		}

		if (CurrentLine[0] == '#')
		{
			char CopyLine[1024] {};

			uqm::strncpy_safe(CopyLine, CurrentLine);
			const char* const openParen = strchr(&CopyLine[1], '(');
			const bool hasValidHeader = openParen && strchr(openParen + 1, ')');
			if (hasValidHeader)
			{
				if (stringI >= 0)
				{
					while (slen[stringI] > 1 && (strdata[StringOffs - 2] == '\n' || strdata[StringOffs - 2] == '\r'))
					{
						--slen[stringI];
						--StringOffs;
						strdata[StringOffs - 1] = '\0';
					}
				}

				slen[++stringI] = 0;
			}
		}
		else if (stringI >= 0)
		{
			char* s;
			l = strlen(CurrentLine) + 1;

			ensureBufSize(strdata, StringOffs + l);

			if (slen[stringI])
			{
				--slen[stringI];
				--StringOffs;
			}
			s = &strdata[StringOffs];
			slen[stringI] += l;
			StringOffs += l;

			uqm::strncpy_safe({s, static_cast<uint32_t>(l)}, {CurrentLine, static_cast<uint32_t>(l - 1)});
		}

		if ((int)fp->tell() - (int)opos >= (int)length)
		{
			break;
		}
	}
	if (stringI >= 0)
	{
		while (slen[stringI] > 1 && (strdata[StringOffs - 2] == '\n' || strdata[StringOffs - 2] == '\r'))
		{
			--slen[stringI];
			--StringOffs;
			strdata[StringOffs - 1] = '\0';
		}
	}

	result = nullptr;
	if (++stringI)
	{
		int flags = 0;
		int stringCount = stringI;

		result = AllocStringTable(stringI, flags);
		if (result)
		{
			STRING_TABLE_DESC* lpST = (STRING_TABLE)result;
			copy_strings_to_strtab(lpST, 0, stringCount, strdata.data(), slen);
		}
	}

	return result;
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
	if (!resource.file.readExact(uqm::io::asBytes(data.data(), length)))
	{
		return nullptr;
	}

	STRING_TABLE lpST {};
	{
		uint32_t* fileData {data.data()};

		dword_convert(fileData, 1); /* Length */

		lpST = AllocStringTable(fileData[0], 0);
		if (lpST)
		{
			int i, size;
			uint8_t* stringptr;

			size = lpST->size;

			dword_convert(fileData + 1, size + 1);
			stringptr = (uint8_t*)(fileData + 2 + size + fileData[1]);
			for (i = 0; i < size; i++)
			{
				SetStringTableEntry(lpST, i, stringptr, fileData[2 + i]);
				stringptr += fileData[2 + i];
			}
		}
	}

	return lpST;
}
