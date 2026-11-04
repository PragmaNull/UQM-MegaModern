#pragma once
#ifndef LIB_MATH2D_MATH2DSTRINGS_H_
#define LIB_MATH2D_MATH2DSTRINGS_H_

#include <fmt/format.h>
#include "core/stl/istream.h"

#include "core/stl/stl.h"
#include "core/string/StringUtils.h"
#include "lib/Math2D/Math2D.h"

namespace uqm
{
// toString for Vec2T types.
template <typename T>
const char* toString(const Vec2T<T>& value, uqstl::span<char> buffer)
{
	static const size_t ReqBufferSizePerElement = []() -> auto {
		if constexpr (uqstl::is_unsigned_v<T>)
		{
			return fmt::formatted_size("{}", uqstl::numeric_limits<T>::max());
		}
		// return the negative representation, as the '-' sign will add 1 to the length.
		return fmt::formatted_size("{}", uqstl::numeric_limits<T>::min());
	}();

	static const size_t ReqBufferSize {(ReqBufferSizePerElement * 2) + 1 + 1}; // 2 elements plus the separator and null-terminator.
	if (buffer.size() < ReqBufferSize)
	{
		throw std::length_error(fmt::format("The input buffer is not large enough to hold the possible output of converting this Vec2 to a string. Requires {} bytes, including the null terminator", ReqBufferSize));
	}
	fmt::format_to_sz_n(buffer, "{},{}", value.x, value.y);
	return buffer.data();
}

template <typename T>
std::istream& operator>>(std::istream& in, Vec2T<T>& out)
{
	char sep {};
	if (!(in >> out.x >> sep >> out.y) || (sep != ',' && sep != 'x'))
	{
		in.setstate(std::ios::failbit);
	}
	return in;
}

} // namespace uqm

namespace fmt
{


///////////////////////////////////////////////////////////////////////////////////////////////////
// Custom formatting for Vec types in the fmt library.
// Format specifiers match those for the underlying Vec type. For example, if it is a vector of
// ints, then "{:08}" will produce an 8-character 0-padded output, ie: 00001234,00000012, or you could
// even specify hex "{:#x}" to get 0xAFD,0x2E8, if you ever desired that.
// but if it is a vector of floating point values, then floating point formatting can be used:
// "{:0.02}" produces 0.12,0.08
//
// In addition to these standard numeric formatters, you can specify whether you would like the
// output wrapped in braces by indicating what brace style you want. Options are (,[, or <.  "{" is not currently supported...
// so "{:(}" will output (1234,5678), and "{:<}" would produce <1234,5678>.

template <typename T>
struct formatter<::uqm::Vec2T<T>> : formatter<T>
{
	using base_formatter = formatter<T>;

	// wrapper characters
	char m_openWrapper {'\0'};
	char m_closeWrapper {'\0'};

	constexpr auto parse(format_parse_context& ctx) -> format_parse_context::iterator
	{
		if (auto it {ctx.begin()}; it != ctx.end())
		{
			switch (*it)
			{
				case '(':
					m_openWrapper = '(';
					m_closeWrapper = ')';
					++it;
					break;
				case '<':
					m_openWrapper = '<';
					m_closeWrapper = '>';
					++it;
					break;
				case '[':
					m_openWrapper = '[';
					m_closeWrapper = ']';
					++it;
					break;
			}
			ctx.advance_to(it);
		}

		return base_formatter::parse(ctx);
	}

	auto format(const ::uqm::Vec2T<T>& v, format_context& ctx) const -> format_context::iterator
	{
		auto out {ctx.out()};
		if (m_openWrapper != '\0')
		{
			*out++ = m_openWrapper;
		}
		out = base_formatter::format(v.x, ctx);
		*out++ = ',';
		out = base_formatter::format(v.y, ctx);
		if (m_closeWrapper != '\0')
		{
			*out++ = m_closeWrapper;
		}
		return out;
	}
};

} // namespace fmt

#endif /* LIB_MATH2D_MATH2DSTRINGS_H_ */