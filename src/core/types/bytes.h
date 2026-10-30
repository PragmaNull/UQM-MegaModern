// Splits integers into bytes, nibbles and words, and joins them again.
#pragma once
#ifndef UQM_CORE_TYPES_BYTES_H_
#define UQM_CORE_TYPES_BYTES_H_

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace uqm
{

// An integer, or an enumeration value as the old C tables use.
template <typename T>
concept IntegerOrEnum = std::integral<T> || std::is_enum_v<T>;

template <std::integral T>
[[nodiscard]] constexpr uint8_t lowByte(T x) noexcept
{
	return static_cast<uint8_t>(x & T {0xFF});
}

template <std::integral T>
[[nodiscard]] constexpr uint8_t highByte(T x) noexcept
{
	if constexpr (sizeof(T) == 1)
	{
		return static_cast<uint8_t>(x);
	}
	else
	{
		return static_cast<uint8_t>(x >> 8);
	}
}

template <IntegerOrEnum T>
[[nodiscard]] constexpr uint8_t lowNibble(T x) noexcept
{
	return static_cast<uint8_t>(static_cast<uint8_t>(x) & 0x0F);
}

template <IntegerOrEnum T>
[[nodiscard]] constexpr uint8_t highNibble(T x) noexcept
{
	return static_cast<uint8_t>(static_cast<uint8_t>(x) >> 4);
}

template <IntegerOrEnum T>
[[nodiscard]] constexpr uint16_t lowWord(T x) noexcept
{
	return static_cast<uint16_t>(static_cast<uint32_t>(x));
}

template <IntegerOrEnum T>
[[nodiscard]] constexpr uint16_t highWord(T x) noexcept
{
	return static_cast<uint16_t>(static_cast<uint32_t>(x) >> 16);
}

// Joins two nibbles into a byte.
template <IntegerOrEnum L, IntegerOrEnum H>
[[nodiscard]] constexpr uint8_t makeByte(L lo, H hi) noexcept
{
	return static_cast<uint8_t>((static_cast<uint8_t>(hi) << 4) | static_cast<uint8_t>(lo));
}

// Joins two bytes into a word.
template <IntegerOrEnum L, IntegerOrEnum H>
[[nodiscard]] constexpr uint16_t makeWord(L lo, H hi) noexcept
{
	return static_cast<uint16_t>((static_cast<uint8_t>(hi) << 8) | static_cast<uint8_t>(lo));
}

// Joins two words into a double word.
template <IntegerOrEnum L, IntegerOrEnum H>
[[nodiscard]] constexpr uint32_t makeDword(L lo, H hi) noexcept
{
	return (static_cast<uint32_t>(hi) << 16) | static_cast<uint16_t>(lo);
}

} // namespace uqm

#endif // UQM_CORE_TYPES_BYTES_H_
