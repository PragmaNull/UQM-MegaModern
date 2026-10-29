// Old uio file API on top of libs/io.
#include "libs/io/FileSystem.h"
#include "libs/io/Path.h"
#include "libs/uio/io.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>

using uqm::io::Access;
using uqm::io::MountPosition;
using uqm::io::OpenMode;

struct uio_MountHandle
{
	uqm::io::Mount mount {};
};

namespace
{

struct DirListStorage : uio_DirList
{
	uqstl::vector<uqstl::string> storage {};
	uqstl::vector<const char*> pointers {};
};

void setErrno(const uqstl::error_code& error)
{
	errno = error ? error.default_error_condition().value() : 0;
}

template <typename T>
T fail(int code, T result)
{
	errno = code;
	return result;
}

MountPosition mountPosition(int flags)
{
	switch (flags & uio_MOUNT_LOCATION_MASK)
	{
		case uio_MOUNT_TOP:
			return MountPosition::Top;
		case uio_MOUNT_BELOW:
			return MountPosition::Below;
		case uio_MOUNT_ABOVE:
			return MountPosition::Above;
		default:
			return MountPosition::Bottom;
	}
}

Access mountAccess(int flags)
{
	return (flags & uio_MOUNT_RDONLY) != 0 ? Access::ReadOnly : Access::ReadWrite;
}

const uqm::io::Mount* relativeMount(uio_MountHandle* relative)
{
	return relative == nullptr ? nullptr : &relative->mount;
}

uio_MountHandle* wrapMount(uqstl::optional<uqm::io::Mount> mount, const uqstl::error_code& error)
{
	if (!mount)
	{
		setErrno(error);
		return nullptr;
	}
	return new uio_MountHandle {uqstl::move(*mount)};
}

uio_DirHandle* wrapDirectory(uio_Repository* repository, uqstl::optional<uqm::io::Directory> directory)
{
	if (!directory)
	{
		return fail<uio_DirHandle*>(ENOENT, nullptr);
	}
	return new uio_DirHandle {{uqstl::move(*directory)}, repository};
}

uqstl::string_view orEmpty(const char* text)
{
	return text == nullptr ? uqstl::string_view {} : uqstl::string_view {text};
}

void fillStat(struct stat* statBuf, const uqm::io::FileInfo& info)
{
	std::memset(statBuf, 0, sizeof(*statBuf));
	statBuf->st_mode = info.isDirectory() ? S_IFDIR : S_IFREG;
	statBuf->st_size = static_cast<decltype(statBuf->st_size)>(info.size);
	statBuf->st_mtime = static_cast<decltype(statBuf->st_mtime)>(info.modifiedTime);
}

uqstl::span<uqstl::byte> bytes(void* buf, size_t count)
{
	return {static_cast<uqstl::byte*>(buf), count};
}

uqstl::span<const uqstl::byte> bytes(const void* buf, size_t count)
{
	return {static_cast<const uqstl::byte*>(buf), count};
}

} // namespace

void uio_init(void)
{
}

void uio_unInit(void)
{
}

uio_Repository* uio_openRepository(int /*flags*/)
{
	return new uio_Repository {};
}

void uio_closeRepository(uio_Repository* repository)
{
	delete repository;
}

uio_MountHandle* uio_mountDir(uio_Repository* destRep, const char* mountPoint, uio_FileSystemID fsType, uio_DirHandle* sourceDir, const char* sourcePath, const char* inPath, uio_AutoMount** /*autoMount*/, int flags, uio_MountHandle* relative)
{
	uqstl::error_code error {};
	switch (fsType)
	{
		case uio_FSTYPE_STDIO:
			if (inPath == nullptr || sourceDir != nullptr)
			{
				return fail<uio_MountHandle*>(EINVAL, nullptr);
			}
			return wrapMount(destRep->fileSystem.mountDirectory(orEmpty(mountPoint), uqstl::filesystem::path {inPath}, mountAccess(flags), mountPosition(flags), relativeMount(relative), error), error);
		case uio_FSTYPE_ZIP:
			if (sourceDir == nullptr || sourcePath == nullptr || !uqm::io::path::normalize(orEmpty(inPath)).empty())
			{
				return fail<uio_MountHandle*>(EINVAL, nullptr);
			}
			if (mountAccess(flags) != Access::ReadOnly)
			{
				return fail<uio_MountHandle*>(EACCES, nullptr);
			}
			return wrapMount(destRep->fileSystem.mountArchive(orEmpty(mountPoint), *sourceDir, sourcePath, mountPosition(flags), relativeMount(relative), error), error);
		default:
			return fail<uio_MountHandle*>(ENODEV, nullptr);
	}
}

uio_MountHandle* uio_transplantDir(const char* mountPoint, uio_DirHandle* sourceDir, int flags, uio_MountHandle* relative)
{
	uqstl::error_code error {};
	return wrapMount(sourceDir->repository->fileSystem.mountTransplant(orEmpty(mountPoint), *sourceDir, mountAccess(flags), mountPosition(flags), relativeMount(relative), error), error);
}

int uio_unmountDir(uio_MountHandle* mountHandle)
{
	delete mountHandle;
	return 0;
}

uio_DirHandle* uio_openDir(uio_Repository* repository, const char* path, int /*flags*/)
{
	return wrapDirectory(repository, repository->fileSystem.root().subdir(orEmpty(path)));
}

uio_DirHandle* uio_openDirRelative(uio_DirHandle* base, const char* path, int /*flags*/)
{
	return wrapDirectory(base->repository, base->subdir(orEmpty(path)));
}

int uio_closeDir(uio_DirHandle* dirHandle)
{
	delete dirHandle;
	return 0;
}

uio_DirList* uio_getDirList(uio_DirHandle* dirHandle, const char* path, const char* pattern, match_MatchType matchType)
{
	uqm::io::Matcher::Kind kind {uqm::io::Matcher::Kind::Literal};
	switch (matchType)
	{
		case match_MATCH_LITERAL:
			break;
		case match_MATCH_PREFIX:
			kind = uqm::io::Matcher::Kind::Prefix;
			break;
		case match_MATCH_SUFFIX:
			kind = uqm::io::Matcher::Kind::Suffix;
			break;
		case match_MATCH_SUBSTRING:
			kind = uqm::io::Matcher::Kind::Substring;
			break;
		case match_MATCH_REGEX:
			kind = uqm::io::Matcher::Kind::Regex;
			break;
	}
	const uqstl::optional<uqm::io::Matcher> matcher {uqm::io::Matcher::create(kind, orEmpty(pattern))};
	if (!matcher)
	{
		return fail<uio_DirList*>(EINVAL, nullptr);
	}

	auto list {uqstl::make_unique<DirListStorage>()};
	uqstl::error_code error {};
	list->storage = dirHandle->list(orEmpty(path), *matcher, error);
	setErrno(error);
	for (const uqstl::string& name : list->storage)
	{
		list->pointers.push_back(name.c_str());
	}
	list->names = list->pointers.data();
	list->numNames = static_cast<int>(list->pointers.size());
	return list.release();
}

void uio_DirList_free(uio_DirList* dirList)
{
	delete static_cast<DirListStorage*>(dirList);
}

uio_Handle* uio_open(uio_DirHandle* dir, const char* file, int flags, mode_t /*mode*/)
{
	OpenMode mode {(flags & (O_WRONLY | O_RDWR)) == O_WRONLY ? OpenMode::Write : OpenMode::Read};
	if ((flags & O_RDWR) != 0)
	{
		mode |= OpenMode::Write;
	}
	if ((flags & O_CREAT) != 0)
	{
		mode |= OpenMode::Create;
	}
	if ((flags & O_EXCL) != 0)
	{
		mode |= OpenMode::Exclusive;
	}
	if ((flags & O_TRUNC) != 0)
	{
		mode |= OpenMode::Truncate;
	}
	if ((flags & O_APPEND) != 0)
	{
		mode |= OpenMode::Append;
	}
	if (O_TEXT != 0 && (flags & O_TEXT) != 0)
	{
		mode |= OpenMode::Text;
	}

	uqstl::error_code error {};
	uqstl::optional<uqm::io::File> opened {dir->open(orEmpty(file), mode, error)};
	if (!opened)
	{
		setErrno(error);
		return nullptr;
	}
	return new uio_Handle {{uqstl::move(*opened)}};
}

int uio_close(uio_Handle* handle)
{
	delete handle;
	return 0;
}

ssize_t uio_read(uio_Handle* handle, void* buf, size_t count)
{
	const size_t done {handle->read(bytes(buf, count))};
	if (done == 0 && handle->error())
	{
		setErrno(handle->error());
		return -1;
	}
	return static_cast<ssize_t>(done);
}

ssize_t uio_write(uio_Handle* handle, const void* buf, size_t count)
{
	const size_t done {handle->write(bytes(buf, count))};
	if (done == 0 && handle->error())
	{
		setErrno(handle->error());
		return -1;
	}
	return static_cast<ssize_t>(done);
}

int uio_fstat(uio_Handle* handle, struct stat* statBuf)
{
	if (!handle->isOpen())
	{
		return fail(EBADF, -1);
	}
	fillStat(statBuf, uqm::io::FileInfo {.size = handle->size()});
	return 0;
}

int uio_stat(uio_DirHandle* dir, const char* path, struct stat* statBuf)
{
	const uqstl::optional<uqm::io::FileInfo> info {dir->stat(orEmpty(path))};
	if (!info)
	{
		return fail(ENOENT, -1);
	}
	fillStat(statBuf, *info);
	return 0;
}

int uio_mkdir(uio_DirHandle* dir, const char* name, mode_t /*mode*/)
{
	const uqstl::error_code error {dir->makeDir(orEmpty(name))};
	setErrno(error);
	return error ? -1 : 0;
}

int uio_unlink(uio_DirHandle* dirHandle, const char* path)
{
	const uqstl::error_code error {dirHandle->remove(orEmpty(path))};
	setErrno(error);
	return error ? -1 : 0;
}

int uio_rename(uio_DirHandle* oldDir, const char* oldPath, uio_DirHandle* newDir, const char* newPath)
{
	const uqstl::error_code error {oldDir->rename(orEmpty(oldPath), *newDir, orEmpty(newPath))};
	setErrno(error);
	return error ? -1 : 0;
}

uio_Stream* uio_fopen(uio_DirHandle* dir, const char* path, const char* modeString)
{
	OpenMode mode {};
	switch (modeString[0])
	{
		case 'r':
			mode = OpenMode::Read;
			break;
		case 'w':
			mode = OpenMode::Write | OpenMode::Create | OpenMode::Truncate;
			break;
		case 'a':
			mode = OpenMode::Write | OpenMode::Create | OpenMode::Append;
			break;
		default:
			return fail<uio_Stream*>(EINVAL, nullptr);
	}
	for (const char* c {modeString + 1}; *c != '\0'; ++c)
	{
		if (*c == '+')
		{
			mode |= OpenMode::Read | OpenMode::Write;
		}
		else if (*c == 't')
		{
			mode |= OpenMode::Text;
		}
	}

	uqstl::error_code error {};
	uqstl::optional<uqm::io::File> opened {dir->open(orEmpty(path), mode, error)};
	if (!opened)
	{
		setErrno(error);
		return nullptr;
	}
	return new uio_Stream {{{uqstl::move(*opened)}}};
}

int uio_fclose(uio_Stream* stream)
{
	stream->close();
	const bool failed {static_cast<bool>(stream->error())};
	delete stream;
	return failed ? EOF : 0;
}

size_t uio_fread(void* buf, size_t size, size_t nmemb, uio_Stream* stream)
{
	if (size == 0 || nmemb == 0)
	{
		return 0;
	}
	return stream->read(bytes(buf, size * nmemb)) / size;
}

char* uio_fgets(char* buf, int size, uio_Stream* stream)
{
	if (size <= 0)
	{
		return nullptr;
	}
	return stream->readLine({buf, static_cast<size_t>(size)}) || size == 1 ? buf : nullptr;
}

int uio_fgetc(uio_Stream* stream)
{
	return stream->getChar();
}

int uio_ungetc(int c, uio_Stream* stream)
{
	return c != EOF && stream->unget() ? c : EOF;
}

int uio_backtrack(int rewinded_bytes, uio_Stream* stream)
{
	return rewinded_bytes >= 0 && stream->unget(static_cast<size_t>(rewinded_bytes)) ? 0 : -1;
}

int uio_fputc(int c, uio_Stream* stream)
{
	const uqstl::byte value {static_cast<uqstl::byte>(c)};
	return stream->write({&value, 1}) == 1 ? static_cast<unsigned char>(c) : EOF;
}

int uio_fputs(const char* s, uio_Stream* stream)
{
	const size_t length {std::strlen(s)};
	return stream->write(bytes(s, length)) == length ? 0 : EOF;
}

int uio_fseek(uio_Stream* stream, long offset, int whence)
{
	uqm::io::SeekFrom from {uqm::io::SeekFrom::Start};
	switch (whence)
	{
		case SEEK_SET:
			break;
		case SEEK_CUR:
			from = uqm::io::SeekFrom::Current;
			break;
		case SEEK_END:
			from = uqm::io::SeekFrom::End;
			break;
		default:
			return fail(EINVAL, -1);
	}
	if (!stream->seek(offset, from))
	{
		setErrno(stream->error());
		return -1;
	}
	return 0;
}

long uio_ftell(uio_Stream* stream)
{
	return static_cast<long>(stream->tell());
}

size_t uio_fwrite(const void* buf, size_t size, size_t nmemb, uio_Stream* stream)
{
	if (size == 0 || nmemb == 0)
	{
		return 0;
	}
	return stream->write(bytes(buf, size * nmemb)) / size;
}

int uio_fflush(uio_Stream* stream)
{
	if (stream == nullptr)
	{
		return 0;
	}
	return stream->flush() ? 0 : EOF;
}

int uio_feof(uio_Stream* stream)
{
	return stream->eof() ? 1 : 0;
}

int uio_ferror(uio_Stream* stream)
{
	return stream->error() ? 1 : 0;
}

void uio_clearerr(uio_Stream* stream)
{
	stream->clearError();
}

uio_Handle* uio_streamHandle(uio_Stream* stream)
{
	return stream;
}
