#pragma once

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#include <shion/common.hpp>

#include "coro.hpp"
#include "awaitable.hpp"

#include <optional>
#include <type_traits>
#include <exception>
#include <utility>
#include <type_traits>
#endif

namespace SHION_NAMESPACE {

SHION_EXPORT struct coroutine_dummy {
	int *handle_dummy = nullptr;
};

namespace detail {

namespace coroutine {

template <typename R>
struct promise_t;

template <typename R>
/**
 * @brief Alias for the handle_t of a coroutine.
 */
using handle_t = std_coroutine::coroutine_handle<promise_t<R>>;

template <typename R>
promise_t<R> get_promise(const handle_t<R>& handle) noexcept
{
	return handle->promise();
}

template <typename R, typename Awaitable>
void abandon_promise(handle_t<R>& handle, Awaitable*) noexcept
{
	handle.destroy();
}

} // namespace coroutine

} // namespace detail

/**
 * @class coroutine coroutine.h coro/coroutine.h
 * @brief Base type for a coroutine, starts on co_await.
 *
 * @tparam R Return type of the coroutine. Can be void, or a complete object that supports move construction and move assignment.
 */
template <typename Reference, typename Value>
class coroutine : public detail::coro::awaitable<detail::coroutine::handle_t<Reference>>
{
	using base = detail::coro::awaitable<detail::coroutine::handle_t<Reference>>;

	/**
	 * @brief Promise has friend access for the constructor
	 */
	friend struct detail::coroutine::promise_t<Reference>;

	/**
	 * @brief Construct from a handle. Internal use only.
	 */
	coroutine(detail::coroutine::handle_t<Reference> h) : base{  std::move(h) } {}

public:
	using base::base;
	using base::operator=;
};

namespace detail::coroutine
{
	template <typename R>
	struct final_awaiter;

#ifdef SHION_CORO_TEST
	struct promise_t_base{};
#endif

	/**
	 * @brief Promise type for coroutine.
	 */
	template <typename R>
	struct promise_t : single_promise<R>
	{
#ifdef SHION_CORO_TEST
		promise_t() {
			++coro_alloc_count<promise_t_base>;
		}

		~promise_t() {
			--coro_alloc_count<promise_t_base>;
		}
#endif

		/**
		 * @brief Function called by the standard library when reaching the end of a coroutine
		 *
		 * @return final_awaiter<R> Resumes any coroutine co_await-ing on this
		 */
		[[nodiscard]] final_awaiter<R> final_suspend() const noexcept;

		/**
		 * @brief Function called by the standard library when the coroutine start
		 *
		 * @return @return <a href="https://en.cppreference.com/w/cpp/coroutine/suspend_always">std::suspend_always</a> Always suspend at the start, for a lazy start
		 */
		[[nodiscard]] std_coroutine::suspend_always initial_suspend() const noexcept {
			return {};
		}

		/**
		 * @brief Function called when an exception escapes the coroutine
		 *
		 * Stores the exception to throw to the co_await-er
		 */
		void unhandled_exception() noexcept {
			this->set_exception(std::current_exception());
		}

		/**
		 * @brief Function called by the standard library when the coroutine co_returns a value.
		 *
		 * Stores the value internally to hand to the caller when it resumes.
		 *
		 * @param expr The value given to co_return
		 */
		void return_value(R&& expr) noexcept(std::is_nothrow_move_constructible_v<R>) requires std::move_constructible<R> {
			this->set_value(static_cast<R&&>(expr));
		}

		/**
		 * @brief Function called by the standard library when the coroutine co_returns a value.
		 *
		 * Stores the value internally to hand to the caller when it resumes.
		 *
		 * @param expr The value given to co_return
		 */
		void return_value(const R &expr) noexcept(std::is_nothrow_copy_constructible_v<R>) requires std::copy_constructible<R> {
			this->set_value(expr);
		}

		/**
		 * @brief Function called by the standard library when the coroutine co_returns a value.
		 *
		 * Stores the value internally to hand to the caller when it resumes.
		 *
		 * @param expr The value given to co_return
		 */
		template <typename T>
		requires (!std::is_same_v<R, std::remove_cvref_t<T>> && std::convertible_to<T, R>)
		void return_value(T&& expr) noexcept (std::is_nothrow_convertible_v<T, R>) {
			this->set_value(std::forward<T>(expr));
		}

		/**
		 * @brief Function called to get the coroutine object
		 */
		shion::coroutine<R> get_return_object() {
			return shion::coroutine<R>{handle_t<R>::from_promise(*this)};
		}
	};

	/**
	 * @brief Struct returned by a coroutine's final_suspend, resumes the continuation
	 */
	template <typename R>
	struct final_awaiter {
		/**
		 * @brief First function called by the standard library when reaching the end of a coroutine
		 *
		 * @return false Always return false, we need to suspend to resume the parent
		 */
		[[nodiscard]] bool await_ready() const noexcept {
			return false;
		}

		/**
		 * @brief Second function called by the standard library when reaching the end of a coroutine.
		 *
		 * @return std::handle_t<> Coroutine handle to resume, this is either the parent if present or std::noop_coroutine()
		 */
		[[nodiscard]] std_coroutine::coroutine_handle<> await_suspend(std_coroutine::coroutine_handle<promise_t<R>> handle) const noexcept {
			auto parent = handle.promise().awaiter;

			return parent ? parent : std_coroutine::noop_coroutine();
		}

		/**
		 * @brief Function called by the standard library when this object is resumed
		 */
		void await_resume() const noexcept {}
	};

	template <typename R>
	final_awaiter<R> promise_t<R>::final_suspend() const noexcept {
		return {};
	}

	/**
	 * @brief Struct returned by a coroutine's final_suspend, resumes the continuation
	 */
	template <>
	struct promise_t<void> : simple_promise<void> {
		/**
		 * @brief Function called by the standard library when reaching the end of a coroutine
		 *
		 * @return final_awaiter<R> Resumes any coroutine co_await-ing on this
		 */
		[[nodiscard]] final_awaiter<void> final_suspend() const noexcept {
			return {};
		}

		/**
		 * @brief Function called by the standard library when the coroutine start
		 *
		 * @return @return <a href="https://en.cppreference.com/w/cpp/coroutine/suspend_always">std::suspend_always</a> Always suspend at the start, for a lazy start
		 */
		[[nodiscard]] std_coroutine::suspend_always initial_suspend() const noexcept {
			return {};
		}

		/**
		 * @brief Function called when an exception escapes the coroutine
		 *
		 * Stores the exception to throw to the co_await-er
		 */
		void unhandled_exception() noexcept
		{
			set_exception(std::current_exception());
		}

		/**
		 * @brief Function called when co_return is used
		 */
		void return_void() noexcept
		{
			set_value();
		}

		/**
		 * @brief Function called to get the coroutine object
		 */
		[[nodiscard]] shion::coroutine<void> get_return_object()
		{
			return shion::coroutine<void>{ handle_t<void>::from_promise(*this) };
		}
	};

} // namespace detail

} // namespace shion

/**
 * @brief Specialization of std::coroutine_traits, helps the standard library figure out a promise type from a coroutine function.
 */
SHION_EXPORT template<typename R, typename... Args>
struct std::coroutine_traits<shion::coroutine<R>, Args...> {
	using promise_type = shion::detail::coroutine::promise_t<R>;
};
