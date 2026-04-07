#pragma once

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#  include <shion/common.hpp>
#endif

namespace SHION_NAMESPACE {

/**
 * @brief Implementation details for internal use only.
 *
 * @attention This is only meant to be used by the library internally. Support will not be given regarding the facilities in this namespace.
 */
namespace detail
{
#ifdef _DOXYGEN_
/**
 * @brief Alias for either std or std::experimental depending on compiler and library. Used by coroutine implementation.
 *
 * @todo Remove and use std when all supported libraries have coroutines in it
 */
namespace std_coroutine {}
#else
#  ifdef STDCORO_EXPERIMENTAL_NAMESPACE
namespace std_coroutine = std::experimental;
#  else
namespace std_coroutine = std;
#  endif
#endif

template <typename T = void>
using coro_handle = std_coroutine::coroutine_handle<T>;

#ifndef _DOXYGEN_
/**
 * @brief Concept to check if a type has a useable `operator co_await()` member
 */
template <typename T>
concept has_co_await_member = requires (T expr) { expr.operator co_await(); };

/**
 * @brief Concept to check if a type has a useable overload of the free function `operator co_await(expr)`
 */
template <typename T>
concept has_free_co_await = requires (T expr) { operator co_await(expr); };

/**
 * @brief Concept to check if a type has useable `await_ready()`, `await_suspend()` and `await_resume()` member functions.
 */
template <typename T>
concept has_await_members = requires (T expr, std::coroutine_handle<> handle) { expr.await_ready(); expr.await_suspend(handle); expr.await_resume(); };

/**
 * @brief Mimics the compiler's behavior of using co_await. That is, it returns whichever works first, in order : `expr.operator co_await();` > `operator co_await(expr)` > `expr`
 */
template <typename T>
requires (has_co_await_member<T>)
decltype(auto) co_await_resolve(T&& expr) noexcept(noexcept(expr.operator co_await())) {
	decltype(auto) awaiter = expr.operator co_await();
	return awaiter;
}

/**
 * @brief Mimics the compiler's behavior of using co_await. That is, it returns whichever works first, in order : `expr.operator co_await();` > `operator co_await(expr)` > `expr`
 */
template <typename T>
requires (!has_co_await_member<T> && has_free_co_await<T>)
decltype(auto) co_await_resolve(T&& expr) noexcept(noexcept(operator co_await(expr))) {
	decltype(auto) awaiter = operator co_await(static_cast<T&&>(expr));
	return awaiter;
}

/**
 * @brief Mimics the compiler's behavior of using co_await. That is, it returns whichever works first, in order : `expr.operator co_await();` > `operator co_await(expr)` > `expr`
 */
template <typename T>
requires (!has_co_await_member<T> && !has_free_co_await<T>)
decltype(auto) co_await_resolve(T&& expr) noexcept {
	return static_cast<T&&>(expr);
}

#else
/**
 * @brief Concept to check if a type has a useable `operator co_await()` member
 *
 * @note This is actually a C++20 concept but Doxygen doesn't do well with them
 */
template <typename T>
bool has_co_await_member;

/**
 * @brief Concept to check if a type has a useable overload of the free function `operator co_await(expr)`
 *
 * @note This is actually a C++20 concept but Doxygen doesn't do well with them
 */
template <typename T>
bool has_free_co_await;

/**
 * @brief Concept to check if a type has useable `await_ready()`, `await_suspend()` and `await_resume()` member functions.
 *
 * @note This is actually a C++20 concept but Doxygen doesn't do well with them
 */
template <typename T>
bool has_await_members;

/**
 * @brief Mimics the compiler's behavior of using co_await. That is, it returns whichever works first, in order : `expr.operator co_await();` > `operator co_await(expr)` > `expr`
 *
 * This function is conditionally noexcept, if the returned expression also is.
 */
decltype(auto) co_await_resolve(auto&& expr) {}
#endif

/**
 * @brief Convenience alias for the result of a certain awaitable's await_resume.
 */
template <typename T>
using awaitable_result = decltype(co_await_resolve(std::declval<T>()).await_resume());

} // namespace detail

SHION_EXPORT template <typename Reference = void, typename Value = void>
class async;

SHION_EXPORT template <typename Reference = void, typename Value = void>
class task;

SHION_EXPORT template <typename Reference = void, typename Value = void>
class coroutine;

SHION_EXPORT struct job;

SHION_EXPORT template <typename T>
concept awaitable_type = requires (T t) { detail::co_await_resolve(static_cast<T&&>(t)); };

namespace detail::coro
{

template <typename StateHolder>
class awaitable_impl;

}

inline namespace coro
{

SHION_EXPORT template <typename Reference, typename Value = void>
class single_promise;

SHION_EXPORT template <typename Reference, typename Value = void>
class async_single_promise;

}

/**
 * @brief Generic awaitable class, represents a future value that can be co_await-ed on.
 *
 * Roughly equivalent of std::future for coroutines, with the crucial distinction that the future does not own a reference to a "shared state".
 * It holds a non-owning reference to the promise, which must be kept alive for the entire lifetime of the awaitable.
 *
 * @tparam T Type of the asynchronous value
 * @see promise
*/
template <typename Reference, typename Value = void>
class basic_awaitable;

/**
 * @brief Generic awaitable class, represents a future value that can be co_await-ed on.
 *
 * Roughly equivalent of std::future for coroutines, with the crucial distinction that the future does not own a reference to a "shared state".
 * It holds a non-owning reference to the promise, which must be kept alive for the entire lifetime of the awaitable.
 *
 * @tparam T Type of the asynchronous value
 * @see promise
*/
template <typename Reference, typename Value = void>
class async_awaitable;

template <typename Reference, typename Value = void>
using awaitable = async_awaitable<Reference, Value>;

#ifdef SHION_CORO_TEST
/**
 * @brief Allocation count of a certain type, for testing purposes.
 *
 * @todo Remove when coro is stable
 */
template <typename T>
inline int coro_alloc_count = 0;
#endif

} // namespace shion
