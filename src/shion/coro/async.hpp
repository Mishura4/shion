#pragma once

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#include "coro.hpp"
#include "awaitable.hpp"

#include <utility>
#include <type_traits>
#include <functional>
#include <atomic>
#include <cstddef>
#endif

namespace SHION_NAMESPACE {

SHION_EXPORT struct async_dummy : awaitable_dummy {
	std::shared_ptr<int> dummy_shared_state = nullptr;
};

namespace detail {

namespace async {

/**
 * @brief Shared state of the async and its callback, to be used across threads.
 */
template <typename R>
struct callback {
	std::shared_ptr<basic_promise<R>> promise{nullptr};

	void operator()(const R& v) const {
		promise->set_value(v);
	}

	void operator()(R&& v) const {
		promise->set_value(std::move(v));
	}
};

template <>
struct callback<void> {
	std::shared_ptr<basic_promise<void>> promise{nullptr};

	void operator()() const {
		promise->set_value();
	}
};

} // namespace async

} // namespace detail

/**
 * @class async async.h coro/async.h
 * @brief A co_await-able object handling an async call in parallel with the caller.
 */
SHION_EXPORT template <typename R>
class async : public awaitable<R> {

	detail::async::callback<R> api_callback{};

	explicit async(std::shared_ptr<basic_promise<R>> &&promise) : awaitable<R>{promise.get()}, api_callback{std::move(promise)} {}

public:
	using awaitable<R>::awaitable; // use awaitable's constructors
	using awaitable<R>::operator=; // use async_base's assignment operator
	using awaitable<R>::await_ready; // expose await_ready as public

	/**
	 * @brief Construct an async object wrapping an object method, the call is made immediately by forwarding to <a href="https://en.cppreference.com/w/cpp/utility/functional/invoke">std::invoke</a> and can be awaited later to retrieve the result.
	 *
	 * @param obj The object to call the method on
	 * @param fun The method of the object to call. Its last parameter must be a callback taking a parameter of type R
	 * @param args Parameters to pass to the method, excluding the callback
	 */
	template <typename Obj, typename Fun, typename... Args>
#ifndef _DOXYGEN_
	requires std::invocable<Fun, Obj, Args..., std::function<void(R)>>
#endif
	explicit async(Obj &&obj, Fun &&fun, Args&&... args) : async{std::make_shared<basic_promise<R>>()} {
		std::invoke(std::forward<Fun>(fun), std::forward<Obj>(obj), std::forward<Args>(args)..., api_callback);
	}

	/**
	 * @brief Construct an async object wrapping an invokeable object, the call is made immediately by forwarding to <a href="https://en.cppreference.com/w/cpp/utility/functional/invoke">std::invoke</a> and can be awaited later to retrieve the result.
	 *
	 * @param fun The object to call using <a href="https://en.cppreference.com/w/cpp/utility/functional/invoke">std::invoke</a>. Its last parameter must be a callable taking a parameter of type R
	 * @param args Parameters to pass to the object, excluding the callback
	 */
	template <typename Fun, typename... Args>
#ifndef _DOXYGEN_
	requires std::invocable<Fun, Args..., std::function<void(R)>>
#endif
	explicit async(Fun &&fun, Args&&... args) : async{std::make_shared<basic_promise<R>>()} {
		std::invoke(std::forward<Fun>(fun), std::forward<Args>(args)..., api_callback);
	}
};

static_assert(is_placeholder_for<async<>, async_dummy>);

} // namespace shion
