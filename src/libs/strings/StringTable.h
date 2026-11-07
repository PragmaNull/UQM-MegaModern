// A table of strings from a resource: text entries, and optionally a name, a sound clip and a timestamp for each.

#pragma once
#ifndef UQM_LIBS_STRINGS_STRINGTABLE_H
#define UQM_LIBS_STRINGS_STRINGTABLE_H

#include <cstdint>

#include "core/stl/map.h"
#include "core/stl/memory.h"
#include "core/stl/optional.h"
#include "core/stl/stl.h"
#include "core/stl/string.h"
#include "core/stl/vector.h"

namespace uqm
{

class StringTable
{
public:
	static constexpr int c_hasSoundClips {1 << 0};
	static constexpr int c_hasTimestamps {1 << 1};
	static constexpr int c_hasNames {1 << 2};

	// A table of 'count' text entries, and 'count' entries for each of the names, sound clips and timestamps that
	// 'flags' gives.
	StringTable(size_t count, int flags);
	StringTable(const StringTable&) = delete;
	StringTable& operator=(const StringTable&) = delete;

	// The number of text entries.
	[[nodiscard]] size_t size() const noexcept
	{
		return m_size;
	}
	[[nodiscard]] int flags() const noexcept
	{
		return m_flags;
	}

	// The text of an entry. Returns nullptr if the entry is empty or 'index' is not in the table.
	[[nodiscard]] const char* text(size_t index) const;
	// The bytes of an entry, for tables of binary data.
	[[nodiscard]] uqstl::string_view bytes(size_t index) const;
	// Return nullptr if the table or the entry has no value.
	[[nodiscard]] const char* name(size_t index) const;
	[[nodiscard]] const char* soundClip(size_t index) const;
	[[nodiscard]] const char* timestamp(size_t index) const;
	// Returns the index of the entry with the name 'name'.
	[[nodiscard]] uqstl::optional<size_t> find(uqstl::string_view name) const;

	// Sets the bytes of entry 'index' in all the entries: the text entries first, then the names, sound clips and
	// timestamps.
	void set(size_t index, uqstl::string_view bytes);
	// Makes find() give 'index' for the name that entry 'index' has.
	void indexName(size_t index);
	// Swaps the bytes of text entry 'index' and text entry 'otherIndex' of 'other'.
	void swap(size_t index, StringTable& other, size_t otherIndex);

private:
	// The value of text entry 'index' in the block of entries 'block', or nullptr.
	[[nodiscard]] const char* blockText(size_t index, int block) const;

	int m_flags {};
	size_t m_size {};
	// The bytes of the entries. A text entry ends with its NUL, and internal NULs are allowed. An empty entry has no
	// bytes.
	uqstl::vector<uqstl::string> m_entries {};
	struct NameHash
	{
		using is_transparent = void;
		size_t operator()(uqstl::string_view name) const noexcept
		{
			return uqstl::hash<uqstl::string_view> {}(name);
		}
	};

	uqstl::unordered_map<uqstl::string, size_t, NameHash, uqstl::equal_to<>> m_names {};
};

using StringTablePtr = uqstl::unique_ptr<StringTable>;

} // namespace uqm

#endif // UQM_LIBS_STRINGS_STRINGTABLE_H
