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
#include <system_error>

namespace uqm::io::standard
{

class MountTable;

// A directory path in a file system. It keeps the mounts alive.
class Directory
{
public:
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

private:
	friend class FileSystem;
	friend class MountTable;
	Directory(uqstl::shared_ptr<MountTable> table, uqstl::string path);

	[[nodiscard]] uqstl::string resolve(uqstl::string_view path) const;

	uqstl::shared_ptr<MountTable> m_table {};
	uqstl::string m_path {};
};

// A mount in a file system. Destroying it removes the mount. It can outlive the file system.
class Mount
{
public:
	Mount() = default;
	~Mount();

	Mount(const Mount&) = delete;
	Mount& operator=(const Mount&) = delete;
	Mount(Mount&& other) noexcept = default;
	Mount& operator=(Mount&& other) noexcept;

	[[nodiscard]] bool isMounted() const { return !m_table.expired(); }
	void unmount();

private:
	friend class MountTable;
	Mount(const uqstl::shared_ptr<MountTable>& table, uint64_t id);

	uqstl::weak_ptr<MountTable> m_table {};
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
	// Returns a directory over only the zip archive 'archivePath' in 'directory', without a mount in this file system.
	[[nodiscard]] uqstl::optional<Directory> openArchive(const Directory& directory, uqstl::string_view archivePath, uqstl::error_code& error);
	// Mounts what 'source' currently shows at 'mountPoint'.
	[[nodiscard]] uqstl::optional<Mount> mountTransplant(uqstl::string_view mountPoint, const Directory& source, Access access, MountPosition position, const Mount* relative, uqstl::error_code& error);

private:
	uqstl::shared_ptr<MountTable> m_table {};
};

} // namespace uqm::io::standard

#endif // UQM_LIBS_IO_STANDARD_FILESYSTEM_H_
