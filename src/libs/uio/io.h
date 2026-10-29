// Old uio file API. It forwards to libs/io until the callers use libs/io directly.
#pragma once
#ifndef LIBS_UIO_IO_H_
#define LIBS_UIO_IO_H_

#include "libs/io/FileSystem.h"

#include <cstdint>
#include <cstdio>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _MSC_VER
typedef int ssize_t;
typedef unsigned short mode_t;
#endif

#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif
#ifndef O_TEXT
#define O_TEXT 0
#endif

struct uio_Repository
{
	uqm::io::FileSystem fileSystem {};
};
typedef struct uio_MountHandle uio_MountHandle;

// The handles are io types, so migrated code can take them as Directory or File.
struct uio_DirHandle : uqm::io::Directory
{
	uio_Repository* repository {};
};

struct uio_Handle : uqm::io::File
{
};

struct uio_Stream : uio_Handle
{
};

typedef int uio_FileSystemID;
#define uio_FSTYPE_STDIO 1
#define uio_FSTYPE_ZIP 2

#define uio_MOUNT_RDONLY (1 << 1)
#define uio_MOUNT_BOTTOM (0 << 2)
#define uio_MOUNT_TOP (1 << 2)
#define uio_MOUNT_BELOW (2 << 2)
#define uio_MOUNT_ABOVE (3 << 2)
#define uio_MOUNT_LOCATION_MASK (3 << 2)

#define uio_OD_ROOT 1

typedef enum
{
	match_MATCH_LITERAL = 0,
	match_MATCH_PREFIX,
	match_MATCH_SUFFIX,
	match_MATCH_SUBSTRING,
	match_MATCH_REGEX,
} match_MatchType;

// Not supported. Pass a list with only nullptr.
typedef struct uio_AutoMount uio_AutoMount;

struct uio_DirList
{
	const char** names;
	int numNames;
};
typedef struct uio_DirList uio_DirList;

void uio_init(void);
void uio_unInit(void);

uio_Repository* uio_openRepository(int flags);
// Destroy all mount and directory handles of the repository first.
void uio_closeRepository(uio_Repository* repository);

// For uio_FSTYPE_STDIO, 'inPath' is the host directory. For uio_FSTYPE_ZIP, 'sourcePath' in 'sourceDir' is the archive and 'inPath' must be "/".
uio_MountHandle* uio_mountDir(uio_Repository* destRep, const char* mountPoint, uio_FileSystemID fsType, uio_DirHandle* sourceDir, const char* sourcePath, const char* inPath, uio_AutoMount** autoMount, int flags, uio_MountHandle* relative);
uio_MountHandle* uio_transplantDir(const char* mountPoint, uio_DirHandle* sourceDir, int flags, uio_MountHandle* relative);
int uio_unmountDir(uio_MountHandle* mountHandle);

uio_DirHandle* uio_openDir(uio_Repository* repository, const char* path, int flags);
uio_DirHandle* uio_openDirRelative(uio_DirHandle* base, const char* path, int flags);
int uio_closeDir(uio_DirHandle* dirHandle);
uio_DirList* uio_getDirList(uio_DirHandle* dirHandle, const char* path, const char* pattern, match_MatchType matchType);
void uio_DirList_free(uio_DirList* dirList);

uio_Handle* uio_open(uio_DirHandle* dir, const char* file, int flags, mode_t mode);
int uio_close(uio_Handle* handle);
ssize_t uio_read(uio_Handle* handle, void* buf, size_t count);
ssize_t uio_write(uio_Handle* handle, const void* buf, size_t count);
int uio_fstat(uio_Handle* handle, struct stat* statBuf);
int uio_stat(uio_DirHandle* dir, const char* path, struct stat* statBuf);
int uio_mkdir(uio_DirHandle* dir, const char* name, mode_t mode);
int uio_unlink(uio_DirHandle* dirHandle, const char* path);
int uio_rename(uio_DirHandle* oldDir, const char* oldPath, uio_DirHandle* newDir, const char* newPath);

uio_Stream* uio_fopen(uio_DirHandle* dir, const char* path, const char* mode);
int uio_fclose(uio_Stream* stream);
size_t uio_fread(void* buf, size_t size, size_t nmemb, uio_Stream* stream);
char* uio_fgets(char* buf, int size, uio_Stream* stream);
int uio_fgetc(uio_Stream* stream);
#define uio_getc uio_fgetc
int uio_ungetc(int c, uio_Stream* stream);
int uio_backtrack(int rewinded_bytes, uio_Stream* stream);
int uio_fputc(int c, uio_Stream* stream);
#define uio_putc uio_fputc
int uio_fputs(const char* s, uio_Stream* stream);
int uio_fseek(uio_Stream* stream, long offset, int whence);
long uio_ftell(uio_Stream* stream);
size_t uio_fwrite(const void* buf, size_t size, size_t nmemb, uio_Stream* stream);
int uio_fflush(uio_Stream* stream);
int uio_feof(uio_Stream* stream);
int uio_ferror(uio_Stream* stream);
void uio_clearerr(uio_Stream* stream);
uio_Handle* uio_streamHandle(uio_Stream* stream);

#endif // LIBS_UIO_IO_H_
