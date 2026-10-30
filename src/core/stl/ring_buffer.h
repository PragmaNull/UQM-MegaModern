#pragma once

#ifndef UQM_CORE_STL_RINGBUFFER_H_
#define UQM_CORE_STL_RINGBUFFER_H_

#ifdef USING_EASTL
#include <EASTL/ring_buffer.h>
#else

#include "core/stl/array.h"
#include <cstddef>
#include "core/stl/iterator.h"
#include "core/stl/stdexcept.h"
#include "core/stl/stl.h"

#include "core/misc/bittricks.h"

namespace std
{

template <typename T, size_t N>
class ring_buffer
{
	static_assert(N > 0, "ring_buffer capacity must be greater than zero.");
	static constexpr size_t Pow2SizeMask {N - 1};

	[[nodiscard]] static constexpr size_t wrapIndex(const size_t n) noexcept
	{
		if constexpr (uqm::isPowerOfTwo(N))
		{
			return n & Pow2SizeMask;
		}
		else
		{
			return n % N;
		}
	}

public:
	using value_type = T;
	using size_type = size_t;
	using reference = value_type&;
	using const_reference = const value_type&;

	ring_buffer() = default;

	[[nodiscard]] constexpr size_type capacity() const noexcept { return N; }
	[[nodiscard]] constexpr size_type size() const noexcept { return m_size; }
	[[nodiscard]] constexpr bool empty() const noexcept { return m_size == 0; }
	[[nodiscard]] constexpr bool full() const noexcept { return m_size == N; }

	// Overwrites oldest element if buffer is full
	void push_back(const T& value)
	{
		at(m_tail) = value;
		advance_tail();
	}

	void push_back(T&& value)
	{
		at(m_tail) = ::std::move(value);
		advance_tail();
	}

	template <typename... Args>
	void emplace_back(Args&&... args)
	{
		// if full, destroy existing value before assigning a new one
		if (full() && !is_trivially_destructible<T>::value)
		{
			at(m_tail).~T();
		}

		new (&at(m_tail)) T {::std::forward<Args>(args)...};
		advance_tail();
	}

	void pop_front()
	{
		if (empty())
		{
			throw out_of_range("ring_buffer::pop_front on empty buffer");
		}

		++m_head;
		--m_size;
	}

	[[nodiscard]] reference front()
	{
		require_not_empty();
		return at(m_head);
	}

	[[nodiscard]] const_reference front() const
	{
		require_not_empty();
		return at(m_head);
	}

	[[nodiscard]] reference back()
	{
		require_not_empty();
		return at(m_tail - 1);
	}

	[[nodiscard]] const_reference back() const
	{
		require_not_empty();
		return at(m_tail - 1);
	}

	void clear() noexcept
	{
		m_head = 0;
		m_tail = 0;
		m_size = 0;
	}

	// ==========================
	// Forward Iterator
	// ==========================

	template <bool IsConst>
	class Iterator
	{
		using BufferType = conditional_t<IsConst, const ring_buffer, ring_buffer>;

	public:
		using iterator_category = forward_iterator_tag;
		using value_type = T;
		using difference_type = ptrdiff_t;
		using reference = conditional_t<IsConst, const T&, T&>;
		using pointer = conditional_t<IsConst, const T*, T*>;

		Iterator(BufferType& buffer, size_type index) noexcept
			: m_buffer {buffer}
			, m_index {index}
		{
		}

		reference operator*() const
		{
			return m_buffer.at(m_index);
		}

		pointer operator->() const
		{
			return addressof(operator*());
		}

		Iterator& operator++() noexcept
		{
			++m_index;
			return *this;
		}

		Iterator operator++(int) noexcept
		{
			Iterator tmp = *this;
			++(*this);
			return tmp;
		}

		friend bool operator==(const Iterator& a, const Iterator& b) noexcept
		{
			return &a.m_buffer == &b.m_buffer && a.m_index == b.m_index;
		}

		friend bool operator!=(const Iterator& a, const Iterator& b) noexcept
		{
			return !(a == b);
		}

	private:
		BufferType& m_buffer;
		size_type m_index;
	};

	using iterator = Iterator<false>;
	using const_iterator = Iterator<true>;

	[[nodiscard]] iterator begin() noexcept { return iterator {*this, m_head}; }
	[[nodiscard]] iterator end() noexcept { return iterator {*this, m_tail}; }

	[[nodiscard]] const_iterator begin() const noexcept { return cbegin(); }
	[[nodiscard]] const_iterator end() const noexcept { return cend(); }

	[[nodiscard]] const_iterator cbegin() const noexcept
	{
		return const_iterator {*this, m_head};
	}

	[[nodiscard]] const_iterator cend() const noexcept
	{
		return const_iterator {*this, m_tail};
	}

protected:
	void advance_tail() noexcept
	{
		++m_tail; // = increment(m_tail);

		if (full())
		{
			++m_head; // overwrite oldest
		}
		else
		{
			++m_size;
		}
	}

	void require_not_empty() const
	{
		if (empty())
		{
			throw out_of_range("ring_buffer access on empty buffer");
		}
	}

	reference at(size_type logical_index)
	{
		return m_buffer[wrapIndex(logical_index)];
	}

	const_reference at(size_type logical_index) const
	{
		return m_buffer[wrapIndex(logical_index)];
	}

private:
	array<T, N> m_buffer {};
	size_type m_head {0}; // index of oldest element
	size_type m_tail {0}; // index of next insertion
	size_type m_size {0};
};

} // namespace std

#endif /* !defined(USING_EASTL) */
#endif /* UQM_CORE_STL_RINGBUFFER_H_ */
