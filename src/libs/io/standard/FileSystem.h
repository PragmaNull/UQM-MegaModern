// Virtual file system that stacks host directories and zip archives.
#pragma once
#ifndef UQM_LIBS_IO_STANDARD_FILESYSTEM_H_
#define UQM_LIBS_IO_STANDARD_FILESYSTEM_H_

#include "core/stl/memory.h"
#include "core/stl/optional.h"
#include "core/stl/string.h"
#include "core/stl/vector.h"
#include "libs/io/Matcher.h"
#include "libs/io/Types.h"
#include "libs/io/standard/File.h"

#include <filesystem>
#include <shared_mutex>
#include <system_error>

namespace uqm::io::standard
{

class FileSystem;
class MountSource;

// A directory path in a file system. The file system must stay alive while the directory is in use.
class Directory
{
public:
	Directory(FileSystem& fileSystem, uqstl::string path);

	// Returns nullopt and sets 'error' if the file cannot be opened.
	[[nodiscard]] uqstl::optional<File> open(uqstl::string_view path, OpenMode mode, uqstl::error_code& error) const;
	[[nodiscard]] uqstl::optional<FileInfo> stat(uqstl::string_view path) const;
	[[nodiscard]] bool exists(uqstl::string_view path) const;
	// Returns the sorted names in the directory 'path' that 'matcher' accepts.
	[[nodiscard]] uqstl::vector<uqstl::string> list(uqstl::string_view path, const Matcher& matcher, uqstl::error_code& error) const;
	// Returns nullopt if 'path' is not a directory.
	[[nodiscard]] uqstl::optional<Directory> subdir(uqstl::string_view path) const;
	uqstl::error_code makeDir(uqstl::string_view path) const;
	uqstl::error_code remove(uqstl::string_view path) const;
	uqstl::error_code rename(uqstl::string_view from, const Directory& toDirectory, uqstl::string_view to) const;

	// The normal path from the root of the file system.
	[[nodiscard]] const uqstl::string& path() const { return m_path; }
	[[nodiscard]] FileSystem& fileSystem() const { return *m_fileSystem; }

private:
	[[nodiscard]] uqstl::string resolve(uqstl::string_view path) const;

	FileSystem* m_fileSystem {};
	uqstl::string m_path {};
};

// A mount in a file system. Destroying it removes the mount.
class Mount
{
public:
	Mount() = default;
	~Mount();

	Mount(const Mount&) = delete;
	Mount& operator=(const Mount&) = delete;
	Mount(Mount&& other) noexcept;
	Mount& operator=(Mount&& other) noexcept;

	[[nodiscard]] bool isMounted() const { return m_fileSystem != nullptr; }
	void unmount();

private:
	friend class FileSystem;
	Mount(FileSystem& fileSystem, uint64_t id);

	FileSystem* m_fileSystem {};
	uint64_t m_id {0};
};

class FileSystem
{
public:
	using File = standard::File;
	using Directory = standard::Directory;
	using Mount = standard::Mount;

	FileSystem();
	~FileSystem();

	FileSystem(const FileSystem&) = delete;
	FileSystem& operator=(const FileSystem&) = delete;
	FileSystem(FileSystem&&) = delete;
	FileSystem& operator=(FileSystem&&) = delete;

	[[nodiscard]] Directory root();

	// Mounts the host directory 'hostPath' at 'mountPoint'.
	[[nodiscard]] uqstl::optional<Mount> mountDirectory(uqstl::string_view mountPoint, const uqstl::filesystem::path& hostPath, Access access, MountPosition position, const Mount* relative, uqstl::error_code& error);
	// Mounts the zip archive 'archivePath' in 'directory' at 'mountPoint', read-only.
	[[nodiscard]] uqstl::optional<Mount> mountArchive(uqstl::string_view mountPoint, const Directory& directory, uqstl::string_view archivePath, MountPosition position, const Mount* relative, uqstl::error_code& error);
	// Mounts what 'source' currently shows at 'mountPoint'.
	[[nodiscard]] uqstl::optional<Mount> mountTransplant(uqstl::string_view mountPoint, const Directory& source, Access access, MountPosition position, const Mount* relative, uqstl::error_code& error);

private:
	friend class standard::Directory;
	friend class standard::Mount;

	struct MountEntry
	{
		uint64_t id {0};
		uqstl::string mountPoint {};
		uqstl::shared_ptr<const MountSource> source {};
		uqstl::string sourcePath {};
		Access access {Access::ReadOnly};
	};

	// An entry that one mount has.
	struct Found
	{
		size_t index {0};
		uqstl::string sourcePath {};
		FileInfo info {};
	};

	[[nodiscard]] uqstl::optional<File> open(const uqstl::string& path, OpenMode mode, uqstl::error_code& error);
	[[nodiscard]] uqstl::optional<FileInfo> stat(const uqstl::string& path) const;
	[[nodiscard]] uqstl::vector<uqstl::string> list(const uqstl::string& path, const Matcher& matcher, uqstl::error_code& error) const;
	uqstl::error_code makeDir(const uqstl::string& path);
	uqstl::error_code remove(const uqstl::string& path);
	uqstl::error_code rename(const uqstl::string& from, const uqstl::string& to);
	void unmount(uint64_t id);

	// These functions expect the caller to hold m_mutex.
	[[nodiscard]] uqstl::optional<Found> findLocked(const uqstl::string& path) const;
	[[nodiscard]] uqstl::optional<FileInfo> statLocked(const uqstl::string& path) const;
	[[nodiscard]] bool isMountAncestorLocked(const uqstl::string& path) const;
	// Returns the index of the highest writable mount above 'below' that has 'path'.
	[[nodiscard]] uqstl::optional<size_t> writableMountLocked(const uqstl::string& path, size_t below) const;
	uqstl::error_code makeParentsLocked(const MountEntry& mount, uqstl::string_view sourcePath) const;
	[[nodiscard]] uqstl::optional<Mount> insertLocked(uqstl::vector<MountEntry> entries, MountPosition position, const Mount* relative, uqstl::error_code& error);

	mutable uqstl::shared_mutex m_mutex {};
	// The highest mount is first.
	uqstl::vector<MountEntry> m_mounts {};
	uint64_t m_nextId {1};
};

} // namespace uqm::io::standard

#endif // UQM_LIBS_IO_STANDARD_FILESYSTEM_H_
