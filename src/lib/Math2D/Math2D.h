#pragma once
#ifndef UQM_LIB_MATH2D_MATH2D_H
#define UQM_LIB_MATH2D_MATH2D_H

#include "core/stl/stl.h"

namespace uqm
{

template <typename T, uqstl::enable_if_t<uqstl::is_arithmetic_v<T>, bool> = true>
struct Vec2T
{
	using value_type = T;

	T x {};
	T y {};

	Vec2T& operator+=(const Vec2T rhs)
	{
		x += rhs.x;
		y += rhs.y;
		return *this;
	}

	Vec2T& operator-=(const Vec2T rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		return *this;
	}

	Vec2T& operator*=(const Vec2T rhs)
	{
		x *= rhs.x;
		y *= rhs.y;
		return *this;
	}

	Vec2T& operator/=(const Vec2T rhs)
	{
		x /= rhs.x;
		y /= rhs.y;
		return *this;
	}

	auto operator<=>(const Vec2T&) const = default;
	bool operator==(const Vec2T&) const = default;
	bool operator!=(const Vec2T&) const = default;
};

template <typename T, uqstl::enable_if_t<uqstl::is_arithmetic_v<T>, bool> = true>
Vec2T<T> operator+(const Vec2T<T>& lhs, const Vec2T<T>& rhs)
{
	return {lhs.x + rhs.x, lhs.y + rhs.y};
}

template <typename T, uqstl::enable_if_t<uqstl::is_arithmetic_v<T>, bool> = true>
Vec2T<T> operator-(const Vec2T<T>& lhs, const Vec2T<T>& rhs)
{
	return {lhs.x - rhs.x, lhs.y - rhs.y};
}

template <typename T, uqstl::enable_if_t<uqstl::is_arithmetic_v<T>, bool> = true>
Vec2T<T> operator*(const Vec2T<T>& lhs, const Vec2T<T>& rhs)
{
	return {lhs.x * rhs.x, lhs.y * rhs.y};
}

template <typename T, typename N,
		  uqstl::enable_if_t<uqstl::is_arithmetic_v<T>, bool> = true,
		  uqstl::enable_if_t<uqstl::is_arithmetic_v<N>, bool> = true>
auto operator*(const Vec2T<T>& lhs, const N scalar)
{
	using outT = decltype(T {} * N {});
	return Vec2T<outT> {lhs.x * scalar, lhs.y * scalar};
}

template <typename T, typename N,
		  uqstl::enable_if_t<uqstl::is_arithmetic_v<T>, bool> = true,
		  uqstl::enable_if_t<uqstl::is_arithmetic_v<N>, bool> = true>
auto operator/(const Vec2T<T>& lhs, const N scalar)
{
	using outT = decltype(T {1} / N {1});
	return Vec2T<outT> {lhs.x / scalar, lhs.y / scalar};
}


using Vec2f = Vec2T<float>;
using Vec2d = Vec2T<double>;
using Vec2u = Vec2T<uint32_t>;


} // namespace uqm

#endif // UQM_LIB_MATH2D_MATH2D_H
