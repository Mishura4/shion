//
// Created by miuna on 4/2/2026.
//

#ifndef SHION_SCOPE_HPP_
#define SHION_SCOPE_HPP_

#include <shion/common/defines.hpp>

#ifndef SHION_BUILDING_MODULES
#include <concept>
#include <utility>
#include <type_traits>
#include <functional>

#include <shion/meta/macros.hpp>
#endif

namespace SHION_NAMESPACE
{

SHION_EXPORT
template <typename Fun>
struct on_scope_exit
{
	SHION_NO_UNIQUE_ADDRESS Fun fun{};

	constexpr on_scope_exit() = default;
	constexpr on_scope_exit(const on_scope_exit&) noexcept = default;
	constexpr on_scope_exit(on_scope_exit&&) noexcept = default;
	template <typename... Args>
	requires (std::constructible_from<Fun, Args...>)
	constexpr on_scope_exit(Args&&... args) noexcept(std::is_nothrow_constructible_v<Fun, Args...>) :
		fun(std::forward<Args>(args)...)
	{
	}

	constexpr ~on_scope_exit() noexcept(std::is_nothrow_invocable_v<Fun>)
	{
		std::invoke(fun);
	}

	constexpr auto operator=(const on_scope_exit&) noexcept -> on_scope_exit& = default;
	constexpr auto operator=(on_scope_exit&&) noexcept -> on_scope_exit& = default;

	friend constexpr auto operator<=>(on_scope_exit const&, on_scope_exit const&) = default;

	friend constexpr auto swap(on_scope_exit& a, on_scope_exit& b) noexcept -> void
	{
		using std::swap;
		swap(a.fun, b.fun);
	}
};

template <typename Fun>
on_scope_exit(Fun fun) -> on_scope_exit<Fun>;

}

#endif //LOTUS_SCOPE_HPP_
