// Virtual file system that stacks host directories and zip archives.
#include "libs/io/standard/FileSystem.h"

#include "libs/io/Path.h"
#include "libs/io/standard/DirectorySource.h"
#include "libs/io/standard/MountTable.h"

namespace uqm::io::standard
{

// Directory

Directory::Directory(uqstl::shared_ptr<MountTable> table, uqstl::string path)
	: m_table {uqstl::move(table)}
	, m_path {uqstl::move(path)}
{
}

uqstl::string Directory::resolve(uqstl::string_view path) const
{
	return path::join(m_path, path);
}

uqstl::optional<File> Directory::open(uqstl::string_view path, OpenMode mode, uqstl::error_code& error) const
{
	return m_table->open(resolve(path), mode, error);
}

uqstl::optional<FileInfo> Directory::stat(uqstl::string_view path) const
{
	return m_table->stat(resolve(path));
}

bool Directory::exists(uqstl::string_view path) const
{
	return stat(path).has_value();
}

uqstl::vector<uqstl::string> Directory::list(uqstl::string_view path, const Matcher& matcher, uqstl::error_code& error) const
{
	return m_table->list(resolve(path), matcher, error);
}

uqstl::optional<Directory> Directory::subdir(uqstl::string_view path) const
{
	uqstl::string full {resolve(path)};
	const uqstl::optional<FileInfo> info {m_table->stat(full)};
	if (!info || !info->isDirectory())
	{
		return uqstl::nullopt;
	}
	return Directory {m_table, uqstl::move(full)};
}

uqstl::error_code Directory::makeDir(uqstl::string_view path) const
{
	return m_table->makeDir(resolve(path));
}

uqstl::error_code Directory::remove(uqstl::string_view path) const
{
	return m_table->remove(resolve(path));
}

uqstl::error_code Directory::rename(uqstl::string_view from, const Directory& toDirectory, uqstl::string_view to) const
{
	if (toDirectory.m_table != m_table)
	{
		return uqstl::make_error_code(uqstl::errc::cross_device_link);
	}
	return m_table->rename(resolve(from), toDirectory.resolve(to));
}

// Mount

Mount::Mount(const uqstl::shared_ptr<MountTable>& table, uint64_t id)
	: m_table {table}
	, m_id {id}
{
}

Mount::~Mount()
{
	unmount();
}

Mount& Mount::operator=(Mount&& other) noexcept
{
	if (this != &other)
	{
		unmount();
		m_table = uqstl::exchange(other.m_table, {});
		m_id = uqstl::exchange(other.m_id, 0);
	}
	return *this;
}

void Mount::unmount()
{
	if (const uqstl::shared_ptr<MountTable> table {m_table.lock()})
	{
		table->unmount(m_id);
	}
	m_table.reset();
}

// FileSystem

FileSystem::FileSystem()
	: m_table {uqstl::make_shared<MountTable>()}
{
}

FileSystem::~FileSystem() = default;

Directory FileSystem::root()
{
	return m_table->root();
}

uqstl::optional<Mount> FileSystem::mountDirectory(uqstl::string_view mountPoint, const uqstl::filesystem::path& hostPath, Access access, MountPosition position, const Mount* relative, uqstl::error_code& error)
{
	uqstl::error_code statusError {};
	if (!uqstl::filesystem::is_directory(hostPath, statusError))
	{
		error = statusError ? statusError : uqstl::make_error_code(uqstl::errc::not_a_directory);
		return uqstl::nullopt;
	}
	return m_table->mountSource(mountPoint, uqstl::make_shared<DirectorySource>(hostPath), access, position, relative, error);
}

uqstl::optional<Mount> FileSystem::mountArchive(uqstl::string_view mountPoint, const Directory& directory, uqstl::string_view archivePath, MountPosition position, const Mount* relative, uqstl::error_code& error)
{
	return m_table->mountArchive(mountPoint, directory, archivePath, position, relative, error);
}

uqstl::optional<Directory> FileSystem::openArchive(const Directory& directory, uqstl::string_view archivePath, uqstl::error_code& error)
{
	return m_table->openArchive(directory, archivePath, error);
}

uqstl::optional<Mount> FileSystem::mountTransplant(uqstl::string_view mountPoint, const Directory& source, Access access, MountPosition position, const Mount* relative, uqstl::error_code& error)
{
	return m_table->mountTransplant(mountPoint, source, access, position, relative, error);
}

} // namespace uqm::io::standard
