//
// Created by miuna on 3/30/2026.
//

module;

#include "../tests.hpp"

module shion.tests;

import std;
import shion;

namespace shion
{

bool tests::concurrent_flush_simple(test& t)
{
	constexpr int first = 42;
	concurrent_flush<int> flush;
	std::optional<int> value;

	flush.push(first);
	flush.flush([&value](int val) {
		value = val;
	});
	TEST_ASSERT(t, value == first);
	return true;
}

bool tests::concurrent_flush_order(test& t)
{
	constexpr int first = 42;
	constexpr int second = 67;
	concurrent_flush<int> flush;
	int step = 0;

	bool success = false;
	flush.push(first);
	flush.push(second);
	flush.flush([&](int val) {
		switch (step)
		{
			case 0:
				if (val != first)
				{
					throw std::logic_error(std::format("first value should be {}", first));
				}
				break;

			case 1:
				if (val != second)
				{
					throw std::logic_error(std::format("second value should be {}", second));
				}
				break;

			default:
				throw std::logic_error(std::format("unknown step {}", step));
		}
		++step;
	});
	TEST_ASSERT(t, step == 2);
	return true;
}

bool tests::concurrent_flush_state_machine(test& t)
{
	constexpr int first = 42;
	constexpr int second = 67;
	constexpr int third = 69;
	concurrent_flush<int> flush;

	flush.push(first);
	flush.push(second);
	flush.push(third);
	auto machine = flush.pop();
	auto enumerable = std::move(machine);
	TEST_ASSERT(t, std::ranges::equal(enumerable, std::array{ first, second, third }));
	enumerable = {};
	return true;
}

}