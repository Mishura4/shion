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
#include <algorithm>
#include <ranges>
#include <vector>
#include <functional>
#endif

#include "../tests.hpp"

module shion.tests;

#if SHION_IMPORT_STD
import std;
#endif

import shion;

namespace shion
{

bool tests::async_void(test& t)
{
	auto async = shion::async<>{[](std::invocable auto callback) -> void {
		callback();
	}};
	TEST_ASSERT(t, async.await_ready());
	return true;
}

bool tests::async_value(test &t)
{
	auto async = shion::async<int>{[](std::invocable<int> auto callback) -> void {
		callback(42);
	}};
	TEST_ASSERT(t, async.await_ready());
	TEST_ASSERT(t, async.get() == 42);
	return true;
}

bool tests::async_reference(test &t)
{
	auto var = 42;
	auto async = shion::async<int&>{[&](std::invocable<int&> auto callback) -> void {
		callback(var);
	}};
	TEST_ASSERT(t, async.await_ready());
	TEST_ASSERT(t, async.get() == 42);
	return true;
}

bool tests::async_move(test& t)
{
	auto async = shion::async<int>{[](std::invocable<int> auto callback) -> void {
		callback(42);
	}};
	auto async2 = std::move(async);
	TEST_ASSERT(t, !async.valid());
	TEST_ASSERT(t, async2.await_ready());
	TEST_ASSERT(t, async2.get() == 42);
	return true;
}

bool tests::async_thread(test& t)
{
	std::jthread thread;
	auto async = shion::async<int>{[&thread](std::invocable<int> auto callback) -> void {
		thread = std::jthread([cb = std::move(callback)]() mutable {
			std::this_thread::sleep_for(2s);
			cb(67);
		});
	}};
	TEST_ASSERT(t, !async.await_ready());
	std::this_thread::sleep_for(3s);
	TEST_ASSERT(t, async.await_ready());
	TEST_ASSERT(t, async.get() == 67);
	return true;
}

bool tests::async_await(test& t)
{
	std::promise<bool> promise;
	std::future<bool> future = promise.get_future();
	std::jthread thread;
	auto coro = [](test& test_, std::promise<bool> p, std::jthread& th) -> coro::co_awaitable<int> {
		auto         async = shion::async<int>{[&th](std::invocable<int> auto callback) -> void {
			th = std::jthread([cb = std::move(callback)]() mutable {
				std::this_thread::sleep_for(2s);
				cb(67);
			});
		}};
		TEST_CO_ASSERT(test_, !async.await_ready());
		co_await async;
		TEST_CO_ASSERT(test_, async.await_ready());
		TEST_CO_ASSERT(test_, async.get() == 67);
		p.set_value(true);
		co_return true;
	}(t, std::move(promise), thread);
	auto result = future.wait_for(4s);
	TEST_ASSERT(t, result == std::future_status::ready);
	TEST_ASSERT(t, future.get());
	return true;
}

bool tests::async_destruction(test& t)
{
	return true;
}

}
