//
// Created by miuna on 3/26/2026.
//

#ifndef SHION_MATH_ARITHMETIC_HPP_
#define SHION_MATH_ARITHMETIC_HPP_

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#include <type_traits>
#include <cmath>
#include <shion/common.hpp>
#endif

SHION_EXPORT namespace SHION_NAMESPACE
{

inline namespace math
{

struct ceildiv_t
{
	template <typename T>
	constexpr T operator()(T n, T d) noexcept
	{
		// Generic - relies on ADL
		return ceildiv(n, d);
	}

	template <std::floating_point T>
	constexpr T operator()(T n, T d) noexcept
	{
		return std::ceil(n / d);
	}

	template <std::integral T>
	constexpr T operator()(T n, T d) noexcept
	{
		// https://stackoverflow.com/a/33790603/30331907
		return n / d + (((n < 0) ^ (d > 0)) && (n % d));
	}
};

inline constexpr auto ceildiv = ceildiv_t{};

}

}

#endif // SHION_MATH_ARITHMETIC_HPP_