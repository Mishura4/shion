module;

#include <shion/common/defines.hpp>

#if SHION_INTELLISENSE
#include <shion/coro/state_machine.hpp>
#endif

#if !SHION_IMPORT_STD
#include <cstddef>
#include <bit>
#include <string>
#include <array>
#include <tuple>
#include <span>
#include <variant>
#endif

#include "../tests.hpp"

module shion.tests;

#if SHION_IMPORT_STD
import std;
#endif

import shion;

namespace shion
{

namespace
{

constexpr auto make_t1 = []() -> shion::state_machine<int> {
	co_yield 1;
	co_yield 1;
	co_yield 2;
	co_yield 3;
	co_return 5;
};

constexpr auto make_t2 = []() -> shion::state_machine<int, std::string> {
	co_yield 1;
	co_yield 1;
	co_yield 2;
	co_yield 3;
	co_return "hello world!";
};

auto state_machine_generator_impl1(tests::test& t) -> bool {
	shion::state_machine<int> t1;
	t1 = make_t1();
	
	TEST_ASSERT(t, t1() == 1);
	TEST_ASSERT(t, t1() == 1);
	TEST_ASSERT(t, t1() == 2);
	TEST_ASSERT(t, t1() == 3);
	TEST_ASSERT(t, t1() == 5);
	TEST_ASSERT(t, t1.done());
	return true;
}

auto state_machine_generator_impl2(tests::test& t) -> bool {
	shion::state_machine<int, std::string> t2;
	t2 = make_t2();

	TEST_ASSERT(t, std::get<0>(t2()) == 1);
	TEST_ASSERT(t, std::get<0>(t2()) == 1);
	TEST_ASSERT(t, std::get<0>(t2()) == 2);
	TEST_ASSERT(t, std::get<0>(t2()) == 3);
	TEST_ASSERT(t, std::get<1>(t2()) == "hello world!");
	TEST_ASSERT(t, t2.done());
	return true;
}

auto state_machine_awaitable_impl1(tests::test& t) -> state_machine<bool> {
	shion::state_machine<int> t1;
	t1 = make_t1();
	
	TEST_CO_ASSERT(t, (co_await t1) == 1);
	TEST_CO_ASSERT(t, (co_await t1) == 1);
	TEST_CO_ASSERT(t, (co_await t1) == 2);
	TEST_CO_ASSERT(t, (co_await t1) == 3);
	TEST_CO_ASSERT(t, (co_await t1) == 5);
	TEST_CO_ASSERT(t, t1.done());
	co_return true;
}

auto state_machine_awaitable_impl2(tests::test& t) -> state_machine<bool> {
	shion::state_machine<int, std::string> t2;
	t2 = make_t2();

	TEST_CO_ASSERT(t, std::get<0>(co_await t2) == 1);
	TEST_CO_ASSERT(t, std::get<0>(co_await t2) == 1);
	TEST_CO_ASSERT(t, std::get<0>(co_await t2) == 2);
	TEST_CO_ASSERT(t, std::get<0>(co_await t2) == 3);
	TEST_CO_ASSERT(t, std::get<1>(co_await t2) == "hello world!");
	TEST_CO_ASSERT(t, t2.done());
	co_return true;
}

}

bool tests::state_machine_generator(test& t)
{
	if (!state_machine_generator_impl1(t))
		return false;
	
	if (!state_machine_generator_impl2(t))
		return false;

	return true;
}

bool tests::state_machine_coroutine(test& t)
{
	if (!state_machine_awaitable_impl1(t)())
		return false;
	
	if (!state_machine_awaitable_impl2(t)())
		return false;

	return true;
}

bool tests::state_machine_continuation(test& t)
{
	bool t1 = false;
	bool t2 = false;
	auto impl = [&]() -> state_machine<int> {
		t1 = co_await state_machine_awaitable_impl1(t);
		t2 = co_await state_machine_awaitable_impl2(t);
		co_return 5;
	}();

	TEST_ASSERT(t, impl() == 5);
	TEST_ASSERT(t, t1);
	TEST_ASSERT(t, t2);
	return true;
}

}