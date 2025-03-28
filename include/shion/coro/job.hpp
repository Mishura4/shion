#pragma once

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#include <shion/meta/type_traits.hpp>

#include <type_traits>
#include <utility>

#include "coro.hpp"
#endif

namespace SHION_NAMESPACE {

SHION_EXPORT struct job_dummy {
};

/**
 * @class job job.h coro/job.h
 * @brief Extremely light coroutine object designed to send off a coroutine to execute on its own.
 *
 * This object stores no state and is the recommended way to use coroutines if you do not need to co_await the result.
 *
 * @warning - It cannot be co_awaited, which means the second it co_awaits something, the program jumps back to the calling function, which continues executing.
 * At this point, if the function returns, every object declared in the function including its parameters are destroyed, which causes @ref lambdas-and-locals "dangling references".
 * For this reason, `co_await` will error if any parameters are passed by reference.
 * If you must pass a reference, pass it as a pointer or with std::ref, but you must fully understand the reason behind this warning, and what to avoid.
 * If you prefer a safer type, use `coroutine` for synchronous execution, or `task` for parallel tasks, and co_await them.
 */
SHION_EXPORT struct job {};

namespace detail {

namespace job {

#ifdef SHION_CORO_TEST
	struct promise{};
#endif

/**
 * @brief Coroutine promise type for a job
 */
template <typename... Args>
struct promise {

#ifdef SHION_CORO_TEST
	promise() {
		++coro_alloc_count<job_promise_base>;
	}

	~promise() {
		--coro_alloc_count<job_promise_base>;
	}
#endif

	/**
	 * @brief Function called when the job is done.
	 *
	 * @return <a href="https://en.cppreference.com/w/cpp/coroutine/suspend_never">std::suspend_never</a> Do not suspend at the end, destroying the handle immediately
	 */
	std_coroutine::suspend_never final_suspend() const noexcept {
		return {};
	}

	/**
	 * @brief Function called when the job is started.
	 *
	 * @return <a href="https://en.cppreference.com/w/cpp/coroutine/suspend_never">std::suspend_never</a> Do not suspend at the start, starting the job immediately
	 */
	std_coroutine::suspend_never initial_suspend() const noexcept {
		return {};
	}

	/**
	 * @brief Function called to get the job object
	 *
	 * @return job
	 */
	shion::job get_return_object() const noexcept {
		return {};
	}

	/**
	 * @brief Function called when an exception is thrown and not caught.
	 *
	 * @throw Immediately rethrows the exception to the caller / resumer
	 */
	void unhandled_exception() const {
		throw;
	}

	/**
	 * @brief Function called when the job returns. Does nothing.
	 */
	void return_void() const noexcept {}
};

} // namespace job

namespace promise {

template <typename T>
void spawn_sync_wait_job(auto* awaitable, std::condition_variable &cv, auto&& result) {
	[](auto* awaitable_, std::condition_variable &cv_, auto&& result_) -> ::shion::job {
		try {
			if constexpr (std::is_void_v<T>) {
				co_await *awaitable_;
				result_.template emplace<1>();
			} else {
				result_.template emplace<1>(co_await *awaitable_);
			}
		} catch (...) {
			result_.template emplace<2>(std::current_exception());
		}
		cv_.notify_all();
	}(awaitable, cv, std::forward<decltype(result)>(result));
}

}

} // namespace detail

static_assert(is_placeholder_for<job, job_dummy>);

} // namespace shion

/**
 * @brief Specialization of std::coroutine_traits, helps the standard library figure out a promise type from a coroutine function.
 */
SHION_EXPORT template<typename... Args>
struct std::coroutine_traits<shion::job, Args...> {
	/**
	 * @brief Promise type for this coroutine signature.
	 *
	 * When the coroutine is created from a lambda, that lambda is passed as a first parameter.
	 * Not ideal but we'll allow any callable that takes the rest of the arguments passed
	 */
	using promise_type = shion::detail::job::promise<Args...>;
};
