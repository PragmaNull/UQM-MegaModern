// The mount stack of a file system, shared by its directories.
#pragma once
#ifndef UQM_LIBS_IO_STANDARD_MOUNTTABLE_H_
#define UQM_LIBS_IO_STANDARD_MOUNTTABLE_H_

#include "libs/io/standard/FileSystem.h"
#include "libs/io/standard/MountSource.h"

#include <shared_mutex>

namespace uqm::io::standard
{

class MountTable : public uqstl::enable_shared_from_this<MountTable>
{
public:
	[[nodiscard]] Directory root();

	[[nodiscard]] uqstl::optional<Mount> mountSource(uqstl::string_view mountPoint, uqstl::shared_ptr<const MountSource> source, Access access, MountPosition position, const Mount* relative, uqstl::error_code& error);
	[[nodiscard]] uqstl::optional<Mount> mountArchive(uqstl::string_view mountPoint, const Directory& directory, uqstl::string_view archivePath, MountPosition position, const Mount* relative, uqstl::error_code& error);
	// Returns a directory over a new mount table that holds only the zip archive 'archivePath' in 'directory'.
	[[nodiscard]] uqstl::optional<Directory> openArchive(const Directory& directory, uqstl::string_view archivePath, uqstl::error_code& error);
	[[nodiscard]] uqstl::optional<Mount> mountTransplant(uqstl::string_view mountPoint, const Directory& source, Access access, MountPosition position, const Mount* relative, uqstl::error_code& error);
	void unmount(uint64_t id);

	// Paths are normal paths from the root.
	[[nodiscard]] uqstl::optional<File> open(const uqstl::string& path, OpenMode mode, uqstl::error_code& error) const;
	[[nodiscard]] uqstl::optional<FileInfo> stat(const uqstl::string& path) const;
	[[nodiscard]] uqstl::vector<uqstl::string> list(const uqstl::string& path, const Matcher& matcher, uqstl::error_code& error) const;
	uqstl::error_code makeDir(const uqstl::string& path) const;
	uqstl::error_code remove(const uqstl::string& path) const;
	uqstl::error_code rename(const uqstl::string& from, const uqstl::string& to) const;

private:
	struct MountEntry
	{
		uint64_t id {0};
		uqstl::string mountPoint {};
		uqstl::shared_ptr<const MountSource> source {};
		uqstl::string sourcePath {};
		Access access {Access::ReadOnly};

		// Returns the path in the source for 'path', which must be within the mount point.
		[[nodiscard]] uqstl::string sourcePathFor(uqstl::string_view path) const;
		[[nodiscard]] bool isWritable() const { return access == Access::ReadWrite; }
	};

	// A mount to create an entry in, and the path in its source.
	struct WriteTarget
	{
		const MountEntry* mount {};
		uqstl::string sourcePath {};
	};

	// An entry that one mount has.
	struct Found
	{
		size_t index {0};
		uqstl::string sourcePath {};
		FileInfo info {};
	};

	[[nodiscard]] uqstl::shared_ptr<const MountSource> openZip(const Directory& directory, uqstl::string_view archivePath, uqstl::error_code& error) const;

	// These functions expect the caller to hold m_mutex.
	[[nodiscard]] uqstl::optional<Found> findLocked(const uqstl::string& path) const;
	[[nodiscard]] uqstl::optional<FileInfo> statLocked(const uqstl::string& path) const;
	[[nodiscard]] bool isMountAncestorLocked(const uqstl::string& path) const;
	// Returns the index of the highest writable mount above 'below' that has 'path'.
	[[nodiscard]] uqstl::optional<size_t> writableMountLocked(const uqstl::string& path, size_t below) const;
	uqstl::error_code makeParentsLocked(const MountEntry& mount, uqstl::string_view sourcePath) const;
	// Finds the highest writable mount above 'below' that has 'path', and creates the parent directories in it.
	// Returns nullopt and sets 'error', to 'noMount' if there is no such mount.
	[[nodiscard]] uqstl::optional<WriteTarget> writeTargetLocked(const uqstl::string& path, size_t below, uqstl::errc noMount, uqstl::error_code& error) const;
	[[nodiscard]] uqstl::optional<Mount> insertLocked(uqstl::vector<MountEntry> entries, MountPosition position, const Mount* relative, uqstl::error_code& error);

	mutable uqstl::shared_mutex m_mutex {};
	// The highest mount is first.
	uqstl::vector<MountEntry> m_mounts {};
	uint64_t m_nextId {1};
};

} // namespace uqm::io::standard

#endif // UQM_LIBS_IO_STANDARD_MOUNTTABLE_H_
