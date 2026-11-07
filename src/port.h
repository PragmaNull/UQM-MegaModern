#pragma once
#ifndef UQM_PORT_H
#define UQM_PORT_H

#include "config.h"

// For quelching intentional fallthrough warnings in switch statements.
#if defined(__GNUC__) && __GNUC__ >= 7
#define FALLTHROUGH __attribute__((fallthrough))
#else
#define FALLTHROUGH
#endif

#ifdef NEVER
// Microsoft Windows headers expect this to be set. The MSVC compiler sets
// it, but MinGW doesn't.
#if defined(_X86_)
#define _M_IX86
#elif defined(_IA64_)
#define _M_IA64
#elif defined(__amd64__)
#define _M_AMD64
#define _M_X64
#elif defined(__m68k__)
#define _68K_
#elif defined(__ppc__)
#define _M_PPC
#endif
#endif


// PragmaNull: Stuff like this gives C/C++ a bad name.
// Turn them back on. Let's not be lazy.  kthnxbye!
// |  |  |
// v  v  v
//
//// Compilation warnings:
//#ifdef _MSC_VER
//// UQM uses a lot of functions that can be used unsafely, but it uses them
//// in a safe way. The warnings about these functions however may drown out
//// serious warnings, so we turn them off.
//#define _CRT_SECURE_NO_DEPRECATE
//
//// Escalate some warnings we consider important
//// "'operator' : 'identifier1' indirection to slightly different base
////   types from 'identifier2'
//#pragma warning(3 :4057)
//// "unreferenced formal parameter"
//#pragma warning(3 :4100)
//// "'function' : unreferenced local function has been removed"
//#pragma warning(3 :4505)
//// "local variable 'name' may be used without having been initialized"
//#pragma warning(3 :4701)
//
//// Downgrade some warnings we consider unimportant
//// "'operator' conversion from 'type1' to 'type2', possible loss of data"
//#pragma warning(4 :4244)
//#endif


#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#endif

// Using "HAVE_STRCASECMP_UQM" instead of "HAVE_STRCASECMP" as the latter
// conflicts with SDL.
#if !defined(HAVE_STRICMP) && !defined(HAVE_STRCASECMP_UQM)
#error Neither stricmp() nor strcasecmp() is available.
#elif !defined(HAVE_STRICMP)
#define stricmp strcasecmp
#elif !defined(HAVE_STRCASECMP_UQM)
#define strcasecmp _stricmp
#else
// We should take care not to define anything if both strcasecmp() and
// stricmp() are defined, as one might exist as a macro to the other.
#endif


#if !defined(_MSC_VER) && !defined(HAVE_READDIR_R)
#include <dirent.h>
#if 0 //defined(__cplusplus)
extern "C" {
#endif
int readdir_r(DIR* dirp, struct dirent* entry, struct dirent** result);
#if 0 //defined(__cplusplus)
}
#endif
#endif

// Directories
#ifdef WIN32
#include <stdlib.h>
#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif
#define NAME_MAX _MAX_FNAME
// _MAX_DIR and FILENAME_MAX could also be candidates.
// If anyone can tell me which one matches NAME_MAX, please
// let me know. - SvdB
#elif defined(_WIN32_WCE)
#include <sys/syslimits.h>
#else
#include <limits.h>
/* PATH_MAX is per POSIX defined in <limits.h>, but:
		 * "A definition of one of the values from Table 2.6 shall bea
		 * omitted from <limits.h> on specific implementations where the
		 * corresponding value is equal to or greater than the
		 * stated minimum, but where the value can vary depending
		 * on the file to which it is applied. The actual value supported
		 * for a specific pathname shall be provided by the pathconf()
		 * function."
		 * _POSIX_NAME_MAX will provide a minimum (14).
		 * This is relevant (at least) for Solaris.
		 */
#ifndef NAME_MAX
#define NAME_MAX _POSIX_NAME_MAX
#endif
#endif

// Some types
#ifdef _MSC_VER
typedef int ssize_t;
typedef unsigned short mode_t;
#endif

// Directories
#include <sys/stat.h>
#ifdef _MSC_VER
#define MKDIR(name, mode) ((void)mode, _mkdir(name))
#elif defined(__MINGW32__)
#define MKDIR(name, mode) ((void)mode, mkdir(name))
#else
#define MKDIR mkdir
#endif
#ifdef _MSC_VER
#include <direct.h>
#define chdir _chdir
#define getcwd _getcwd
#define access _access
#define F_OK 0
#define W_OK 2
#define R_OK 4
#endif

// Memory
#ifdef WIN32
#ifdef __MINGW32__
#include <malloc.h>
#elif defined(_MSC_VER)
#define alloca _alloca
#endif
#elif defined(__linux__) || defined(__svr4__)
#include <alloca.h>
#endif

// String formatting
#ifdef _MSC_VER
#include <stdarg.h>
// Defined in port.c
#if 0 //defined(__cplusplus)
extern "C" {
#endif
#if (_MSC_VER < 1500)
// Use Windows SDK version of snprintf & vsnprintf when building with Visual Studio
// versions higher than 2005
int snprintf(char* str, size_t size, const char* format, ...);
int vsnprintf(char* str, size_t size, const char* format, va_list args);
#endif
#if 0 //defined(__cplusplus)
}
#endif

#endif /* _MSC_VER */


#ifndef HAVE_WCHAR_T
typedef unsigned short wchar_t;
#endif

#ifndef HAVE_WINT_T
typedef unsigned int wint_t;
#endif

// Use SDL_INCLUDE to portably include the SDL files from the right location.
// The SDL_DIR definition is provided by the build configuration.
//#define "file) #file

// Mark a function as using printf-style function arguments, so that
// extra consistency checks can be made by the compiler.
// The first argument to PRINTF_FUNCTION and VPRINTF_FUNCTION is the
// index of the format string argument, the second is the index of
// the first argument which is specified in the format string (in the
// case of PRINTF_FUNCTION) or of the va_list argument (in the case of
// VPRINTF_FUNCTION).
#ifdef __GNUC__
#define PRINTF_FUNCTION(formatArg, firstArg) \
	__attribute__((format(printf, formatArg, firstArg)))
#define VPRINTF_FUNCTION(formatArg) \
	__attribute__((format(printf, formatArg, 0)))
#else
#define PRINTF_FUNCTION(formatArg, firstArg)
#define VPRINTF_FUNCTION(formatArg)
#endif

#if defined(__GNUC__)
#define _NORETURN __attribute__((noreturn))
#else
#define _NORETURN
#endif

// Buffered vs. unbuffered logfile
// stderr is normally unbuffered when connected to a terminal, but it
// will be buffered when connected to a file, when a --logfile argument
// is passed to uqm.
// Unbuffered output is slower, which can be significant if much debug output
// is requested, but after a crash occurs the logfile will still be up to
// date.
// On platforms where there is no console, having up-to-date log files
// after a crash is valuable enough to make the logfile unbuffered by
// default there.
#if defined(_WIN32_WCE)
#define UNBUFFERED_LOGFILE
#endif

// Variations in path handling
#if defined(WIN32) || defined(__SYMBIAN32__)
// HAVE_DRIVE_LETTERS is defined to signify that DOS/Windows style drive
// letters are to be recognised on this platform.
#define HAVE_DRIVE_LETTERS
// BACKSLASH_IS_PATH_SEPARATOR is defined to signify that the backslash
// character is to be recognised as a path separator on this platform.
// This does not affect the acceptance of forward slashes as path
// separators.
#define BACKSLASH_IS_PATH_SEPARATOR
#endif
#if defined(WIN32)
// HAVE_UNC_PATHS is defined to signify that Universal Naming Convention
// style paths are to be recognised on this platform.
#define HAVE_UNC_PATHS
// HAVE_CWD_PER_DRIVE is defined to signify that every drive has its own
// current working directory.
#define HAVE_CWD_PER_DRIVE
#endif
// REJECT_DRIVE_PATH_WITHOUT_SLASH can also be defined, if paths of the form
// "d:foo/bar" (without a slash after the drive letter) are to be rejected.

#endif // UQM_PORT_H
