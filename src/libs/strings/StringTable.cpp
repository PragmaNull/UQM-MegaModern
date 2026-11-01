// A table of strings from a resource: text entries, and optionally a name, a sound clip and a timestamp for each.
#include "StringTable.h"

namespace uqm
{

StringTable::StringTable(size_t count, int flags)
	: m_flags {flags}
	, m_size {count}
{
	size_t blocks {1};
	for (const int flag : {c_hasNames, c_hasSoundClips, c_hasTimestamps})
	{
		blocks += (flags & flag) != 0 ? 1 : 0;
	}
	m_entries.resize(count * blocks);
}

const char* StringTable::text(size_t index) const
{
	return blockText(index, 0);
}

uqstl::string_view StringTable::bytes(size_t index) const
{
	return index < m_size ? uqstl::string_view {m_entries[index]} : uqstl::string_view {};
}

const char* StringTable::name(size_t index) const
{
	return (m_flags & c_hasNames) ? blockText(index, 1) : nullptr;
}

const char* StringTable::soundClip(size_t index) const
{
	return (m_flags & c_hasSoundClips) ? blockText(index, (m_flags & c_hasNames) ? 2 : 1) : nullptr;
}

const char* StringTable::timestamp(size_t index) const
{
	if (!(m_flags & c_hasTimestamps))
	{
		return nullptr;
	}
	return blockText(index, 1 + ((m_flags & c_hasNames) ? 1 : 0) + ((m_flags & c_hasSoundClips) ? 1 : 0));
}

uqstl::optional<size_t> StringTable::find(uqstl::string_view name) const
{
	const auto it {m_names.find(name)};
	return it != m_names.end() ? uqstl::optional<size_t> {it->second} : uqstl::nullopt;
}

void StringTable::set(size_t index, uqstl::string_view bytes)
{
	m_entries[index].assign(bytes);
}

void StringTable::indexName(size_t index)
{
	if (const char* const entryName {name(index)})
	{
		m_names.emplace(entryName, index);
	}
}

void StringTable::swap(size_t index, StringTable& other, size_t otherIndex)
{
	m_entries[index].swap(other.m_entries[otherIndex]);
}

const char* StringTable::blockText(size_t index, int block) const
{
	if (index >= m_size)
	{
		return nullptr;
	}
	const uqstl::string& entryBytes {m_entries[index + block * m_size]};
	return entryBytes.empty() ? nullptr : entryBytes.c_str();
}

} // namespace uqm
