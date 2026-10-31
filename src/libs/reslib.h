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

#ifndef LIBS_RESLIB_H_
#define LIBS_RESLIB_H_

//#include <stdio.h>
#include "core/misc/bittricks.h"
#include "port.h"
#include "libs/io/FileSystem.h"
#include "libs/resource/resindex.h"

#if 0 //defined(__cplusplus)
extern "C" {
#endif

typedef struct resource_index_desc RESOURCE_INDEX_DESC;
typedef RESOURCE_INDEX_DESC* RESOURCE_INDEX;

typedef const char* RESOURCE;

typedef union
{
	uint32_t num;
	float fnum;
	void* ptr;
	const char* str;
} RESOURCE_DATA;

#define NULL_RESOURCE nullptr


typedef void(ResourceLoadFun)(const ResourceArg& arg, RESOURCE_DATA* resdata);
// For types whose data is a string, such as a path.
typedef void(ResourceTextLoadFun)(const char* text, RESOURCE_DATA* resdata);
typedef bool(ResourceFreeFun)(void* handle);

// A resource file for a loader to read.
struct ResourceFile
{
	uqm::io::File& file;
	const uqm::io::Directory& dir;
	// The path of the file in 'dir'.
	uqstl::string_view path;
	uint32_t length;
};
typedef void*(ResourceLoadFileFun)(const ResourceFile& resource);

void* LoadResourceFromPath(const char* pathname, ResourceLoadFileFun fn);

uqstl::optional<uqm::io::File> res_OpenResFile(const uqm::io::Directory& dir, const char* filename, uqm::io::OpenMode mode);
// Opens a file that the caller owns and deletes, for structures in C memory. Returns nullptr and sets errno on failure.
uqgsl::owner<uqm::io::File*> res_NewResFile(const uqm::io::Directory& dir, const char* filename);
// Opens 'filename' and calls 'loadFun' for it. Returns nullptr if 'filename' cannot be opened or is empty.
void* res_LoadResFile(const uqm::io::Directory& dir, const char* filename, ResourceLoadFileFun* loadFun);
// fread and fseek style functions, for C callbacks and macros.
size_t ReadResFile(void* lpBuf, size_t size, size_t count, uqm::io::File& fp);
size_t WriteResFile(const void* lpBuf, size_t size, size_t count, uqm::io::File& fp);
int GetResFileChar(uqm::io::File& fp);
int PutResFileChar(char ch, uqm::io::File& fp);
int PutResFileNewline(uqm::io::File& fp);
long SeekResFile(uqm::io::File& fp, long offset, int whence);
long TellResFile(uqm::io::File& fp);
size_t LengthResFile(uqm::io::File& fp);
bool DeleteResFile(const uqm::io::Directory& dir, const char* filename);

RESOURCE_INDEX InitResourceSystem(void);
void UninitResourceSystem(void);
bool InstallResTypeVectors(const char* res_type, ResourceLoadFun* loadFun, ResourceFreeFun* freeFun);
bool InstallResTextTypeVectors(const char* res_type, ResourceTextLoadFun* loadFun, ResourceFreeFun* freeFun);
void* res_GetResource(RESOURCE res);
void* res_DetachResource(RESOURCE res);
void res_FreeResource(RESOURCE res);
uint16_t CountResourceTypes(void);
uint32_t res_GetIntResource(RESOURCE res);
bool res_GetBooleanResource(RESOURCE res);
const char* res_GetResourceType(RESOURCE res);

// Loads a JSON resource index.
void LoadResourceIndex(const uqm::io::Directory& dir, const char* filename, const char* prefix);
// Converts the resource index 'oldName' in the old "key = TYPE:data" format to the JSON resource index 'newName', and
// renames the old file to '<oldName>.bak'. Returns false if there is no old file or it cannot be converted.
bool ConvertLegacyResourceIndex(const uqm::io::Directory& dir, const char* oldName, const char* newName);


#if 0 //defined(__cplusplus)
}
#endif

#include "libs/strlib.h"
#include "libs/gfxlib.h"

#if 0 //defined(__cplusplus)
extern "C" {
#endif

typedef STRING_TABLE DIRENTRY_REF;
typedef STRING DIRENTRY;

extern DIRENTRY_REF LoadDirEntryTable(const uqm::io::Directory& dir, const char* path, const uqm::io::Matcher& matcher);
#define CaptureDirEntryTable CaptureStringTable
#define ReleaseDirEntryTable ReleaseStringTable
#define DestroyDirEntryTable DestroyStringTable
#define GetDirEntryTableRef GetStringTable
#define GetDirEntryTableCount GetStringTableCount
#define GetDirEntryTableIndex GetStringTableIndex
#define SetAbsDirEntryTableIndex SetAbsStringTableIndex
#define SetRelDirEntryTableIndex SetRelStringTableIndex
#define GetDirEntryLength GetStringLengthBin
#define GetDirEntryAddress GetStringAddress

/* Key-Value resources */

bool res_HasKey(const char* key);

bool res_IsString(const char* key);
const char* res_GetString(const char* key);
void res_PutString(const char* key, const char* value);

bool res_IsInteger(const char* key);
int res_GetInteger(const char* key);
void res_PutInteger(const char* key, int value);

bool res_IsFloat(const char* key);
int res_GetFloat(const char* key);
void res_PutFloat(const char* key, float value);

bool res_IsBoolean(const char* key);
bool res_GetBoolean(const char* key);
void res_PutBoolean(const char* key, bool value);


bool res_Remove(const char* key);

#if 0 //defined(__cplusplus)
}
#endif

#endif /* LIBS_RESLIB_H_ */
