// Virtual file system that stacks host directories and zip archives.
#include "libs/io/standard/FileSystem.h"

#include "libs/io/Path.h"
#include "libs/io/standard/DirectorySource.h"
#include "libs/io/standard/ZipSource.h"

#include <mutex>
#include <set>

namespace uqm::io::standard
{

namespace
{

constexpr size_t c_copyBufferSize {0x10000};

Access combine(Access a, Access b)
{
	return a == Access::ReadWrite && b == Access::ReadWrite ? Access::ReadWrite : Access::ReadOnly;
}

uqstl::error_code copyStream(Stream& from, Stream& to)
{
	uqstl::vector<uqstl::byte> buffer(c_copyBufferSize);
	uqstl::error_code error {};
	while (true)
	{
		const size_t count {from.read(buffer, error)};
		if (error || count == 0)
		{
			return error;
		}
		if (to.write(uqstl::span {buffer}.first(count), error) != count)
		{
			return error ? error : uqstl::make_error_code(uqstl::errc::io_error);
		}
	}
}

} // namespace

// Directory

Directory::Directory(FileSystem& fileSystem, uqstl::string path)
	: m_fileSystem {&fileSystem}
	, m_path {uqstl::move(path)}
{
}

uqstl::string Directory::resolve(uqstl::string_view path) const
{
	return path::join(m_path, path);
}

uqstl::optional<File> Directory::open(uqstl::string_view path, OpenMode mode, uqstl::error_code& error) const
{
	return m_fileSystem->open(resolve(path), mode, error);
}

uqstl::optional<FileInfo> Directory::stat(uqstl::string_view path) const
{
	return m_fileSystem->stat(resolve(path));
}

bool Directory::exists(uqstl::string_view path) const
{
	return stat(path).has_value();
}

uqstl::vector<uqstl::string> Directory::list(uqstl::string_view path, const Matcher& matcher, uqstl::error_code& error) const
{
	return m_fileSystem->list(resolve(path), matcher, error);
}

uqstl::optional<Directory> Directory::subdir(uqstl::string_view path) const
{
	uqstl::string full {resolve(path)};
	const uqstl::optional<FileInfo> info {m_fileSystem->stat(full)};
	if (!info || !info->isDirectory())
	{
		return uqstl::nullopt;
	}
	return Directory {*m_fileSystem, uqstl::move(full)};
}

uqstl::error_code Directory::makeDir(uqstl::string_view path) const
{
	return m_fileSystem->makeDir(resolve(path));
}

uqstl::error_code Directory::remove(uqstl::string_view path) const
{
	return m_fileSystem->remove(resolve(path));
}

uqstl::error_code Directory::rename(uqstl::string_view from, const Directory& toDirectory, uqstl::string_view to) const
{
	if (toDirectory.m_fileSystem != m_fileSystem)
	{
		return uqstl::make_error_code(uqstl::errc::cross_device_link);
	}
	return m_fileSystem->rename(resolve(from), toDirectory.resolve(to));
}

// Mount

Mount::Mount(FileSystem& fileSystem, uint64_t id)
	: m_fileSystem {&fileSystem}
	, m_id {id}
{
}

Mount::~Mount()
{
	unmount();
}

Mount::Mount(Mount&& other) noexcept
	: m_fileSystem {uqstl::exchange(other.m_fileSystem, nullptr)}
	, m_id {uqstl::exchange(other.m_id, 0)}
{
}

Mount& Mount::operator=(Mount&& other) noexcept
{
	if (this != &other)
	{
		unmount();
		m_fileSystem = uqstl::exchange(other.m_fileSystem, nullptr);
		m_id = uqstl::exchange(other.m_id, 0);
	}
	return *this;
}

void Mount::unmount()
{
	if (m_fileSystem != nullptr)
	{
		uqstl::exchange(m_fileSystem, nullptr)->unmount(m_id);
	}
}

// FileSystem

FileSystem::FileSystem() = default;
FileSystem::~FileSystem() = default;

Directory FileSystem::root()
{
	return Directory {*this, {}};
}

uqstl::optional<Mount> FileSystem::mountDirectory(uqstl::string_view mountPoint, const uqstl::filesystem::path& hostPath, Access access, MountPosition position, const Mount* relative, uqstl::error_code& error)
{
	uqstl::error_code statusError {};
	if (!uqstl::filesystem::is_directory(hostPath, statusError))
	{
		error = statusError ? statusError : uqstl::make_error_code(uqstl::errc::not_a_directory);
		return uqstl::nullopt;
	}

	uqstl::vector<MountEntry> entries {};
	entries.push_back({.mountPoint = path::normalize(mountPoint), .source = uqstl::make_shared<DirectorySource>(hostPath), .access = access});
	const uqstl::unique_lock lock {m_mutex};
	return insertLocked(uqstl::move(entries), position, relative, error);
}

uqstl::optional<Mount> FileSystem::mountArchive(uqstl::string_view mountPoint, const Directory& directory, uqstl::string_view archivePath, MountPosition position, const Mount* relative, uqstl::error_code& error)
{
	uqstl::shared_ptr<const MountSource> archive {};
	uqstl::string sourcePath {};
	{
		const uqstl::shared_lock lock {m_mutex};
		const uqstl::optional<Found> found {findLocked(path::join(directory.path(), archivePath))};
		if (!found || found->info.isDirectory())
		{
			error = uqstl::make_error_code(found ? uqstl::errc::is_a_directory : uqstl::errc::no_such_file_or_directory);
			return uqstl::nullopt;
		}
		archive = m_mounts[found->index].source;
		sourcePath = found->sourcePath;
	}

	uqstl::shared_ptr<ZipSource> zip {ZipSource::create(uqstl::move(archive), uqstl::move(sourcePath), error)};
	if (zip == nullptr)
	{
		return uqstl::nullopt;
	}

	uqstl::vector<MountEntry> entries {};
	entries.push_back({.mountPoint = path::normalize(mountPoint), .source = uqstl::move(zip), .access = Access::ReadOnly});
	const uqstl::unique_lock lock {m_mutex};
	return insertLocked(uqstl::move(entries), position, relative, error);
}

uqstl::optional<Mount> FileSystem::mountTransplant(uqstl::string_view mountPoint, const Directory& source, Access access, MountPosition position, const Mount* relative, uqstl::error_code& error)
{
	const uqstl::string target {path::normalize(mountPoint)};
	const uqstl::string& sourceDir {source.path()};

	const uqstl::unique_lock lock {m_mutex};
	uqstl::vector<MountEntry> entries {};
	for (const MountEntry& mount : m_mounts)
	{
		if (path::isWithin(sourceDir, mount.mountPoint))
		{
			entries.push_back({.mountPoint = target, .source = mount.source, .sourcePath = path::join(mount.sourcePath, path::relativeTo(sourceDir, mount.mountPoint)), .access = combine(access, mount.access)});
		}
		else if (path::isWithin(mount.mountPoint, sourceDir))
		{
			entries.push_back({.mountPoint = path::join(target, path::relativeTo(mount.mountPoint, sourceDir)), .source = mount.source, .sourcePath = mount.sourcePath, .access = combine(access, mount.access)});
		}
	}
	if (entries.empty())
	{
		error = uqstl::make_error_code(uqstl::errc::no_such_file_or_directory);
		return uqstl::nullopt;
	}
	return insertLocked(uqstl::move(entries), position, relative, error);
}

uqstl::optional<File> FileSystem::open(const uqstl::string& path, OpenMode mode, uqstl::error_code& error)
{
	const uqstl::shared_lock lock {m_mutex};
	const uqstl::optional<Found> found {findLocked(path)};

	const auto openIn {[&](const MountEntry& mount, const uqstl::string& sourcePath, OpenMode openMode) -> uqstl::optional<File> {
		uqstl::unique_ptr<Stream> stream {mount.source->open(sourcePath, openMode, error)};
		if (stream == nullptr)
		{
			return uqstl::nullopt;
		}
		return File {uqstl::move(stream)};
	}};

	if (found && found->info.isDirectory())
	{
		error = uqstl::make_error_code(uqstl::errc::is_a_directory);
		return uqstl::nullopt;
	}

	if (!hasFlag(mode, OpenMode::Write))
	{
		if (!found)
		{
			error = uqstl::make_error_code(isMountAncestorLocked(path) ? uqstl::errc::is_a_directory : uqstl::errc::no_such_file_or_directory);
			return uqstl::nullopt;
		}
		return openIn(m_mounts[found->index], found->sourcePath, mode);
	}

	if (found)
	{
		if (hasFlag(mode, OpenMode::Create | OpenMode::Exclusive))
		{
			error = uqstl::make_error_code(uqstl::errc::file_exists);
			return uqstl::nullopt;
		}
		const MountEntry& mount {m_mounts[found->index]};
		if (mount.access == Access::ReadWrite && mount.source->canWrite())
		{
			return openIn(mount, found->sourcePath, mode);
		}

		// Copy the file to a writable mount above it.
		const uqstl::optional<size_t> targetIndex {writableMountLocked(path, found->index)};
		if (!targetIndex)
		{
			error = uqstl::make_error_code(uqstl::errc::permission_denied);
			return uqstl::nullopt;
		}
		const MountEntry& target {m_mounts[*targetIndex]};
		const uqstl::string targetPath {path::join(target.sourcePath, path::relativeTo(path, target.mountPoint))};
		error = makeParentsLocked(target, targetPath);
		if (error)
		{
			return uqstl::nullopt;
		}
		if (!hasFlag(mode, OpenMode::Truncate))
		{
			const uqstl::unique_ptr<Stream> from {mount.source->open(found->sourcePath, OpenMode::Read, error)};
			const uqstl::unique_ptr<Stream> to {from == nullptr ? nullptr : target.source->open(targetPath, OpenMode::Write | OpenMode::Create | OpenMode::Truncate, error)};
			if (to == nullptr)
			{
				return uqstl::nullopt;
			}
			error = copyStream(*from, *to);
			if (error)
			{
				return uqstl::nullopt;
			}
		}
		return openIn(target, targetPath, mode | OpenMode::Create);
	}

	if (!hasFlag(mode, OpenMode::Create))
	{
		error = uqstl::make_error_code(uqstl::errc::no_such_file_or_directory);
		return uqstl::nullopt;
	}
	const uqstl::optional<FileInfo> parent {statLocked(uqstl::string {path::parent(path)})};
	if (!parent || !parent->isDirectory())
	{
		error = uqstl::make_error_code(uqstl::errc::no_such_file_or_directory);
		return uqstl::nullopt;
	}
	const uqstl::optional<size_t> targetIndex {writableMountLocked(path, m_mounts.size())};
	if (!targetIndex)
	{
		error = uqstl::make_error_code(uqstl::errc::read_only_file_system);
		return uqstl::nullopt;
	}
	const MountEntry& target {m_mounts[*targetIndex]};
	const uqstl::string targetPath {path::join(target.sourcePath, path::relativeTo(path, target.mountPoint))};
	error = makeParentsLocked(target, targetPath);
	if (error)
	{
		return uqstl::nullopt;
	}
	return openIn(target, targetPath, mode);
}

uqstl::optional<FileInfo> FileSystem::stat(const uqstl::string& path) const
{
	const uqstl::shared_lock lock {m_mutex};
	return statLocked(path);
}

uqstl::vector<uqstl::string> FileSystem::list(const uqstl::string& path, const Matcher& matcher, uqstl::error_code& error) const
{
	const uqstl::shared_lock lock {m_mutex};
	const uqstl::optional<FileInfo> info {statLocked(path)};
	if (!info || !info->isDirectory())
	{
		error = uqstl::make_error_code(info ? uqstl::errc::not_a_directory : uqstl::errc::no_such_file_or_directory);
		return {};
	}

	uqstl::set<uqstl::string, uqstl::less<>> names {};
	for (const MountEntry& mount : m_mounts)
	{
		if (!path::isWithin(path, mount.mountPoint))
		{
			continue;
		}
		const uqstl::string sourcePath {path::join(mount.sourcePath, path::relativeTo(path, mount.mountPoint))};
		const uqstl::optional<FileInfo> sourceInfo {mount.source->stat(sourcePath)};
		if (!sourceInfo)
		{
			continue;
		}
		if (!sourceInfo->isDirectory())
		{
			// A file hides the directories in the mounts below it.
			break;
		}
		uqstl::error_code listError {};
		for (uqstl::string& name : mount.source->list(sourcePath, listError))
		{
			names.insert(uqstl::move(name));
		}
	}
	for (const MountEntry& mount : m_mounts)
	{
		if (mount.mountPoint != path && path::isWithin(mount.mountPoint, path))
		{
			const uqstl::string_view rest {path::relativeTo(mount.mountPoint, path)};
			names.emplace(rest.substr(0, rest.find('/')));
		}
	}

	uqstl::vector<uqstl::string> result {};
	for (const uqstl::string& name : names)
	{
		if (matcher.matches(name))
		{
			result.push_back(name);
		}
	}
	return result;
}

uqstl::error_code FileSystem::makeDir(const uqstl::string& path)
{
	const uqstl::shared_lock lock {m_mutex};
	if (statLocked(path))
	{
		return uqstl::make_error_code(uqstl::errc::file_exists);
	}
	const uqstl::optional<FileInfo> parent {statLocked(uqstl::string {path::parent(path)})};
	if (!parent || !parent->isDirectory())
	{
		return uqstl::make_error_code(uqstl::errc::no_such_file_or_directory);
	}
	const uqstl::optional<size_t> targetIndex {writableMountLocked(path, m_mounts.size())};
	if (!targetIndex)
	{
		return uqstl::make_error_code(uqstl::errc::read_only_file_system);
	}
	const MountEntry& target {m_mounts[*targetIndex]};
	const uqstl::string targetPath {path::join(target.sourcePath, path::relativeTo(path, target.mountPoint))};
	if (const uqstl::error_code error {makeParentsLocked(target, targetPath)})
	{
		return error;
	}
	return target.source->makeDir(targetPath);
}

uqstl::error_code FileSystem::remove(const uqstl::string& path)
{
	const uqstl::shared_lock lock {m_mutex};
	bool removed {false};
	for (const MountEntry& mount : m_mounts)
	{
		if (!path::isWithin(path, mount.mountPoint))
		{
			continue;
		}
		const uqstl::string sourcePath {path::join(mount.sourcePath, path::relativeTo(path, mount.mountPoint))};
		if (!mount.source->stat(sourcePath))
		{
			continue;
		}
		if (mount.access != Access::ReadWrite || !mount.source->canWrite())
		{
			return uqstl::make_error_code(uqstl::errc::permission_denied);
		}
		if (const uqstl::error_code error {mount.source->remove(sourcePath)})
		{
			return error;
		}
		removed = true;
	}
	return removed ? uqstl::error_code {} : uqstl::make_error_code(uqstl::errc::no_such_file_or_directory);
}

uqstl::error_code FileSystem::rename(const uqstl::string& from, const uqstl::string& to)
{
	const uqstl::shared_lock lock {m_mutex};
	const uqstl::optional<Found> found {findLocked(from)};
	if (!found)
	{
		return uqstl::make_error_code(uqstl::errc::no_such_file_or_directory);
	}
	const MountEntry& mount {m_mounts[found->index]};
	if (mount.access != Access::ReadWrite || !mount.source->canWrite())
	{
		return uqstl::make_error_code(uqstl::errc::permission_denied);
	}
	if (!path::isWithin(to, mount.mountPoint))
	{
		return uqstl::make_error_code(uqstl::errc::cross_device_link);
	}
	const uqstl::string targetPath {path::join(mount.sourcePath, path::relativeTo(to, mount.mountPoint))};
	if (const uqstl::error_code error {makeParentsLocked(mount, targetPath)})
	{
		return error;
	}
	return mount.source->rename(found->sourcePath, targetPath);
}

void FileSystem::unmount(uint64_t id)
{
	const uqstl::unique_lock lock {m_mutex};
	uqstl::erase_if(m_mounts, [id](const MountEntry& mount) {
		return mount.id == id;
	});
}

uqstl::optional<FileSystem::Found> FileSystem::findLocked(const uqstl::string& path) const
{
	for (size_t i {0}; i < m_mounts.size(); ++i)
	{
		const MountEntry& mount {m_mounts[i]};
		if (!path::isWithin(path, mount.mountPoint))
		{
			continue;
		}
		uqstl::string sourcePath {path::join(mount.sourcePath, path::relativeTo(path, mount.mountPoint))};
		uqstl::optional<FileInfo> info {mount.source->stat(sourcePath)};
		if (info)
		{
			info->writable = info->writable && mount.access == Access::ReadWrite && mount.source->canWrite();
			return Found {i, uqstl::move(sourcePath), *info};
		}
	}
	return uqstl::nullopt;
}

uqstl::optional<FileInfo> FileSystem::statLocked(const uqstl::string& path) const
{
	if (const uqstl::optional<Found> found {findLocked(path)})
	{
		return found->info;
	}
	if (path.empty() || isMountAncestorLocked(path))
	{
		return FileInfo {.type = FileType::Directory};
	}
	return uqstl::nullopt;
}

bool FileSystem::isMountAncestorLocked(const uqstl::string& path) const
{
	return uqstl::any_of(m_mounts.begin(), m_mounts.end(), [&path](const MountEntry& mount) {
		return path::isWithin(mount.mountPoint, path);
	});
}

uqstl::optional<size_t> FileSystem::writableMountLocked(const uqstl::string& path, size_t below) const
{
	for (size_t i {0}; i < below; ++i)
	{
		const MountEntry& mount {m_mounts[i]};
		if (mount.access == Access::ReadWrite && mount.source->canWrite() && path::isWithin(path, mount.mountPoint))
		{
			return i;
		}
	}
	return uqstl::nullopt;
}

uqstl::error_code FileSystem::makeParentsLocked(const MountEntry& mount, uqstl::string_view sourcePath) const
{
	const uqstl::string_view parent {path::parent(sourcePath)};
	size_t end {0};
	while (end < parent.size())
	{
		end = uqstl::min(parent.find('/', end + 1), parent.size());
		const uqstl::string_view part {parent.substr(0, end)};
		const uqstl::optional<FileInfo> info {mount.source->stat(part)};
		if (!info)
		{
			if (const uqstl::error_code error {mount.source->makeDir(part)})
			{
				return error;
			}
		}
		else if (!info->isDirectory())
		{
			return uqstl::make_error_code(uqstl::errc::not_a_directory);
		}
	}
	return {};
}

uqstl::optional<Mount> FileSystem::insertLocked(uqstl::vector<MountEntry> entries, MountPosition position, const Mount* relative, uqstl::error_code& error)
{
	size_t index {0};
	switch (position)
	{
		case MountPosition::Top:
			break;
		case MountPosition::Bottom:
			index = m_mounts.size();
			break;
		case MountPosition::Above:
		case MountPosition::Below:
			{
				if (relative == nullptr || relative->m_fileSystem != this)
				{
					error = uqstl::make_error_code(uqstl::errc::invalid_argument);
					return uqstl::nullopt;
				}
				const uint64_t relativeId {relative->m_id};
				const auto matches {[relativeId](const MountEntry& mount) {
					return mount.id == relativeId;
				}};
				const auto first {uqstl::find_if(m_mounts.begin(), m_mounts.end(), matches)};
				if (first == m_mounts.end())
				{
					error = uqstl::make_error_code(uqstl::errc::invalid_argument);
					return uqstl::nullopt;
				}
				const auto last {uqstl::find_if(m_mounts.rbegin(), m_mounts.rend(), matches)};
				index = position == MountPosition::Above ? static_cast<size_t>(first - m_mounts.begin()) : static_cast<size_t>(m_mounts.rend() - last);
				break;
			}
	}

	const uint64_t id {m_nextId++};
	for (MountEntry& entry : entries)
	{
		entry.id = id;
	}
	m_mounts.insert(m_mounts.begin() + static_cast<ptrdiff_t>(index), uqstl::make_move_iterator(entries.begin()), uqstl::make_move_iterator(entries.end()));
	return Mount {*this, id};
}

} // namespace uqm::io::standard
