module;

#include <shion/export.hpp>
#include <shion/common/defines.hpp>

#if !SHION_IMPORT_STD
#include <functional>
#include <type_traits>
#include <concepts>
#include <utility>
#include <array>
#include <functional>
#endif

export module shion:parexp;

#if SHION_IMPORT_STD
import std;
#endif

import :common;
import :monad;
import :utility;

using namespace SHION_NAMESPACE ::literals;

#include "shion/parexp/expr.hpp"

using namespace shion;

struct add_t {
	template <typename T, typename U>
	constexpr decltype(auto) operator()(T&& a, U&& b) const noexcept
	{
		return std::forward<T>(a) + std::forward<U>(b);
	}
};

struct sub_t {
	template <typename T, typename U>
	constexpr decltype(auto) operator()(T&& a, U&& b) const noexcept
	{
		return std::forward<T>(a) - std::forward<U>(b);
	}
};

constexpr auto sub = sub_t{};
constexpr auto f1 = shion::expr(sub, 1, 2); // Subtract 1 and 2, produce -1
static_assert(f1() == -1);

constexpr auto f2 = expr(sub, 7); // Subtract 7 and ?, produce 7 - ?
static_assert(f2(4) == 3);
static_assert(f2(f1()) == 8);

constexpr auto f3 = expr(f2, f1); // ... Produce 7 - (1 - 2) = 8
static_assert(f3() == 8);

// Now let's say we need to reverse `7 - ?`...

constexpr auto f2_rev = expr(sub, std::identity{}, 7); // Subtract ? and 7, produce ? - 7
static_assert(f2_rev(4) == -3);
static_assert(f2_rev(f1()) == -8);

constexpr auto f4 = expr(f2_rev, f1); // ... Produce (1 - 2) - 7 = -8
static_assert(f4() == -8);

//constexpr auto foo = linq(f, 5);
//constexpr auto bar = foo();
//static_assert(bar == true);

//auto test = linq(4);
