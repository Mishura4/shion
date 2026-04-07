#pragma once

#include <shion/common/defines.hpp>

#if !SHION_IMPORT_STD
#    include <iostream>
#    include <mutex>
#    include <utility>
#    include <type_traits>
#    include <functional>
#    include <atomic>
#    include <cstddef>
#    include <variant>
#    include <optional>
#    include <exception>
#    include <condition_variable>
#endif

#if !SHION_BUILDING_MODULES
#  include <shion/coro/coro.hpp>
#  include <shion/coro/promise.hpp>
#  include <shion/common.hpp>
#endif

namespace SHION_NAMESPACE {

SHION_EXPORT struct awaitable_dummy
{
	int *promise_dummy = nullptr;
};

#if false

/**
 * @brief Base class for a promise type.
 *
 * Contains the base logic for @ref promise, but does not contain the set_value methods.
 */
SHION_EXPORT template <typename T>
class moveable_promise {
	std::unique_ptr<async_single_promise<T>> shared_state = std::make_unique<async_single_promise<T>>();

public:
	/**
	 * @copydoc basic_promise<T>::emplace_value
	 */
	template <bool Notify = true, typename... Args>
	requires (std::constructible_from<T, Args...>)
	constexpr void emplace_value(Args&&... args) {
		shared_state->template emplace_value<Notify>(std::forward<Args>(args)...);
	}

	/**
	 * @copydoc basic_promise<T>::set_value(const T&)
	 */
	template <bool Notify = true>
	constexpr void set_value(const T& v) requires (std::copy_constructible<T>) {
		shared_state->template set_value<Notify>(v);
	}

	/**
	 * @copydoc basic_promise<T>::set_value(T&&)
	 */
	template <bool Notify = true>
	constexpr void set_value(T&& v) requires (std::move_constructible<T>) {
		shared_state->template set_value<Notify>(std::move(v));
	}

	/**
	 * @copydoc basic_promise<T>::set_value(T&&)
	 */
	template <bool Notify = true>
	constexpr void set_exception(std::exception_ptr ptr) {
		shared_state->template set_exception<Notify>(std::move(ptr));
	}

	/**
	 * @copydoc basic_promise<T>::get_awaitable
	 */
	constexpr auto get_awaitable() {
		return shared_state->get_awaitable();
	}
};

template <>
class moveable_promise<void> {
	std::unique_ptr<async_single_promise<void>> shared_state = std::make_unique<async_single_promise<void>>();

public:
	/**
	 * @copydoc basic_promise<void>::set_value
	 */
	template <bool Notify = true>
	constexpr void set_value() {
		shared_state->set_value<Notify>();
	}

	/**
	 * @copydoc basic_promise<T>::set_exception
	 */
	template <bool Notify = true>
	constexpr void set_exception(std::exception_ptr ptr) {
		shared_state->set_exception<Notify>(std::move(ptr));
	}

	/**
	 * @copydoc basic_promise<T>::get_awaitable
	 */
	constexpr auto get_awaitable()
	{
		return shared_state->get_awaitable();
	}
};

#endif

}
