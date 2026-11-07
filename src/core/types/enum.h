#pragma once
#ifndef UQM_CORE_TYPES_ENUM_H
#define UQM_CORE_TYPES_ENUM_H

#include "core/stl/concepts.h"
#include <compare>
#include <cstdint>
#include <utility>
#include "core/stl/stl.h"

#ifdef WITH_MAGIC_ENUM
#include <magic_enum/magic_enum.hpp>
#endif

template <typename E>
concept EnumType = std::is_enum_v<E>;

// An enum whose values are also used as numbers (indexes, offsets, counts).
// Declare 'consteval bool isArithmeticEnum(E) { return true; }' in the namespace of the enum.
template <typename E>
concept ArithmeticEnum = EnumType<E> && requires { requires isArithmeticEnum(E {}); };

// An enum whose values are bit flags.
// Declare 'consteval bool isFlagEnum(E) { return true; }' in the namespace of the enum.
template <typename E>
concept BitFlagEnum = EnumType<E> && requires { requires isFlagEnum(E {}); };

// The value of an arithmetic or flag enum as an integer, for example as an array index: array[*Race::Arilou].
template <EnumType E>
	requires(ArithmeticEnum<E> || BitFlagEnum<E>)
[[nodiscard]] constexpr auto operator*(E e) noexcept
{
	return static_cast<std::underlying_type_t<E>>(e);
}

template <ArithmeticEnum E>
[[nodiscard]] constexpr auto operator-(E e) noexcept
{
	return -static_cast<std::underlying_type_t<E>>(e);
}

template <ArithmeticEnum E, std::integral I>
[[nodiscard]] constexpr E operator+(E e, I n) noexcept
{
	return static_cast<E>(*e + n);
}

template <ArithmeticEnum E, std::integral I>
[[nodiscard]] constexpr E operator+(I n, E e) noexcept
{
	return static_cast<E>(n + *e);
}

template <ArithmeticEnum E, std::integral I>
[[nodiscard]] constexpr E operator-(E e, I n) noexcept
{
	return static_cast<E>(*e - n);
}

template <ArithmeticEnum E>
[[nodiscard]] constexpr int operator-(E a, E b) noexcept
{
	return *a - *b;
}

template <ArithmeticEnum E, std::integral I>
constexpr E& operator+=(E& e, I n) noexcept
{
	return e = e + n;
}

template <ArithmeticEnum E, std::integral I>
constexpr E& operator-=(E& e, I n) noexcept
{
	return e = e - n;
}

template <ArithmeticEnum E>
constexpr E& operator++(E& e) noexcept
{
	return e = e + 1;
}

template <ArithmeticEnum E>
constexpr E operator++(E& e, int) noexcept
{
	const E old {e};
	++e;
	return old;
}

template <ArithmeticEnum E>
constexpr E& operator--(E& e) noexcept
{
	return e = e - 1;
}

template <ArithmeticEnum E>
constexpr E operator--(E& e, int) noexcept
{
	const E old {e};
	--e;
	return old;
}

template <ArithmeticEnum E, std::integral I>
[[nodiscard]] constexpr bool operator==(E e, I n) noexcept
{
	return uqstl::cmp_equal(*e, n);
}

template <ArithmeticEnum E, std::integral I>
[[nodiscard]] constexpr uqstl::strong_ordering operator<=>(E e, I n) noexcept
{
	if (uqstl::cmp_less(*e, n))
	{
		return uqstl::strong_ordering::less;
	}
	if (uqstl::cmp_greater(*e, n))
	{
		return uqstl::strong_ordering::greater;
	}
	return uqstl::strong_ordering::equal;
}

template <BitFlagEnum E>
[[nodiscard]] constexpr E operator|(E a, E b) noexcept
{
	return static_cast<E>(static_cast<std::underlying_type_t<E>>(a) | static_cast<std::underlying_type_t<E>>(b));
}

template <BitFlagEnum E>
[[nodiscard]] constexpr E operator&(E a, E b) noexcept
{
	return static_cast<E>(static_cast<std::underlying_type_t<E>>(a) & static_cast<std::underlying_type_t<E>>(b));
}

template <BitFlagEnum E>
[[nodiscard]] constexpr E operator^(E a, E b) noexcept
{
	return static_cast<E>(static_cast<std::underlying_type_t<E>>(a) ^ static_cast<std::underlying_type_t<E>>(b));
}

template <BitFlagEnum E>
[[nodiscard]] constexpr E operator~(E a) noexcept
{
	return static_cast<E>(~static_cast<std::underlying_type_t<E>>(a));
}

template <BitFlagEnum E>
constexpr E& operator|=(E& a, E b) noexcept
{
	return a = a | b;
}

template <BitFlagEnum E>
constexpr E& operator&=(E& a, E b) noexcept
{
	return a = a & b;
}

template <BitFlagEnum E>
constexpr E& operator^=(E& a, E b) noexcept
{
	return a = a ^ b;
}

// Bit operations on an integer that holds the bits of a flag enum.
template <BitFlagEnum E, std::integral I>
[[nodiscard]] constexpr I operator&(I a, E b) noexcept
{
	return static_cast<I>(a & static_cast<std::underlying_type_t<E>>(b));
}

template <BitFlagEnum E, std::integral I>
[[nodiscard]] constexpr I operator|(I a, E b) noexcept
{
	return static_cast<I>(a | static_cast<std::underlying_type_t<E>>(b));
}

template <BitFlagEnum E, std::integral I>
[[nodiscard]] constexpr bool operator==(I a, E b) noexcept
{
	return uqstl::cmp_equal(a, static_cast<std::underlying_type_t<E>>(b));
}

template <BitFlagEnum E, std::integral I>
constexpr I& operator&=(I& a, E b) noexcept
{
	return a = a & b;
}

template <BitFlagEnum E, std::integral I>
constexpr I& operator|=(I& a, E b) noexcept
{
	return a = a | b;
}

template <BitFlagEnum E, std::integral I>
constexpr I& operator^=(I& a, E b) noexcept
{
	return a = static_cast<I>(a ^ static_cast<std::underlying_type_t<E>>(b));
}

// fmt formats arithmetic and flag enums as their numbers.
template <EnumType E>
	requires(ArithmeticEnum<E> || BitFlagEnum<E>)
[[nodiscard]] constexpr auto format_as(E e) noexcept
{
	return static_cast<std::underlying_type_t<E>>(e);
}

// True if any of the bits of 'b' are set in 'a'.
template <BitFlagEnum E>
[[nodiscard]] constexpr bool any(E a, E b) noexcept
{
	return static_cast<std::underlying_type_t<E>>(a & b) != 0;
}

// The enum operators for the enums of the game code.
namespace uqm
{
using ::any;
using ::format_as;
using ::operator*;
using ::operator-;
using ::operator+;
using ::operator++;
using ::operator--;
using ::operator+=;
using ::operator-=;
using ::operator==;
using ::operator<=>;
using ::operator|;
using ::operator&;
using ::operator^;
using ::operator~;
using ::operator|=;
using ::operator&=;
using ::operator^=;
} // namespace uqm

#define ENUM_BITWISE_OPS(enumName)                                                                                                             \
	[[nodiscard]] constexpr inline enumName operator|(enumName lhs, enumName rhs) noexcept                                                     \
	{                                                                                                                                          \
		return static_cast<enumName>(static_cast<std::underlying_type_t<enumName>>(lhs) | static_cast<std::underlying_type_t<enumName>>(rhs)); \
	}                                                                                                                                          \
	[[nodiscard]] constexpr inline enumName operator&(enumName lhs, enumName rhs) noexcept                                                     \
	{                                                                                                                                          \
		return static_cast<enumName>(static_cast<std::underlying_type_t<enumName>>(lhs) & static_cast<std::underlying_type_t<enumName>>(rhs)); \
	}                                                                                                                                          \
	[[nodiscard]] constexpr inline enumName operator^(enumName lhs, enumName rhs) noexcept                                                     \
	{                                                                                                                                          \
		return static_cast<enumName>(static_cast<std::underlying_type_t<enumName>>(lhs) ^ static_cast<std::underlying_type_t<enumName>>(rhs)); \
	}                                                                                                                                          \
	[[nodiscard]] constexpr inline enumName operator~(enumName rhs) noexcept                                                                   \
	{                                                                                                                                          \
		return static_cast<enumName>(~static_cast<std::underlying_type_t<enumName>>(rhs));                                                     \
	}                                                                                                                                          \
	constexpr inline enumName& operator|=(enumName& lhs, enumName rhs) noexcept                                                                \
	{                                                                                                                                          \
		lhs = lhs | rhs;                                                                                                                       \
		return lhs;                                                                                                                            \
	}                                                                                                                                          \
	constexpr inline enumName& operator&=(enumName& lhs, enumName rhs) noexcept                                                                \
	{                                                                                                                                          \
		lhs = lhs & rhs;                                                                                                                       \
		return lhs;                                                                                                                            \
	}                                                                                                                                          \
	constexpr inline enumName& operator^=(enumName& lhs, enumName rhs) noexcept                                                                \
	{                                                                                                                                          \
		lhs = lhs ^ rhs;                                                                                                                       \
		return lhs;                                                                                                                            \
	}                                                                                                                                          \
	[[nodiscard]] constexpr inline bool testBit(enumName lhs, enumName rhs) noexcept                                                           \
	{                                                                                                                                          \
		return static_cast<int>(lhs & rhs) != 0;                                                                                               \
	}                                                                                                                                          \
	[[nodiscard]] constexpr inline bool testAllBits(enumName lhs, enumName rhs) noexcept                                                       \
	{                                                                                                                                          \
		return (lhs & rhs) == rhs;                                                                                                             \
	}                                                                                                                                          \
	[[nodiscard]] constexpr inline bool testFlag(enumName lhs, enumName rhs) noexcept { return testBit(lhs, rhs); }                            \
	[[nodiscard]] constexpr inline bool testAnyFlag(enumName lhs, enumName rhs) noexcept { return testBit(lhs, rhs); }                         \
	[[nodiscard]] constexpr inline bool testExactFlags(enumName lhs, enumName rhs) noexcept { return testAllBits(lhs, rhs); }

template <typename E>
concept FlagEnum = EnumType<E> && requires(E a) {
	{ a | a } -> std::same_as<E>;
};

#ifdef WITH_MAGIC_ENUM
// Returns the OR of all declared values of 'E'.
template <EnumType E>
constexpr int64_t declaredBits()
{
	int64_t bits {};
	for (const E value : magic_enum::enum_values<E>())
	{
		bits |= static_cast<int64_t>(value);
	}
	return bits;
}
#endif

#endif // UQM_CORE_TYPES_ENUM_H
