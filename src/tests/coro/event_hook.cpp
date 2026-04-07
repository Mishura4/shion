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
#endif

#include "../tests.hpp"

module shion.tests;

#if SHION_IMPORT_STD
import std;
#endif

import shion;

namespace shion
{
/*
bool tests::event_hook_simple(test& t)
{
	event_hook<int> hook;
	auto task1 = [](event_hook<int>& hook) -> state_machine<int> {
		int i = co_await hook;
		co_yield i;
	}(hook);
	task1.advance();
	hook.push(42);
	TEST_ASSERT(t, task1.get() == 42);
	return true;
}

bool tests::event_hook_order(test& t)
{
	std::vector<int> v;
	event_hook<int> hook;
	auto task1 = [](event_hook<int>& hook, std::vector<int>& v) -> state_machine<void> {
		int i = co_await hook;
		v.push_back(1);
	}(hook, v);
	auto task2 = [](event_hook<int>& hook, std::vector<int>& v) -> state_machine<void> {
		int i = co_await hook;
		v.push_back(2);
	}(hook, v);
	auto task3 = [](event_hook<int>& hook, std::vector<int>& v) -> state_machine<void> {
		int i = co_await hook;
		v.push_back(3);
	}(hook, v);
	task1.advance();
	task2.advance();
	task3.advance();
	hook.push(42);
	TEST_ASSERT(t, std::ranges::equal(v, std::array{ 1, 2, 3 }));
	return true;
}

bool tests::event_hook_stress_push(test& t)
{
	auto num_proc = std::thread::hardware_concurrency();
	if (num_proc == 1)
	{
		t.skip("Can't stress test with only one thread");
		return true;
	}

	constexpr int total_events = 16 * 1024;
	std::atomic<int> events_fired{0};
	double events_per_thread = static_cast<double>(total_events) / num_proc;
	event_hook<int> hook;
	auto threads = std::vector<std::jthread>(num_proc);
	auto exceptions = std::vector<std::exception_ptr>(num_proc);
	auto thread_fun = [&](int thread_id) {
		try
		{
			auto my_event_count_f = std::floor((thread_id + 1) * events_per_thread) - std::floor(thread_id * events_per_thread);
			auto my_event_count   = static_cast<std::size_t>(my_event_count_f);
			for (auto i = 0zu; i < my_event_count; ++i)
			{
				auto hooker_fun = [](event_hook<int>& hook, std::atomic<int>& var) -> job {
					co_await hook;
					var.fetch_add(1, std::memory_order_relaxed);
				};
				hooker_fun(hook, events_fired);
				hook.push(thread_id);
			}
		}
		catch (...)
		{
			exceptions[thread_id] = std::current_exception();
		}
	};
	for (auto&& [i, thread] : std::views::enumerate(threads)) thread = std::jthread(thread_fun, i);
	for (auto&& thread : threads) thread.join();

	for (auto&& [i, exception] : std::views::enumerate(exceptions))
	{
		if (exception)
		{
			try
			{
				std::rethrow_exception(exception);
			}
			catch (const std::exception& e)
			{
				t.fail(std::format("Thread {} exited with exception {}", i, e.what()));
			}
			catch (...)
			{
				t.fail(std::format("Thread {} exited with an unknown exception", i));
			}
		}
	}
	TEST_ASSERT(t, events_fired == total_events);
	return true;
}
*/
}
