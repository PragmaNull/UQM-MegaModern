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
		*dword_array++ = MAKE_DWORD(
			MAKE_WORD(p[3], p[2]),
			MAKE_WORD(p[1], p[0]));
		p += 4;
	} while (--num_dwords);
}

static STRING
set_strtab_entry(STRING_TABLE_DESC* strtab, int index, const char* value,
				 int len)
{
	STRING str = &strtab->strings[index];

	if (str->data)
	{
		HFree(str->data);
		str->data = nullptr;
		str->length = 0;
	}
	if (len)
	{
		str->data = (STRINGPTR)HMalloc(len);
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
		set_strtab_entry(strtab, firstIndex + stringI,
						 off, lens[stringI]);
		off += lens[stringI];
	}
}

// Check whether a buffer has a certain minimum size, and enlarge it
// if necessary.
// buf: pointer to the pointer to the buffer. May be nullptr.
// curSize: pointer to the current size (multiple of 'increment')
// minSize: required minimum size
// increment: size to increment the buffer with if necessary
// On success, *buf and *curSize are updated. On failure, they are
// unchanged.
// returns false if and only if the buffer needs to be enlarged but
// memory allocation failed.
static bool
ensureBufSize(char** buf, size_t* curSize, size_t minSize, size_t increment)
{
	char* newBuf;
	size_t newSize;

	if (minSize <= *curSize)
	{
		// Buffer is large enough as it is.
		return true;
	}

	newSize = ((minSize + (increment - 1)) / increment) * increment;
	// Smallest multiple of 'increment' larger or equal to minSize.
	newBuf = (char*)HRealloc(*buf, newSize);
	if (newBuf == nullptr)
	{
		return false;
	}

	// Success
	*buf = newBuf;
	*curSize = newSize;
	return true;
}

void _GetConversationData(const char* path, RESOURCE_DATA* resdata)
{
	unsigned long dataLen;
	void* result;
	int stringI;
	int path_len;
	// int num_data_sets; unused
	uint32_t opos;

	char* namedata = nullptr;
	// Contains the names (indexes) of the dialogs.
	uint32_t nlen[MAX_STRINGS];
	// Length of each of the names.
	uint32_t NameOffs;
	size_t tot_name_size;

	char* strdata = nullptr;
	// Contains the dialog strings.
	uint32_t slen[MAX_STRINGS];
	// Length of each of the dialog strings.
	uint32_t StringOffs;
	size_t tot_string_size;

	char* clipdata = nullptr;
	// Contains the file names of the speech files.
	uint32_t clen[MAX_STRINGS];
	// Length of each of the speech file names.
	uint32_t ClipOffs;
	size_t tot_clip_size;

	char* ts_data = nullptr;
	// Contains the timestamp data for synching the text with the
	// speech.
	uint32_t tslen[MAX_STRINGS];
	// Length of each of the timestamp strings.
	uint32_t TSOffs;
	size_t tot_ts_size = 0;

	char CurrentLine[1024] {};
	char paths[1024] {};
	char* clip_path;
	char* ts_path;

	uqstl::optional<uqm::io::File> fp {};
	uqstl::optional<uqm::io::File> timestamp_fp {};
	std::unordered_map<std::string, STRING_TABLE_ENTRY_DESC*>* nameHashTable = nullptr;
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

	tot_string_size = POOL_SIZE;
	strdata = (char*)HMalloc(tot_string_size);
	if (strdata == 0)
	{
		goto err;
	}

	tot_name_size = POOL_SIZE;
	namedata = (char*)HMalloc(tot_name_size);
	if (namedata == 0)
	{
		goto err;
	}

	tot_clip_size = POOL_SIZE;
	clipdata = (char*)HMalloc(tot_clip_size);
	if (clipdata == 0)
	{
		goto err;
	}
	ts_data = nullptr;

	nameHashTable = new std::unordered_map<std::string, STRING_TABLE_ENTRY_DESC*>();
	if (nameHashTable == nullptr)
	{
		goto err;
	}

	path_len = clip_path ? strlen(clip_path) : 0;

	if (ts_path)
	{
		timestamp_fp = res_OpenResFile(uqm::contentDir(), ts_path, uqm::io::OpenMode::Read);
		if (timestamp_fp)
		{
			tot_ts_size = POOL_SIZE;
			ts_data = (char*)HMalloc(tot_ts_size);
			if (ts_data == 0)
			{
				goto err;
			}
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
				if (!ensureBufSize(&namedata, &tot_name_size,
								   NameOffs + l, POOL_SIZE))
				{
					goto err;
				}
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
								if (!ensureBufSize(&ts_data, &tot_ts_size, TSOffs + l,
												   POOL_SIZE))
								{
									goto err;
								}

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
						HFree(ts_data);
						ts_data = nullptr;
						timestamp_fp.reset();
						TSOffs = 0;
					}
				}
				clen[stringI] = 0;
				if (!ts.empty())
				{
					const uint32_t tsLen = ts.size();
					l = path_len + tsLen + 1;
					if (!ensureBufSize(&clipdata, &tot_clip_size,
									   ClipOffs + l, POOL_SIZE))
					{
						goto err;
					}

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

			if (!ensureBufSize(&strdata, &tot_string_size, StringOffs + l,
							   POOL_SIZE))
			{
				goto err;
			}

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
				lpST, stringI, stringCount, strdata, slen);
			stringI += stringCount;

			// Store the dialog names.
			copy_strings_to_strtab(
				lpST, stringI, stringCount, namedata, nlen);
			stringI += stringCount;

			// Store sound clip file names.
			if (lpST->flags & HAS_SOUND_CLIPS)
			{
				copy_strings_to_strtab(
					lpST, stringI, stringCount, clipdata, clen);
				stringI += stringCount;
			}

			// Store time stamp data.
			if (lpST->flags & HAS_TIMESTAMP)
			{
				copy_strings_to_strtab(
					lpST, stringI, stringCount, ts_data, tslen);
				//stringI += stringCount;
			}

			// Store the STRING in the hash table indexed by the dialog
			// name.
			str = &lpST->strings[stringCount];
			for (stringI = 0; stringI < stringCount; stringI++)
			{
				(*nameHashTable)[(const char*)str[stringI].data] = &str[stringI];
			}

			lpST->nameIndex = nameHashTable;
		}
	}
	HFree(strdata);
	if (clipdata != nullptr)
	{
		HFree(clipdata);
	}
	if (ts_data != nullptr)
	{
		HFree(ts_data);
	}

	resdata->ptr = result;
	return;

err:
	if (nameHashTable != nullptr)
	{
		delete nameHashTable;
	}
	if (ts_data != nullptr)
	{
		HFree(ts_data);
	}
	if (clipdata != nullptr)
	{
		HFree(clipdata);
	}
	if (strdata != nullptr)
	{
		HFree(strdata);
	}
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
	size_t tot_string_size;
	char CurrentLine[1024];
	char* strdata = nullptr;

	tot_string_size = POOL_SIZE;
	strdata = (char*)HMalloc(tot_string_size);
	if (strdata == 0)
	{
		goto err;
	}

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

			if (!ensureBufSize(&strdata, &tot_string_size, StringOffs + l,
							   POOL_SIZE))
			{
				goto err;
			}

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
			copy_strings_to_strtab(lpST, 0, stringCount, strdata, slen);
		}
	}
	HFree(strdata);

	return result;

err:
	if (strdata != nullptr)
	{
		HFree(strdata);
	}
	return 0;
}


void* _GetBinaryTableData(const ResourceFile& resource)
{
	void* result;
	result = GetResourceData(resource.file, resource.length);

	if (result)
	{
		uint32_t* fileData;
		STRING_TABLE lpST;

		fileData = (uint32_t*)result;

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
				set_strtab_entry(lpST, i, (char*)stringptr, fileData[2 + i]);
				stringptr += fileData[2 + i];
			}
		}
		HFree(result);
		result = lpST;
	}

	return result;
}
