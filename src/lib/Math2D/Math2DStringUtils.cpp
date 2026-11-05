#include "Math2DStringUtils.h"


namespace uqm
{

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//template <typename T, uqstl::enable_if_t<uqstl::is_base_of_v<Vec2TBase, T>, bool> = true>
//const char* toString(const Vec2T<T>& value, uqstl::span<char> buffer)
//{
//	static const size_t ReqBufferSizePerElement = []() -> auto {
//		if constexpr (uqstl::is_unsigned_v<T>)
//		{
//			return fmt::formatted_size("{}", uqstl::numeric_limits<T>::max());
//		}
//		// return the negative representation, as the '-' sign will add 1 to the length.
//		return fmt::formatted_size("{}", uqstl::numeric_limits<T>::min());
//	};
//
//	static const size_t ReqBufferSize {(ReqBufferSizePerElement * 2) + 1 + 1}; // 2 elements plus the separator and null-terminator.
//	if (buffer.size() < ReqBufferSize)
//	{
//		throw std::length_error(fmt::format("The input buffer is not large enough to hold the possible output of converting this Vec2 to a string. Requires {} bytes, including the null terminator", ReqBufferSize));
//	}
//	fmt::format_to_sz_n(buffer.data(), buffer.size(), "{},{}", value.x, value.y);
//	return buffer.data();
//}

//// explicit template definitions
//template <>
//const char* toString<float>(const Vec2T<float>& value, uqstl::span<char> buffer);
//template <>
//const char* toString<double>(const Vec2T<double>& value, uqstl::span<char> buffer);
//template <>
//const char* toString<uint32_t>(const Vec2T<uint32_t>& value, uqstl::span<char> buffer);

///////////////////////////////////////////////////////////////////////////////////////////////////

//template <typename T>
//std::istream& operator>>(std::istream& in, Vec2T<T>& out)
//{
//	char sep {};
//	if (!(in >> out.x >> sep >> out.y) || (sep != ',' && sep != 'x'))
//	{
//		in.setstate(std::ios::failbit);
//	}
//	return in;
//}
//
//template <>
//std::istream& operator>> <float>(std::istream& in, Vec2T<float>& out);
//template <>
//std::istream& operator>> <double>(std::istream& in, Vec2T<double>& out);
//template <>
//std::istream& operator>> <uint32_t>(std::istream& in, Vec2T<uint32_t>& out);

///////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace uqm

namespace fmt
{
//
//template <typename T>
//auto formatter<::uqm::Vec2T<T>>::format(const ::uqm::Vec2T<T>& v, format_context& ctx) const -> format_context::iterator
//{
//	base_formatter::format(v.x, ctx);
//	auto out {ctx.out()};
//	fmt::format_to(out, ',');
//	return base_formatter::format(v.y, ctx);
//}
//
//template <>
//auto formatter<::uqm::Vec2f>::format(const ::uqm::Vec2f& v, format_context& ctx) const -> format_context::iterator;
//template <>
//auto formatter<::uqm::Vec2d>::format(const ::uqm::Vec2d& v, format_context& ctx) const -> format_context::iterator;
//template <>
//auto formatter<::uqm::Vec2u>::format(const ::uqm::Vec2u& v, format_context& ctx) const -> format_context::iterator;

} // namespace fmt