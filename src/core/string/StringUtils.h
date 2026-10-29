#pragma once

#ifndef UQM_CORE_STRING_STRINGUTILS_H_
#define UQM_CORE_STRING_STRINGUTILS_H_

#define __STDC_WANT_LIB_EXT1__ 1

#include <algorithm>
#include <ranges>
#include <charconv>
#include <cctype>
#include <fmt/format.h>
#include "core/stl/string.h"
#include "core/types/enum.h"

namespace uqm
{
static constexpr const char* TrueText {"true"};
static constexpr const char* FalseText {"false"};


template <typename T>
concept HasToStringImpl =
	requires(T v) {
		{ toStringImpl(v) } -> ::std::convertible_to<const char*>;
	};

[[nodiscard]] inline constexpr bool isEmpty(const char* sz) noexcept
{
	return sz == nullptr || sz[0] == '\0';
}

[[nodiscard]] constexpr size_t strlenConstexpr(uqgsl::czstring str) noexcept
{
	if (isEmpty(str)) [[unlikely]]
	{
		return 0;
	}
	return uqstl::string_view {str}.size();
}

[[nodiscard]] inline const char* toString(bool value)
{
	return value ? TrueText : FalseText;
}

template <EnumType E>
[[nodiscard]] static constexpr const char* toString(const E e)
{
#ifdef WITH_MAGIC_ENUM
	// a toString implementation will override the magic_enum output.
	if constexpr (HasToStringImpl<E>)
	{
		return toStringImpl(e);
	}
	else
	{
		return magic_enum::enum_name(e).data();
	}
#else
	static_assert(HasToStringImpl<E>, "Enum must have a toStringImpl() method to use this toString() method.") return toStringImpl(e);
#endif
}

template <typename T, typename uqstl::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
inline const char* toString(const T value, uqstl::span<char> buffer)
{
	if (buffer.size() == 0) [[unlikely]]
	{
		buffer[0] = '\0';
		return buffer.data();
	}

	if constexpr (uqstl::is_same_v<bool, uqstl::remove_cv_t<T>>)
	{
		static constexpr size_t TrueLen {strlenConstexpr(TrueText)};
		static constexpr size_t FalseLen {strlenConstexpr(FalseText)};
		if (value)
		{
			if (buffer.size() > TrueLen)
			{
				uqstl::memcpy(buffer.data(), TrueText, TrueLen);
				buffer[TrueLen] = '\0';
			}
			else
			{
				buffer[0] = '\0';
			}
		}
		else
		{
			if (buffer.size() > FalseLen)
			{
				uqstl::memcpy(buffer.data(), FalseText, FalseLen);
				buffer[FalseLen] = '\0';
			}
			else
			{
				buffer[0] = '\0';
			}
		}
		return buffer.data();
	}

	const auto [_, len] {fmt::format_to_n(buffer.data(), buffer.size() - 1, "{}", value)};
	buffer[len] = '\0';
	return buffer.data();
}

template <EnumType E>
[[nodiscard]] inline uqstl::optional<E> fromString(const uqstl::string_view s)
{
	return magic_enum::enum_cast<E>(s, magic_enum::case_insensitive);
}


[[nodiscard]] inline bool compareCharICase(const char a, const char b) noexcept
{
	return uqstl::tolower(static_cast<const unsigned char>(a)) == uqstl::tolower(static_cast<const unsigned char>(b));
}

[[nodiscard]] inline bool compareICase(uqstl::string_view lhs, uqstl::string_view rhs) noexcept
{
	return std::ranges::equal(lhs, rhs, [](const char a, const char b) {
		return uqstl::tolower(static_cast<const unsigned char>(a)) == uqstl::tolower(static_cast<const unsigned char>(b));
	});
}


[[nodiscard]] inline constexpr bool isEmpty(uqstl::string_view str) noexcept
{
	return str.empty();
}
[[nodiscard]] inline constexpr bool isEmpty(const uqstl::string& str) noexcept
{
	return str.empty();
}

template <typename StringT>
[[nodiscard]] inline constexpr const char* c_str(const StringT& str) noexcept
{
	return str.c_str();
}
template <>
[[nodiscard]] inline constexpr const char* c_str<char*>(char* const& str) noexcept
{
	return str;
}
template <>
[[nodiscard]] inline constexpr const char* c_str<const char*>(const char* const& str) noexcept
{
	return str;
}
template <>
[[nodiscard]] inline constexpr const char* c_str<uqgsl::not_null_czstring>(const uqgsl::not_null_czstring& str) noexcept
{
	return str;
}
template <>
[[nodiscard]] inline constexpr const char* c_str<uqstl::string_view>(const uqstl::string_view& str) noexcept
{
	return str.data();
}

template <typename T, typename uqstl::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
inline uqstl::errc parseStr(uqstl::string_view str, T& out)
{
	const char* const end {uqstl::to_address(str.end())};
	T value {};
	const auto [ptr, ec] = uqstl::from_chars(str.data(), end, value);
	if (ec != uqstl::errc {})
	{
		return ec;
	}
	if (ptr != end)
	{
		return uqstl::errc::invalid_argument;
	}
	out = value;
	return {};
}

// template specialization for bool. Impl in cpp.
template <>
uqstl::errc parseStr(uqstl::string_view str, bool& out);

template <EnumType E>
inline uqstl::errc parseStr(uqstl::string_view str, E& out)
{
	using UnderlyingT = std::underlying_type_t<E>;

	UnderlyingT intermediate {};
	const auto ec {parseStr(str, intermediate)};
	if (ec == std::errc {}) [[likely]]
	{
		out = static_cast<E>(intermediate);
	}
	return ec;
}

inline size_t strncpy_safe(uqstl::span<char> dest, uqstl::string_view src)
{
	if (dest.empty()) [[unlikely]]
	{
		return 0;
	}

#ifdef __STDC_LIB_EXT1__
	strncpy_s(dest.data(), dest.size(), src.data(), src.size());
	return std::min(src.size(), dest.size() - 1);
#else

	const size_t toCopy {std::min(src.size(), dest.size() - 1)};

	for (size_t i = 0; i < toCopy; ++i)
	{
		dest[i] = src[i];
		if (src[i] == '\0')
		{
			return i;
		}
	}
	dest[toCopy] = '\0';

	return toCopy;
#endif
}

// Splits 'input' on every occurrence of 'delimiter'.
// If Delimiter is char, splits on that exact character.
// If Delimiter is string_view, splits on any character contained in it (character-set semantics).
// Tokens are written into 'out' as non-owning views into 'input'.
// Consecutive or trailing delimiters produce empty tokens.
// Returns the number of tokens written (at most out.size()).
template <typename DelimiterT, typename OutT = uqstl::string_view>
	requires(uqstl::is_same_v<DelimiterT, char> || uqstl::is_same_v<DelimiterT, uqgsl::czstring>)
static void tokenize(uqstl::string_view input, uqstl::vector<OutT>& out, DelimiterT delimiter, bool includeEmpty)
{
	while (true)
	{
		const auto pos = [&]() {
			if constexpr (uqstl::is_same_v<DelimiterT, char>)
			{
				return input.find(delimiter);
			}
			else
			{
				return input.find_first_of(delimiter);
			}
		}();

		if (pos == uqstl::string_view::npos)
		{
			if (includeEmpty || !input.empty())
			{
				out.emplace_back(input);
			}
			break;
		}

		if (includeEmpty || pos > 0)
		{
			out.emplace_back(input.substr(0, pos));
		}
		input.remove_prefix(pos + 1);
	}
}
template <typename DelimiterT, typename OutT = uqstl::string_view>
	requires(uqstl::is_same_v<DelimiterT, char> || uqstl::is_same_v<DelimiterT, uqgsl::czstring>)
static auto tokenize(uqstl::string_view input, DelimiterT delimiter, bool includeEmpty) -> uqstl::vector<OutT>
{
	uqstl::vector<OutT> result {};
	tokenize(input, result, delimiter, includeEmpty);
	return result;
}

inline uqstl::string_view trimLeft(const uqstl::string_view in, const char* trimChars = " \t\r\n")
{
	if (const auto firstNonSpacePos {in.find_first_not_of(trimChars)}; firstNonSpacePos != uqstl::string_view::npos)
	{
		return in.substr(firstNonSpacePos);
	}
	return in;
}
inline uqstl::string_view trimRight(const uqstl::string_view in, const char* trimChars = " \t\r\n")
{
	if (const auto lastNonSpacePos {in.find_last_not_of(trimChars)}; lastNonSpacePos != uqstl::string_view::npos)
	{
		return in.substr(0, lastNonSpacePos + 1);
	}
	return in;
}

inline uqstl::string_view trim(const uqstl::string_view in, const char* trimChars = " \t\r\n")
{
	return trimRight(trimLeft(in, trimChars), trimChars);
}


static inline constexpr uint64_t FNV1a_OffsetBasis {14695981039346656037ull};
static inline constexpr uint64_t FNV1a_Prime {1099511628211ull};
// FNV-1a hash function for compile-time string hashing. Not suitable for cryptographic purposes, but good for things like switch statements on strings.
static constexpr uint64_t hashQuick64(const uqstl::string_view str) noexcept
{

	uint64_t hash = FNV1a_OffsetBasis;

	for (unsigned char c : str)
	{
		hash ^= c;
		hash *= FNV1a_Prime;
	}

	return hash;
}

static constexpr std::array<unsigned char, 256> makeLowercaseTable()
{
	std::array<unsigned char, 256> table {};

	for (int i = 0; i < 256; ++i)
	{
		if (i >= 'A' && i <= 'Z')
		{
			table[i] = static_cast<unsigned char>(i + ('a' - 'A'));
		}
		else
		{
			table[i] = static_cast<unsigned char>(i);
		}
	}

	return table;
}

static constexpr uint64_t hashQuick64CaseInsensitive(const uqstl::string_view str) noexcept
{
	constexpr uint64_t OffsetBasis {14695981039346656037ull};
	constexpr uint64_t Prime {1099511628211ull};

	constexpr auto AsciiToLowerTable {makeLowercaseTable()};

	uint64_t hash = FNV1a_OffsetBasis;

	for (unsigned char c : str)
	{
		hash ^= AsciiToLowerTable[static_cast<int>(c)];
		hash *= FNV1a_Prime;
	}

	return hash;
}


///////////////////////////////////////////// ENUM //////////////////////////////////////////////////

template <EnumType E>
struct EnumNames
{
	template <typename T = uqstl::string_view>
	static auto list() -> uqstl::vector<T>
	{
		uqstl::vector<T> out {};
		const auto enumValues {magic_enum::enum_values<E>()};
		out.reserve(enumValues.size());
		for (const E value : enumValues)
		{
			out.emplace_back(toString(value));
		}
		return out;
	}

	template <typename T = uqstl::string_view>
	static auto map() -> uqstl::unordered_map<T, E>
	{
		uqstl::unordered_map<T, E> out {};
		const auto enumValues {magic_enum::enum_values<E>()};
		for (const E value : enumValues)
		{
			out.insert(uqstl::make_pair(toString(value), value));
		}
		return out;
	}

	template <typename T = uqstl::string_view>
	static auto pairs() -> uqstl::vector<uqstl::pair<T, E>>
	{
		uqstl::vector<uqstl::pair<T, E>> out {};
		const auto enumValues {magic_enum::enum_values<E>()};
		for (const E value : enumValues)
		{
			out.emplace_back(toString(value), value);
		}
		return out;
	}
};


} // namespace uqm

namespace fmt
{
// same as format_to_n, but zero-terminates the string. "outSize" must be large enough to
// accomodate the trailing null terminator.
template <typename OutputIt, typename... T,
		  FMT_ENABLE_IF(detail::is_output_iterator<OutputIt, char>::value)>
FMT_INLINE auto format_to_sz_n(OutputIt out, size_t outSize, format_string<T...> fmt,
							   T&&... args) -> format_to_n_result<OutputIt>
{
	const auto result {vformat_to_n(out, outSize - 1, fmt.str, vargs<T...> {{args...}})};
	*result.out = '\0';
	return result;
}

template <typename... T>
FMT_INLINE auto format_to_sz_n(uqstl::span<char> out, format_string<T...> fmt, T&&... args)
{
	return format_to_sz_n(out.data(), out.size(), fmt, std::forward<T>(args)...);
}
template <size_t N, typename... T>
FMT_INLINE auto format_to_sz_n(char (&out)[N], format_string<T...> fmt, T&&... args)
{
	return format_to_sz_n(out, N, fmt, std::forward<T>(args)...);
}


template <EnumType E>
struct formatter<E> : formatter<std::underlying_type_t<E>>
{
	using base_formatter = formatter<std::underlying_type_t<E>>;

	bool m_useString {false};
	bool m_includeNumericValueWithString {false};

	constexpr auto parse(format_parse_context& ctx)
	{
		if (auto it {ctx.begin()}; it != ctx.end() && *it == 's')
		{
#ifndef WITH_MAGIC_ENUM
			if constexpr (!::uqm::HasToStringImpl<E>)
			{
				fmt::report_error("Enum requires a toString() impl to use the 's' specifier, or install the magic_enum library.");
			}
#endif

			m_useString = true;
			++it;
			// 's' followed by 'n' will output the numeric value as well.
			if (it != ctx.end() && *it == 'n')
			{
				m_includeNumericValueWithString = true;
				++it;
			}
			ctx.advance_to(it);
		}

		return base_formatter::parse(ctx);
	}

	template <typename FormatContext>
	auto format(E value, FormatContext& ctx) const -> format_context::iterator
	{
		auto out {ctx.out()};
		if (m_useString)
		{
#ifdef WITH_MAGIC_ENUM
			// a toString implementation will override the magic_enum output.
			if constexpr (::uqm::HasToStringImpl<E>)
			{
				out = fmt::format_to(out, "{}", toStringImpl(value));
			}
			else
			{
				out = fmt::format_to(out, "{}", magic_enum::enum_name(value));
			}
#else
			out = fmt::format_to(out, "{}", toString(value));
#endif
			if (m_includeNumericValueWithString)
			{
				*out++ = '-';
				*out++ = '>';
				out = base_formatter::format(static_cast<std::underlying_type_t<E>>(value), ctx);
			}

			return out;
		}

		return base_formatter::format(static_cast<std::underlying_type_t<E>>(value), ctx);
	}
};

template <EnumType E>
struct formatter<::uqm::EnumNames<E>> : formatter<E>
{
	using base_formatter = formatter<E>;
	const char* m_sep {", "};

	constexpr auto parse(format_parse_context& ctx)
	{
		if (auto it {ctx.begin()}; it != ctx.end() && *it == '|')
		{
			m_sep = " | ";
			++it;
			ctx.advance_to(it);
		}

		return base_formatter::parse(ctx);
	}

	template <typename FormatContext>
	auto format(::uqm::EnumNames<E> value, FormatContext& ctx) const -> format_context::iterator
	{
		auto out {ctx.out()};
		static constexpr auto enumValues {magic_enum::enum_values<E>()};
		bool first {true};
		for (const auto val : enumValues)
		{
			if (!first)
			{
				out = fmt::format_to(out, "{}", m_sep);
			}
			out = base_formatter::format(static_cast<E>(val), ctx);
			first = false;
		}

		return out;
	}
};

} // namespace fmt

#endif /* UQM_CORE_STRING_STRINGUTILS_H_ */
