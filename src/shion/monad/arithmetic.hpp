#ifndef SHION_MONAD_ARITHMETIC_H_
#define SHION_MONAD_ARITHMETIC_H_

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES

#include <utility>

#endif

namespace SHION_NAMESPACE 
{

namespace monad
{

SHION_EXPORT struct add_t {
	constexpr auto operator()(const auto& lhs, const auto& rhs) const -> decltype(auto)
	{
		return lhs + rhs;
	}
};

SHION_EXPORT inline constexpr auto add = add_t{};

SHION_EXPORT struct sub_t {
	constexpr auto operator()(const auto& lhs, const auto& rhs) const -> decltype(auto)
	{
		return lhs - rhs;
	}
};

SHION_EXPORT inline constexpr auto sub = sub_t{};

SHION_EXPORT struct mul_t {
	constexpr auto operator()(const auto& lhs, const auto& rhs) const -> decltype(auto)
	{
		return lhs * rhs;
	}
};

SHION_EXPORT inline constexpr auto mul = mul_t{};

SHION_EXPORT struct div_t {
	constexpr auto operator()(const auto& lhs, const auto& rhs) const -> decltype(auto)
	{
		return lhs / rhs;
	}
};

SHION_EXPORT inline constexpr auto div = div_t{};

SHION_EXPORT struct mod_t {
	constexpr auto operator()(const auto& lhs, const auto& rhs) const -> decltype(auto)
	{
		return lhs % rhs;
	}
};

SHION_EXPORT inline constexpr auto mod = mod_t{};

}

}

#endif /* SHION_MONAD_ARITHMETIC_H_ */
