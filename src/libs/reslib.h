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
#include "libs/compiler.h"
#include "port.h"
#include "libs/memlib.h"
#include "libs/uio.h"
#include "libs/io/FileSystem.h"

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

extern const char* _cur_resfile_name;

typedef void(ResourceLoadFun)(const char* pathname, RESOURCE_DATA* resdata);
typedef bool(ResourceFreeFun)(void* handle);
typedef void(ResourceStringFun)(RESOURCE_DATA* handle, char* buf, unsigned int size);

// Loads a resource from a file. A directory resource gets a nullptr file and length 1.
typedef void*(ResourceLoadFileFun)(uqm::io::File * fp, uint32_t len);

void* LoadResourceFromPath(const char* pathname, ResourceLoadFileFun fn);

uqstl::optional<uqm::io::File> res_OpenResFile(const uqm::io::Directory& dir, const char* filename, uqm::io::OpenMode mode);
// Calls 'loadFun' for 'filename' while _cur_resfile_name is 'filename'. Returns nullptr if 'filename' cannot be opened.
void* res_LoadResFile(const uqm::io::Directory& dir, const char* filename, ResourceLoadFileFun* loadFun);
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
bool InstallResTypeVectors(const char* res_type, ResourceLoadFun* loadFun, ResourceFreeFun* freeFun, ResourceStringFun* stringFun);
void* res_GetResource(RESOURCE res);
void* res_DetachResource(RESOURCE res);
void res_FreeResource(RESOURCE res);
uint16_t CountResourceTypes(void);
uint32_t res_GetIntResource(RESOURCE res);
bool res_GetBooleanResource(RESOURCE res);
const char* res_GetResourceType(RESOURCE res);

void LoadResourceIndex(const uqm::io::Directory& dir, const char* filename, const char* prefix);
void SaveResourceIndex(const uqm::io::Directory& dir, const char* rmpfile, const char* root, bool strip_root);

void* GetResourceData(uqm::io::File& fp, uint32_t length);

#define AllocResourceData HMalloc
bool FreeResourceData(void*);

#if 0 //defined(__cplusplus)
}
#endif

#include "libs/strlib.h"
#include "libs/gfxlib.h"

#if 0 //defined(__cplusplus)
extern "C" {
#endif
// For Color

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

bool res_IsColor(const char* key);
Color res_GetColor(const char* key);
void res_PutColor(const char* key, Color value);

bool res_Remove(const char* key);

#if 0 //defined(__cplusplus)
}
#endif

#endif /* LIBS_RESLIB_H_ */
