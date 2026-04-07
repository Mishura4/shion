//
// Created by miuna on 4/7/2026.
//

module;

#include <shion/common/defines.hpp>

#if !SHION_IMPORT_STD
#include <cstddef>
#include <bit>
#include <string>
#include <array>
#include <tuple>
#include <span>
#include <variant>
#include <coroutine>
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

auto fibonacci() -> enumerator<int>
{
	auto a = 0;
	auto b = 1;

	while (true)
	{
		a = std::exchange(b, a + b);
		co_yield a;
	}
}

auto wrapped_fibonacci() -> enumerable<const int&>
{
	auto f = fibonacci();
	while (true)
	{
		auto accessor = co_await f.next();
		co_yield *accessor;
	}
}

auto finite_enumeration() -> enumerator<int>
{
	co_yield 6;
	co_yield 7;
}

auto finite_enumerable() -> enumerable<int>
{
	co_yield 6;
	co_yield 7;
}

auto access_local() -> enumerator<int&>
{
	int i = 0;

	while (true)
	{
		co_yield i;
	}
}

auto throws() -> enumerator<int>
{
	co_yield 0;
	throw std::exception{};
	co_yield 1;
}

}


bool tests::enumerator_finite(test& t)
{
	auto enumerator = finite_enumeration();
	std::vector<int> values;
	while (enumerator)
	{
		values.push_back(*enumerator);
		++enumerator;
	}
	enumerator = {};
	TEST_ASSERT(t, std::ranges::equal(values, std::array{6, 7}));

	values = finite_enumerable() | std::ranges::to<std::vector<int>>();
	TEST_ASSERT(t, std::ranges::equal(values, std::array{6, 7}));

	auto enumerable = finite_enumerable();
	values = enumerable | std::ranges::to<std::vector<int>>();
	TEST_ASSERT(t, std::ranges::equal(values, std::array{6, 7}));

	enumerable = finite_enumerable();
	auto moved = std::move(enumerable);
	moved = {};
	return true;
}

bool tests::enumerator_infinite(test& t)
{
	auto values = enumerable(fibonacci()) | std::views::take(5) | std::ranges::to<std::vector<int>>();
	TEST_ASSERT(t, std::ranges::equal(values, std::array{1, 1, 2, 3, 5}));
	return true;
}

bool tests::enumerator_exceptions(test& t)
{
	enumerator<int> enumerator = throws();
	TEST_ASSERT(t, *enumerator == 0);
	bool threw = false;
	try
	{
		++enumerator;
	}
	catch (...)
	{
		threw = true;
	}
	TEST_ASSERT(t, threw);
	TEST_ASSERT(t, enumerator.done());
	return true;
}

bool tests::enumerator_destructions(test& t)
{
	bool destroyed;
	auto do_the_thing = [](bool& flag) -> enumerator<int>
	{
		on_scope_exit on_exit = [&] {
			flag = true;
		};
		co_yield 1;
	};

	// End of scope test
	destroyed = false;
	auto enumerator = do_the_thing(destroyed);
	TEST_ASSERT(t, !destroyed);
	++enumerator;
	TEST_ASSERT(t, destroyed);

	// Coroutine destruction test
	destroyed = false;
	enumerator = do_the_thing(destroyed);
	enumerator = {};
	TEST_ASSERT(t, destroyed);
	return true;
}

bool tests::enumerator_references(test& t)
{
	auto enumerator = access_local();
	TEST_ASSERT(t, *enumerator == 0);
	*enumerator = 1;
	++enumerator;
	TEST_ASSERT(t, *enumerator == 1);
	return true;
}

}
