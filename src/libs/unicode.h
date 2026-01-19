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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#pragma once
//#ifndef UNICODE_H
//#define UNICODE_H

#include "port.h"
#include "types.h"
		// for uint32
#include <sys/types.h>
		// For size_t
#include "compiler.h"
		// for UNICODE

#if 0 //defined(__cplusplus)
extern "C" {
#endif

typedef uint32 UniChar;

//#ifdef UNICODE_INTERNAL
//using UNICODE_CHAR = unsigned char;
//#else
//using UNICODE_CHAR = char;
//#endif

UniChar getCharFromString(const unsigned char** ptr);
UniChar getCharFromStringN(const unsigned char** ptr,
		const unsigned char* end);
unsigned char *getLineFromString(const unsigned char* start,
		const unsigned char **end, const unsigned char** startNext);
size_t utf8StringCount(const unsigned char*start);
size_t utf8StringCountN(const unsigned char *start,
		const unsigned char *end);
size_t utf8CharCount (const unsigned char *start, UniChar uni_char);
int utf8StringPos (const unsigned char *pStr, UniChar ch);
int utf8StringLastPos (const unsigned char *pStr, UniChar ch);
unsigned char *utf8StringCopy (unsigned char *dst, size_t size,
		const unsigned char *src);
int utf8StringCompare (const unsigned char *str1, const unsigned char *str2);
unsigned char *skipUTF8Chars(const unsigned char *ptr, size_t num);
size_t getUniCharFromString(UniChar *wstr, size_t maxcount,
		const unsigned char *start);
size_t getUniCharFromStringN(UniChar *wstr, size_t maxcount,
		const unsigned char *start, const unsigned char *end);
int getStringFromChar(unsigned char *ptr, size_t size, UniChar ch);
size_t getStringFromWideN(unsigned char *ptr, size_t size,
		const UniChar *wstr, size_t count);
size_t getStringFromWide(unsigned char *ptr, size_t size,
		const UniChar *wstr);

// versions which accept a simple char*
inline UniChar getCharFromString(const char** ptr) 
{ 
	return getCharFromString(reinterpret_cast<const unsigned char**>(ptr)); 
}
inline UniChar getCharFromStringN(const char** ptr, const char* end) 
{ 
	return getCharFromStringN(reinterpret_cast<const unsigned char**>(ptr), reinterpret_cast<const unsigned char*>(end)); 
}
inline char* getLineFromString(const char* start, const char** end, const char** startNext) 
{
	unsigned char* ln = getLineFromString(reinterpret_cast<const unsigned char*>(start), reinterpret_cast<const unsigned char**>(end), reinterpret_cast<const unsigned char**>(startNext));
	return reinterpret_cast<char*>(ln);
}
inline size_t utf8StringCount(const char* start)
{
	return utf8StringCount(reinterpret_cast<const unsigned char*>(start));
}
inline size_t utf8StringCountN(const char* start, const char* end)
{
	return utf8StringCountN(reinterpret_cast<const unsigned char*>(start), reinterpret_cast<const unsigned char*>(end));
}
inline size_t utf8CharCount(const char* start, UniChar uni_char)
{
	return utf8CharCount(reinterpret_cast<const unsigned char*>(start), uni_char);
}
inline int utf8StringPos(const char* pStr, UniChar ch)
{
	return utf8StringPos(reinterpret_cast<const unsigned char*>(pStr), ch);
}
inline int utf8StringLastPos(const char* pStr, UniChar ch)
{
	return utf8StringLastPos(reinterpret_cast<const unsigned char*>(pStr), ch);
}
inline char* utf8StringCopy(char* dst, size_t size, const char* src)
{
	unsigned char* ret = utf8StringCopy(reinterpret_cast<unsigned char*>(dst), size, reinterpret_cast<const unsigned char*>(src));
	return reinterpret_cast<char*>(ret);
}
inline int utf8StringCompare(const char* str1, const char* str2)
{
	return utf8StringCompare(reinterpret_cast<const unsigned char*>(str1), reinterpret_cast<const unsigned char*>(str2));
}
inline char* skipUTF8Chars(const char* ptr, size_t num)
{
	unsigned char* ret = skipUTF8Chars(reinterpret_cast<const unsigned char*>(ptr), num);
	return reinterpret_cast<char*>(ret);
}
inline size_t getUniCharFromString(UniChar* wstr, size_t maxcount, const char* start)
{
	return getUniCharFromString(wstr, maxcount, reinterpret_cast<const unsigned char*>(start));
}
inline size_t getUniCharFromStringN(UniChar* wstr, size_t maxcount, const char* start, const char* end)
{
	return getUniCharFromStringN(wstr, maxcount, reinterpret_cast<const unsigned char*>(start), reinterpret_cast<const unsigned char*>(end));
}
inline int getStringFromChar(char* ptr, size_t size, UniChar ch)
{
	return getStringFromChar(reinterpret_cast<unsigned char*>(ptr), size, ch);
}
inline size_t getStringFromWideN(char* ptr, size_t size, const UniChar* wstr, size_t count)
{
	return getStringFromWideN(reinterpret_cast<unsigned char*>(ptr), size, wstr, count);
}
inline size_t getStringFromWide(char* ptr, size_t size, const UniChar* wstr)
{
	return getStringFromWide(reinterpret_cast<unsigned char*>(ptr), size, wstr);
}

int UniChar_isGraph(UniChar ch);
int UniChar_isPrint(UniChar ch);
UniChar UniChar_toUpper(UniChar ch);
UniChar UniChar_toLower(UniChar ch);

UNICODE *AlignText (const UNICODE *str, sint16 *loc_x);
UNICODE *AddPadd (const UNICODE *str, sint16 *padding);

#undef UNICODE_CHAR

#if 0 //defined(__cplusplus)
}
#endif

//#endif  /* UNICODE_H */

