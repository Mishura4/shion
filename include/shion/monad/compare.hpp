#ifndef SHION_MONAD_COMPARE_H_
#define SHION_MONAD_COMPARE_H_

#include <shion/defines.hpp>

#if !SHION_BUILDING_MODULES

#include <functional>
#include <utility>

#endif

namespace shion
{

namespace monad
{



SHION_EXPORT struct equal_t {
	constexpr auto operator()(const auto& lhs, const auto& rhs) const -> decltype(auto)
	{
		return lhs == rhs;
	}
};

SHION_EXPORT inline constexpr auto equal = equal_t{};

SHION_EXPORT struct not_equal_t {
	constexpr auto operator()(const auto& lhs, const auto& rhs) const -> decltype(auto)
	{
		return lhs != rhs;
	}
};

SHION_EXPORT inline constexpr auto not_equal = not_equal_t{};

SHION_EXPORT struct greater_t {
	constexpr auto operator()(const auto& lhs, const auto& rhs) const -> decltype(auto)
	{
		return lhs > rhs;
	}
};

SHION_EXPORT inline constexpr auto greater = greater_t{};

SHION_EXPORT struct greater_eq_t {
	constexpr auto operator()(const auto& lhs, const auto& rhs) const -> decltype(auto)
	{
		return lhs >= rhs;
	}
};

SHION_EXPORT inline constexpr auto greater_eq = greater_eq_t{};

SHION_EXPORT struct less_t {
	constexpr auto operator()(const auto& lhs, const auto& rhs) const -> decltype(auto)
	{
		return lhs < rhs;
	}
};

SHION_EXPORT inline constexpr auto less = less_t{};

SHION_EXPORT struct less_eq_t {
	constexpr auto operator()(const auto& lhs, const auto& rhs) const -> decltype(auto)
	{
		return lhs <= rhs;
	}
};

SHION_EXPORT inline constexpr auto less_eq = less_eq_t{};

SHION_EXPORT struct spaceship_t {
	constexpr auto operator()(const auto& lhs, const auto& rhs) const -> decltype(auto)
	{
		return lhs <=> rhs;
	}
};

SHION_EXPORT inline constexpr auto spaceship = spaceship_t{};

}

}

#endif /* SHION_MONAD_COMPARE_H_ */
