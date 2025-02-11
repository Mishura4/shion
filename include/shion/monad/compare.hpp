#ifndef SHION_MONAD_COMPARE_H_
#define SHION_MONAD_COMPARE_H_
#ifdef SHION_USE_MODULES

import std;

#else

#include <shion/decl.hpp>

#include <functional>

#endif

namespace shion
{

namespace monad
{



SHION_DECL struct equal_t {
	constexpr auto operator()(const auto& lhs, const auto& rhs) const -> decltype(auto)
	{
		return lhs == rhs;
	}
};

SHION_DECL_INLINE constexpr auto equal = equal_t{};

SHION_DECL struct not_equal_t {
	constexpr auto operator()(const auto& lhs, const auto& rhs) const -> decltype(auto)
	{
		return lhs != rhs;
	}
};

SHION_DECL_INLINE constexpr auto not_equal = not_equal_t{};

SHION_DECL struct greater_t {
	constexpr auto operator()(const auto& lhs, const auto& rhs) const -> decltype(auto)
	{
		return lhs > rhs;
	}
};

SHION_DECL_INLINE constexpr auto greater = greater_t{};

SHION_DECL struct greater_eq_t {
	constexpr auto operator()(const auto& lhs, const auto& rhs) const -> decltype(auto)
	{
		return lhs >= rhs;
	}
};

SHION_DECL_INLINE constexpr auto greater_eq = greater_eq_t{};

SHION_DECL struct less_t {
	constexpr auto operator()(const auto& lhs, const auto& rhs) const -> decltype(auto)
	{
		return lhs < rhs;
	}
};

SHION_DECL_INLINE constexpr auto less = less_t{};

SHION_DECL struct less_eq_t {
	constexpr auto operator()(const auto& lhs, const auto& rhs) const -> decltype(auto)
	{
		return lhs <= rhs;
	}
};

SHION_DECL_INLINE constexpr auto less_eq = less_eq_t{};

SHION_DECL struct spaceship_t {
	constexpr auto operator()(const auto& lhs, const auto& rhs) const -> decltype(auto)
	{
		return lhs <=> rhs;
	}
};

SHION_DECL_INLINE constexpr auto spaceship = spaceship_t{};

}

}

#endif /* SHION_MONAD_COMPARE_H_ */
