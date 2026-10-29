/*  propfile.c, Copyright (c) 2008 Michael C. Martin */

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope thta it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  Se the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "core/log/log.h"
#include "core/string/StringUtils.h"
#include "propfile.h"
#include "libs/file/textfile.h"
#include "libs/reslib.h"
#include "options.h"

void removeSubstr(char* str, const char* toRemove)
{
	const size_t remLen = strlen(toRemove);
	char* copyEnd;
	char* copyFrom;

	if (nullptr == (str = strstr(str, toRemove)))
	{
		// no match.
		//printf("No match in {}\n", str);
		return;
	}

	// str points to toRemove in str now.
	copyFrom = str + remLen;

	while (nullptr != (copyEnd = strstr(copyFrom, toRemove)))
	{
		//printf ("match at %3ld in {}\n", copyEnd - str, str);
		memmove(str, copyFrom, copyEnd - copyFrom);
		str += copyEnd - copyFrom;
		copyFrom = copyEnd + remLen;
	}
	memmove(str, copyFrom, 1 + strlen(copyFrom));
}

void PropFile_from_string(char* d, PROPERTY_HANDLER handler, const char* prefix)
{
	int len, i;

	len = strlen(d);
	i = 0;
	while (i < len)
	{
		int key_start, key_end, value_start, value_end;
		/* Starting a line: search for non-whitespace */
		while ((i < len) && isspace(d[i]))
		{
			i++;
		}
		if (i >= len)
		{
			break; /* Done parsing! */
		}
		/* If it was a comment, skip to end of comment/file */
		if (d[i] == '#')
		{
			while ((i < len) && (d[i] != '\n'))
			{
				i++;
			}
			if (i >= len)
			{
				break;
			}
			continue; /* Back to keyword search */
		}
		key_start = i;
		/* Find the = on this line */
		while ((i < len) && (d[i] != '=') && (d[i] != '\n') && (d[i] != '#'))
		{
			i++;
		}
		if (i >= len)
		{ /* Bare key at EOF */
			uqm::log::warn("Warning: Bare keyword at EOF");
			break;
		}
		/* Comments here mean incomplete line too */
		if (d[i] != '=')
		{
			uqm::log::warn("Warning: Key without value");
			while ((i < len) && (d[i] != '\n'))
			{
				i++;
			}
			if (i >= len)
			{
				break;
			}
			continue; /* Back to keyword search */
		}
		/* Key ends at first whitespace before = , or at key_start*/
		key_end = i;
		while ((key_end > key_start) && isspace(d[key_end - 1]))
		{
			key_end--;
		}

		/* Consume the = */
		i++;
		/* Value starts at first non-whitespace after = on line... */
		while ((i < len) && (d[i] != '#') && (d[i] != '\n') && isspace(d[i]))
		{
			i++;
		}
		value_start = i;
		/* Until first non-whitespace before terminator */
		while ((i < len) && (d[i] != '#') && (d[i] != '\n'))
		{
			i++;
		}
		value_end = i;
		while ((value_end > value_start) && isspace(d[value_end - 1]))
		{
			value_end--;
		}
		/* Skip past EOL or EOF */
		while ((i < len) && (d[i] != '\n'))
		{
			i++;
		}
		i++;

		/* We now have start and end values for key and value.
		   We terminate the strings for both by writing \0s, then
		   make a new map entry. */
		d[key_end] = '\0';
		d[value_end] = '\0';
		if (prefix)
		{
			char buf[256] {};
			fmt::format_to_sz_n(buf, "{}{}", prefix, d + key_start);
			handler(buf, d + value_start);
		}
		else
		{
			handler(d + key_start, d + value_start);
		}
	}
}

void PropFile_from_file(uqm::io::File& f, PROPERTY_HANDLER handler, const char* prefix)
{
	size_t flen;
	char* data;

	flen = LengthResFile(f);

	data = (char*)malloc(flen + 1);
	if (!data)
	{
		return;
	}

	// We may end up with less bytes than we asked for due to the
	// DOS->Unix newline conversion
	flen = ReadResFile(data, 1, flen, f);
	data[flen] = '\0';

	PropFile_from_string(data, handler, prefix);
	free(data);
}

void PropFile_from_filename(const uqm::io::Directory& path, const char* fname, PROPERTY_HANDLER handler, const char* prefix)
{
	uqstl::optional<uqstl::string> text {uqm::readTextFile(path, fname)};
	if (!text)
	{
		return;
	}
	PropFile_from_string(text->data(), handler, prefix);
}
