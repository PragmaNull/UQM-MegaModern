// The mount stack of a file system, shared by its directories.
#include "libs/io/standard/MountTable.h"

#include "libs/io/Path.h"
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

uqstl::string MountTable::MountEntry::sourcePathFor(uqstl::string_view path) const
{
	const uqstl::string_view relative {path::relativeTo(path, mountPoint)};
	return sourcePath.empty() ? uqstl::string {relative} : path::join(sourcePath, relative);
}

Directory MountTable::root()
{
	return Directory {shared_from_this(), {}};
}

uqstl::optional<Mount> MountTable::mountSource(uqstl::string_view mountPoint, uqstl::shared_ptr<const MountSource> source, Access access, MountPosition position, const Mount* relative, uqstl::error_code& error)
{
	uqstl::vector<MountEntry> entries {};
	entries.push_back({.mountPoint = path::normalize(mountPoint), .source = uqstl::move(source), .access = access});
	const uqstl::unique_lock lock {m_mutex};
	return insertLocked(uqstl::move(entries), position, relative, error);
}

uqstl::optional<Mount> MountTable::mountArchive(uqstl::string_view mountPoint, const Directory& directory, uqstl::string_view archivePath, MountPosition position, const Mount* relative, uqstl::error_code& error)
{
	if (directory.m_table.get() != this)
	{
		return fail<uqstl::optional<Mount>>(error, uqstl::errc::invalid_argument);
	}
	uqstl::shared_ptr<const MountSource> archive {};
	uqstl::string sourcePath {};
	{
		const uqstl::shared_lock lock {m_mutex};
		const uqstl::optional<Found> found {findLocked(path::join(directory.path(), archivePath))};
		if (!found || found->info.isDirectory())
		{
			return fail<uqstl::optional<Mount>>(error, found ? uqstl::errc::is_a_directory : uqstl::errc::no_such_file_or_directory);
		}
		archive = m_mounts[found->index].source;
		sourcePath = found->sourcePath;
	}

	uqstl::shared_ptr<ZipSource> zip {ZipSource::create(uqstl::move(archive), uqstl::move(sourcePath), error)};
	if (zip == nullptr)
	{
		return uqstl::nullopt;
	}

	return mountSource(mountPoint, uqstl::move(zip), Access::ReadOnly, position, relative, error);
}

uqstl::optional<Mount> MountTable::mountTransplant(uqstl::string_view mountPoint, const Directory& source, Access access, MountPosition position, const Mount* relative, uqstl::error_code& error)
{
	if (source.m_table.get() != this)
	{
		return fail<uqstl::optional<Mount>>(error, uqstl::errc::invalid_argument);
	}
	const uqstl::string target {path::normalize(mountPoint)};
	const uqstl::string& sourceDir {source.path()};

	const uqstl::unique_lock lock {m_mutex};
	uqstl::vector<MountEntry> entries {};
	for (const MountEntry& mount : m_mounts)
	{
		if (path::isWithin(sourceDir, mount.mountPoint))
		{
			entries.push_back({.mountPoint = target, .source = mount.source, .sourcePath = mount.sourcePathFor(sourceDir), .access = combine(access, mount.access)});
		}
		else if (path::isWithin(mount.mountPoint, sourceDir))
		{
			entries.push_back({.mountPoint = path::join(target, path::relativeTo(mount.mountPoint, sourceDir)), .source = mount.source, .sourcePath = mount.sourcePath, .access = combine(access, mount.access)});
		}
	}
	if (entries.empty())
	{
		return fail<uqstl::optional<Mount>>(error, uqstl::errc::no_such_file_or_directory);
	}
	return insertLocked(uqstl::move(entries), position, relative, error);
}

uqstl::optional<File> MountTable::open(const uqstl::string& path, OpenMode mode, uqstl::error_code& error) const
{
	const uqstl::shared_lock lock {m_mutex};
	if (!testAllBits(mode, OpenMode::Write))
	{
		for (const MountEntry& mount : m_mounts)
		{
			if (!path::isWithin(path, mount.mountPoint))
			{
				continue;
			}
			uqstl::error_code openError {};
			if (uqstl::unique_ptr<Stream> stream {mount.source->open(mount.sourcePathFor(path), mode, openError)})
			{
				return File {uqstl::move(stream)};
			}
			if (openError != uqstl::errc::no_such_file_or_directory)
			{
				error = openError;
				return uqstl::nullopt;
			}
		}
		return fail<uqstl::optional<File>>(error, isMountAncestorLocked(path) ? uqstl::errc::is_a_directory : uqstl::errc::no_such_file_or_directory);
	}

	const uqstl::optional<Found> found {findLocked(path)};

	const auto openIn {[&](const MountEntry& mount, const uqstl::string& sourcePath, OpenMode openMode) -> uqstl::optional<File> {
		uqstl::unique_ptr<Stream> stream {mount.source->open(sourcePath, openMode, error)};
		if (stream == nullptr)
		{
			return uqstl::nullopt;
		}
		return File {uqstl::move(stream)};
	}};

	using Result = uqstl::optional<File>;
	if (found && found->info.isDirectory())
	{
		return fail<Result>(error, uqstl::errc::is_a_directory);
	}

	if (found)
	{
		if (testAllBits(mode, OpenMode::Create | OpenMode::Exclusive))
		{
			return fail<Result>(error, uqstl::errc::file_exists);
		}
		const MountEntry& mount {m_mounts[found->index]};
		if (mount.isWritable())
		{
			return openIn(mount, found->sourcePath, mode);
		}

		// Copy the file to a writable mount above it.
		const uqstl::optional<WriteTarget> target {writeTargetLocked(path, found->index, uqstl::errc::permission_denied, error)};
		if (!target)
		{
			return uqstl::nullopt;
		}
		if (!testAllBits(mode, OpenMode::Truncate))
		{
			const uqstl::unique_ptr<Stream> from {mount.source->open(found->sourcePath, OpenMode::Read, error)};
			const uqstl::unique_ptr<Stream> to {from == nullptr ? nullptr : target->mount->source->open(target->sourcePath, OpenMode::Write | OpenMode::Create | OpenMode::Truncate, error)};
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
		return openIn(*target->mount, target->sourcePath, mode | OpenMode::Create);
	}

	if (!testAllBits(mode, OpenMode::Create))
	{
		return fail<Result>(error, uqstl::errc::no_such_file_or_directory);
	}
	const uqstl::optional<FileInfo> parent {statLocked(uqstl::string {path::parent(path)})};
	if (!parent || !parent->isDirectory())
	{
		return fail<Result>(error, uqstl::errc::no_such_file_or_directory);
	}
	const uqstl::optional<WriteTarget> target {writeTargetLocked(path, m_mounts.size(), uqstl::errc::read_only_file_system, error)};
	if (!target)
	{
		return uqstl::nullopt;
	}
	return openIn(*target->mount, target->sourcePath, mode);
}

uqstl::optional<FileInfo> MountTable::stat(const uqstl::string& path) const
{
	const uqstl::shared_lock lock {m_mutex};
	return statLocked(path);
}

uqstl::vector<uqstl::string> MountTable::list(const uqstl::string& path, const Matcher& matcher, uqstl::error_code& error) const
{
	const uqstl::shared_lock lock {m_mutex};
	const uqstl::optional<FileInfo> info {statLocked(path)};
	if (!info || !info->isDirectory())
	{
		return fail<uqstl::vector<uqstl::string>>(error, info ? uqstl::errc::not_a_directory : uqstl::errc::no_such_file_or_directory);
	}

	uqstl::set<uqstl::string, uqstl::less<>> names {};
	for (const MountEntry& mount : m_mounts)
	{
		if (!path::isWithin(path, mount.mountPoint))
		{
			continue;
		}
		const uqstl::string sourcePath {mount.sourcePathFor(path)};
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

uqstl::error_code MountTable::makeDir(const uqstl::string& path) const
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
	uqstl::error_code error {};
	const uqstl::optional<WriteTarget> target {writeTargetLocked(path, m_mounts.size(), uqstl::errc::read_only_file_system, error)};
	if (!target)
	{
		return error;
	}
	return target->mount->source->makeDir(target->sourcePath);
}

uqstl::error_code MountTable::remove(const uqstl::string& path) const
{
	const uqstl::shared_lock lock {m_mutex};
	uqstl::vector<uqstl::pair<const MountEntry*, uqstl::string>> copies {};
	for (const MountEntry& mount : m_mounts)
	{
		if (!path::isWithin(path, mount.mountPoint))
		{
			continue;
		}
		uqstl::string sourcePath {mount.sourcePathFor(path)};
		if (!mount.source->stat(sourcePath))
		{
			continue;
		}
		if (!mount.isWritable())
		{
			return uqstl::make_error_code(uqstl::errc::permission_denied);
		}
		copies.emplace_back(&mount, uqstl::move(sourcePath));
	}
	if (copies.empty())
	{
		return uqstl::make_error_code(uqstl::errc::no_such_file_or_directory);
	}
	for (const auto& [mount, sourcePath] : copies)
	{
		if (const uqstl::error_code error {mount->source->remove(sourcePath)})
		{
			return error;
		}
	}
	return {};
}

uqstl::error_code MountTable::rename(const uqstl::string& from, const uqstl::string& to) const
{
	const uqstl::shared_lock lock {m_mutex};
	const uqstl::optional<Found> found {findLocked(from)};
	if (!found)
	{
		return uqstl::make_error_code(uqstl::errc::no_such_file_or_directory);
	}
	const MountEntry& mount {m_mounts[found->index]};
	if (!mount.isWritable())
	{
		return uqstl::make_error_code(uqstl::errc::permission_denied);
	}
	if (!path::isWithin(to, mount.mountPoint))
	{
		return uqstl::make_error_code(uqstl::errc::cross_device_link);
	}
	const uqstl::string targetPath {mount.sourcePathFor(to)};
	if (const uqstl::error_code error {makeParentsLocked(mount, targetPath)})
	{
		return error;
	}
	return mount.source->rename(found->sourcePath, targetPath);
}

void MountTable::unmount(uint64_t id)
{
	const uqstl::unique_lock lock {m_mutex};
	uqstl::erase_if(m_mounts, [id](const MountEntry& mount) {
		return mount.id == id;
	});
}

uqstl::optional<MountTable::Found> MountTable::findLocked(const uqstl::string& path) const
{
	for (size_t i {0}; i < m_mounts.size(); ++i)
	{
		const MountEntry& mount {m_mounts[i]};
		if (!path::isWithin(path, mount.mountPoint))
		{
			continue;
		}
		uqstl::string sourcePath {mount.sourcePathFor(path)};
		uqstl::optional<FileInfo> info {mount.source->stat(sourcePath)};
		if (info)
		{
			info->writable = info->writable && mount.isWritable();
			return Found {i, uqstl::move(sourcePath), *info};
		}
	}
	return uqstl::nullopt;
}

uqstl::optional<FileInfo> MountTable::statLocked(const uqstl::string& path) const
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

bool MountTable::isMountAncestorLocked(const uqstl::string& path) const
{
	return uqstl::any_of(m_mounts.begin(), m_mounts.end(), [&path](const MountEntry& mount) {
		return path::isWithin(mount.mountPoint, path);
	});
}

uqstl::optional<size_t> MountTable::writableMountLocked(const uqstl::string& path, size_t below) const
{
	for (size_t i {0}; i < below; ++i)
	{
		const MountEntry& mount {m_mounts[i]};
		if (mount.isWritable() && path::isWithin(path, mount.mountPoint))
		{
			return i;
		}
	}
	return uqstl::nullopt;
}

uqstl::error_code MountTable::makeParentsLocked(const MountEntry& mount, uqstl::string_view sourcePath) const
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

uqstl::optional<MountTable::WriteTarget> MountTable::writeTargetLocked(const uqstl::string& path, size_t below, uqstl::errc noMount, uqstl::error_code& error) const
{
	const uqstl::optional<size_t> index {writableMountLocked(path, below)};
	if (!index)
	{
		return fail<uqstl::optional<WriteTarget>>(error, noMount);
	}
	WriteTarget target {.mount = &m_mounts[*index], .sourcePath = m_mounts[*index].sourcePathFor(path)};
	error = makeParentsLocked(*target.mount, target.sourcePath);
	if (error)
	{
		return uqstl::nullopt;
	}
	return target;
}

uqstl::optional<Mount> MountTable::insertLocked(uqstl::vector<MountEntry> entries, MountPosition position, const Mount* relative, uqstl::error_code& error)
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
				if (relative == nullptr || relative->m_table.lock().get() != this)
				{
					return fail<uqstl::optional<Mount>>(error, uqstl::errc::invalid_argument);
				}
				const uint64_t relativeId {relative->m_id};
				const auto matches {[relativeId](const MountEntry& mount) {
					return mount.id == relativeId;
				}};
				const auto first {uqstl::find_if(m_mounts.begin(), m_mounts.end(), matches)};
				if (first == m_mounts.end())
				{
					return fail<uqstl::optional<Mount>>(error, uqstl::errc::invalid_argument);
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
		if (!entry.source->canWrite())
		{
			entry.access = Access::ReadOnly;
		}
	}
	m_mounts.insert(m_mounts.begin() + static_cast<ptrdiff_t>(index), uqstl::make_move_iterator(entries.begin()), uqstl::make_move_iterator(entries.end()));
	return Mount {shared_from_this(), id};
}

} // namespace uqm::io::standard
