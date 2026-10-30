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
#include "libs/memlib.h"


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

void _GetConversationData(const char* path, RESOURCE_DATA* resdata)
{
	unsigned long dataLen;
	void* result;
	int stringI;
	int path_len;
	// int num_data_sets; unused
	uint32_t opos;

	uqstl::vector<char> namedata(POOL_SIZE);
	// Contains the names (indexes) of the dialogs.
	uint32_t nlen[MAX_STRINGS];
	// Length of each of the names.
	uint32_t NameOffs;

	uqstl::vector<char> strdata(POOL_SIZE);
	// Contains the dialog strings.
	uint32_t slen[MAX_STRINGS];
	// Length of each of the dialog strings.
	uint32_t StringOffs;

	uqstl::vector<char> clipdata(POOL_SIZE);
	// Contains the file names of the speech files.
	uint32_t clen[MAX_STRINGS];
	// Length of each of the speech file names.
	uint32_t ClipOffs;

	uqstl::vector<char> ts_data {};
	// Contains the timestamp data for synching the text with the
	// speech.
	uint32_t tslen[MAX_STRINGS];
	// Length of each of the timestamp strings.
	uint32_t TSOffs;

	char CurrentLine[1024] {};
	char paths[1024] {};
	char* clip_path;
	char* ts_path;

	uqstl::optional<uqm::io::File> fp {};
	uqstl::optional<uqm::io::File> timestamp_fp {};
	uqstl::unique_ptr<uqstl::unordered_map<uqstl::string, STRING_TABLE_ENTRY_DESC*>> nameHashTable {};
	// Hash table of string names (such as "GLAD_WHEN_YOU_COME_BACK")
	// to a STRING.

	/* Parse out the conversation components. */
	uqm::strncpy_safe(paths, path);
	paths[1023] = '\0';
	clip_path = strchr(paths, ':');
	if (clip_path == nullptr)
	{
		ts_path = nullptr;
	}
	else
	{
		*clip_path = '\0';
		clip_path++;

		ts_path = strchr(clip_path, ':');
		if (ts_path != nullptr)
		{
			*ts_path = '\0';
			ts_path++;
		}
	}

	fp = res_OpenResFile(uqm::contentDir(), paths, uqm::io::OpenMode::Read);
	if (!fp)
	{
		uqm::log::warn("Warning: Can't open '{}'", paths);
		resdata->ptr = nullptr;
		return;
	}

	dataLen = static_cast<decltype(dataLen)>(fp->size());
	uqm::log::info("\t'{}' -- conversation phrases -- {} bytes", paths,
				   dataLen);
	if (clip_path)
	{
		uqm::log::info("\t'{}' -- voice clip directory", clip_path);
	}
	else
	{
		uqm::log::info("\tNo associated voice clips");
	}
	if (ts_path)
	{
		uqm::log::info("\t'{}' -- timestamps", ts_path);
	}
	else
	{
		uqm::log::info("\tNo associated timestamp file");
	}

	if (dataLen == 0)
	{
		uqm::log::warn("Warning: Trying to load empty file '{}'.",
					   path);
		goto err;
	}

	nameHashTable = uqstl::make_unique<uqstl::unordered_map<uqstl::string, STRING_TABLE_ENTRY_DESC*>>();

	path_len = clip_path ? strlen(clip_path) : 0;

	if (ts_path)
	{
		timestamp_fp = res_OpenResFile(uqm::contentDir(), ts_path, uqm::io::OpenMode::Read);
		if (timestamp_fp)
		{
			ts_data.resize(POOL_SIZE);
		}
	}

	opos = fp->tell();
	stringI = -1;
	NameOffs = 0;
	StringOffs = 0;
	ClipOffs = 0;
	TSOffs = 0;
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
			// String header, of the following form:
			//     #(GLAD_WHEN_YOU_COME_BACK) commander-000.ogg

			uqstl::string_view name {};
			uqstl::string_view ts {};

			uqstl::string_view currentLine {CurrentLine};
			if (const auto openParenPos {currentLine.find('(')}; openParenPos != uqstl::string_view::npos)
			{
				if (const auto closeParenPos {currentLine.find(')', openParenPos + 1)}; closeParenPos != uqstl::string_view::npos)
				{
					name = currentLine.substr(openParenPos + 1, closeParenPos - (openParenPos + 1));
					currentLine.remove_prefix(closeParenPos + 1);

					currentLine = uqm::trim(currentLine);


					ts = currentLine;
				}
			}


			if (!name.empty())
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

				// Store the string name.
				l = name.size() + 1;
				ensureBufSize(namedata, NameOffs + l);
				uqm::strncpy_safe({&namedata[NameOffs], static_cast<uint32_t>(l)}, name);
				NameOffs += l;
				nlen[stringI] = l;

				// now lets check for timestamp data
				if (timestamp_fp)
				{
					// We have a time stamp file.
					char TimeStampLine[1024] {};
					char* tsptr;
					bool ts_ok = false;
					timestamp_fp->readLine(TimeStampLine);
					if (TimeStampLine[0] == '#')
					{
						// Line is of the following form:
						//     #(GIVE_FUEL_AGAIN) 3304,3255
						uqstl::string_view timeStampView {TimeStampLine};

						tslen[stringI] = 0;
						if (const auto tsPos {timeStampView.find(name)}; tsPos != uqstl::string_view::npos)
						{
							timeStampView.remove_prefix(tsPos + name.size() + 1);
							ts_ok = true;

							timeStampView = uqm::trim(timeStampView);

							if (!timeStampView.empty())
							{
								l = timeStampView.size() + 1;
								ensureBufSize(ts_data, TSOffs + l);

								uqm::strncpy_safe({&ts_data[TSOffs], static_cast<uint32_t>(l)}, timeStampView);
								TSOffs += l;
								tslen[stringI] = l;
							}
						}
					}
					if (!ts_ok)
					{
						// timestamp data is invalid, remove all of it
						uqm::log::warn("Invalid timestamp data "
									   "for '{}'.  Disabling timestamps",
									   name);
						ts_data.clear();
						timestamp_fp.reset();
						TSOffs = 0;
					}
				}
				clen[stringI] = 0;
				if (!ts.empty())
				{
					const uint32_t tsLen = ts.size();
					l = path_len + tsLen + 1;
					ensureBufSize(clipdata, ClipOffs + l);

					if (clip_path)
					{
						uqm::strncpy_safe({&clipdata[ClipOffs], static_cast<uint32_t>(l)}, clip_path);
					}
					uqm::strncpy_safe({&clipdata[ClipOffs + path_len], static_cast<uint32_t>(l - path_len)}, ts);
					ClipOffs += l;
					clen[stringI] = l;
				}
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

		if ((int)fp->tell() - (int)opos >= (int)dataLen)
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
	// num_data_sets = (ClipOffs ? 1 : 0) + (TSOffs ? 1 : 0) + 1; unused
	if (++stringI)
	{
		int flags = 0;
		int stringCount = stringI;

		if (ClipOffs)
		{
			flags |= HAS_SOUND_CLIPS;
		}
		if (TSOffs)
		{
			flags |= HAS_TIMESTAMP;
		}
		flags |= HAS_NAMEINDEX;

		result = AllocStringTable(stringCount, flags);
		if (result)
		{
			// Copy all the gatherered data in a STRING_TABLE
			STRING_TABLE_DESC* lpST = (STRING_TABLE)result;
			STRING str;
			stringI = 0;

			// Store the dialog string.
			copy_strings_to_strtab(
				lpST, stringI, stringCount, strdata.data(), slen);
			stringI += stringCount;

			// Store the dialog names.
			copy_strings_to_strtab(
				lpST, stringI, stringCount, namedata.data(), nlen);
			stringI += stringCount;

			// Store sound clip file names.
			if (lpST->flags & HAS_SOUND_CLIPS)
			{
				copy_strings_to_strtab(
					lpST, stringI, stringCount, clipdata.data(), clen);
				stringI += stringCount;
			}

			// Store time stamp data.
			if (lpST->flags & HAS_TIMESTAMP)
			{
				copy_strings_to_strtab(
					lpST, stringI, stringCount, ts_data.data(), tslen);
				//stringI += stringCount;
			}

			// Store the STRING in the hash table indexed by the dialog
			// name.
			str = &lpST->strings[stringCount];
			for (stringI = 0; stringI < stringCount; stringI++)
			{
				(*nameHashTable)[(const char*)str[stringI].data] = &str[stringI];
			}

			lpST->nameIndex = nameHashTable.release();
		}
	}

	resdata->ptr = result;
	return;

err:
	resdata->ptr = nullptr;
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
