#pragma once

#include "../shion_essentials.hpp"

namespace shion {

struct coroutine_dummy {
	int *handle_dummy = nullptr;
};

}

#include "coro.hpp"
#include "awaitable.hpp"

#include <optional>
#include <type_traits>
#include <exception>
#include <utility>
#include <type_traits>

namespace shion {

namespace detail {

namespace coroutine {

	template <typename R>
struct promise_t;

template <typename R>
/**
 * @brief Alias for the handle_t of a coroutine.
 */
using handle_t = std_coroutine::coroutine_handle<promise_t<R>>;

} // namespace coroutine

} // namespace detail

/**
 * @class coroutine coroutine.h coro/coroutine.h
 * @brief Base type for a coroutine, starts on co_await.
 *
 * @tparam R Return type of the coroutine. Can be void, or a complete object that supports move construction and move assignment.
 */
template <typename R>
class coroutine : public basic_awaitable<coroutine<R>> {
	/**
	 * @brief Promise has friend access for the constructor
	 */
	friend struct detail::coroutine::promise_t<R>;

	/**
	 * @brief Coroutine handle.
	 */
	detail::coroutine::handle_t<R> handle{nullptr};

	/**
	 * @brief Construct from a handle. Internal use only.
	 */
	coroutine(detail::coroutine::handle_t<R> h) : handle{h} {}

	struct awaiter {
		coroutine &coro;

		/**
		 * @brief First function called by the standard library when the coroutine is co_await-ed.
		 *
		 * @remark Do not call this manually, use the co_await keyword instead.
		 * @throws invalid_operation_exception if the coroutine is empty or finished.
		 * @return bool Whether the coroutine is done
		 */
		[[nodiscard]] bool await_ready() const {
			if (!coro.handle) {
				throw shion::logic_exception("cannot co_await an empty coroutine");
			}
			return coro.handle.done();
		}

		/**
		 * @brief Second function called by the standard library when the coroutine is co_await-ed.
		 *
		 * Stores the calling coroutine in the promise to resume when this coroutine suspends.
		 *
		 * @remark Do not call this manually, use the co_await keyword instead.
		 * @param caller The calling coroutine, now suspended
		 */
		template <typename T>
		[[nodiscard]] detail::coroutine::handle_t<R> await_suspend(detail::std_coroutine::coroutine_handle<T> caller) noexcept {
			coro.handle.promise().parent = caller;
			return coro.handle;
		}

		R await_resume() {
			detail::coroutine::promise_t<R> &promise = coro.handle.promise();
			if (promise.exception) {
				std::rethrow_exception(promise.exception);
			}
			if constexpr (!std::is_void_v<R>) {
				return *std::exchange(promise.result, std::nullopt);
			} else {
				return; // unnecessary but makes lsp happy
			}
		}
	};

public:
	/**
	 * @brief Default constructor, creates an empty coroutine.
	 */
	coroutine() = default;

	/**
	 * @brief Copy constructor is disabled
	 */
	coroutine(const coroutine &) = delete;

	/**
	 * @brief Move constructor, grabs another coroutine's handle
	 *
	 * @param other Coroutine to move the handle from
	 */
	coroutine(coroutine &&other) noexcept : handle(std::exchange(other.handle, nullptr)) {}

	/**
	 * @brief Destructor, destroys the handle.
	 */
	~coroutine() {
		if (handle) {
			handle.destroy();
		}
	}

	/**
	 * @brief Copy assignment is disabled
	 */
	coroutine &operator=(const coroutine &) = delete;

	/**
	 * @brief Move assignment, grabs another coroutine's handle
	 *
	 * @param other Coroutine to move the handle from
	 */
	coroutine &operator=(coroutine &&other) noexcept {
		handle = std::exchange(other.handle, nullptr);
		return *this;
	}
	
	[[nodiscard]] auto operator co_await() {
		return awaiter{*this};
	}
};

namespace detail::coroutine {
	template <typename R>
	struct final_awaiter;

#ifdef SHION_CORO_TEST
	struct promise_t_base{};
#endif

	/**
	 * @brief Promise type for coroutine.
	 */
	template <typename R>
	struct promise_t {
		/**
		 * @brief Handle of the coroutine co_await-ing this coroutine.
		 */
		std_coroutine::coroutine_handle<> parent{nullptr};

		/**
		 * @brief Return value of the coroutine
		 */
		std::optional<R> result{};

		/**
		 * @brief Pointer to an uncaught exception thrown by the coroutine
		 */
		std::exception_ptr exception{nullptr};

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
			exception = std::current_exception();
		}

		/**
		 * @brief Function called by the standard library when the coroutine co_returns a value.
		 *
		 * Stores the value internally to hand to the caller when it resumes.
		 *
		 * @param expr The value given to co_return
		 */
		void return_value(R&& expr) noexcept(std::is_nothrow_move_constructible_v<R>) requires std::move_constructible<R> {
			result = static_cast<R&&>(expr);
		}

		/**
		 * @brief Function called by the standard library when the coroutine co_returns a value.
		 *
		 * Stores the value internally to hand to the caller when it resumes.
		 *
		 * @param expr The value given to co_return
		 */
		void return_value(const R &expr) noexcept(std::is_nothrow_copy_constructible_v<R>) requires std::copy_constructible<R> {
			result = expr;
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
			result = std::forward<T>(expr);
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
			auto parent = handle.promise().parent;

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
	struct promise_t<void> {
		/**
		 * @brief Handle of the coroutine co_await-ing this coroutine.
		 */
		std_coroutine::coroutine_handle<> parent{nullptr};

		/**
		 * @brief Pointer to an uncaught exception thrown by the coroutine
		 */
		std::exception_ptr exception{nullptr};

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
		void unhandled_exception() noexcept {
			exception = std::current_exception();
		}

		/**
		 * @brief Function called when co_return is used
		 */
		void return_void() const noexcept {}

		/**
		 * @brief Function called to get the coroutine object
		 */
		[[nodiscard]] shion::coroutine<void> get_return_object() {
			return shion::coroutine<void>{handle_t<void>::from_promise(*this)};
		}
	};

} // namespace detail

static_assert(detail::is_abi_compatible<coroutine<void>, coroutine_dummy>);
static_assert(detail::is_abi_compatible<coroutine<int>, coroutine_dummy>);

} // namespace shion

/**
 * @brief Specialization of std::coroutine_traits, helps the standard library figure out a promise type from a coroutine function.
 */
template<typename R, typename... Args>
struct shion::detail::std_coroutine::coroutine_traits<shion::coroutine<R>, Args...> {
	using promise_type = shion::detail::coroutine::promise_t<R>;
};
