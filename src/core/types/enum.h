#pragma once
#include <concepts>
#include <cstdint>
#include <type_traits>

#ifdef WITH_MAGIC_ENUM
#include <magic_enum/magic_enum.hpp>
#endif

template <typename E>
concept EnumType = std::is_enum_v<E>;

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
