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

SHION_EXPORT struct async_dummy
{
	std::shared_ptr<int> dummy_shared_state = nullptr;
};

namespace detail {

namespace async {

/**
 * @brief Shared state of the async and its callback, to be used across threads.
 */
template <typename R>
struct callback {
	std::shared_ptr<async_single_promise<R>> promise{nullptr};

	void operator()(const R& v) const {
		promise->set_value(v);
	}

	void operator()(R&& v) const {
		promise->set_value(std::move(v));
	}

	friend auto get_promise(const callback& cb) -> async_single_promise<R>&
	{
		return *cb.promise;
	}
};

template <>
struct callback<void>  {
	using state_type = async_promise<void>;

	std::shared_ptr<state_type> promise{nullptr};

	bool valid() const noexcept
	{
		return promise != nullptr;
	}

	void operator()() const {
		promise->set_value();
	}

	auto get_promise() -> state_type&
	{
		return *promise;
	}

	void release() noexcept
	{
		promise = {};
	}
};

} // namespace async

} // namespace detail

/**
 * @class async async.h coro/async.h
 * @brief A co_await-able object handling an async call in parallel with the caller.
 */
template <typename Reference, typename Value>
class async : public detail::coro::awaitable_impl<detail::async::callback<Reference>>
{
	using base = detail::coro::awaitable_impl<detail::async::callback<Reference>>;

	explicit async(std::shared_ptr<coro::single_promise<Reference>> &&promise) :
		base{ detail::async::callback<Reference>(std::move(promise)) }
	{
	}

public:
	using base::base; // use awaitable's constructors
	using base::operator=; // use async_base's assignment operator
	using base::await_ready; // expose await_ready as public
	using base::await_suspend; // expose await_suspend as public
	using base::await_resume; // expose await_resume as public

	/**
	 * @brief Construct an async object wrapping an object method, the call is made immediately by forwarding to <a href="https://en.cppreference.com/w/cpp/utility/functional/invoke">std::invoke</a> and can be awaited later to retrieve the result.
	 *
	 * @param obj The object to call the method on
	 * @param fun The method of the object to call. Its last parameter must be a callback taking a parameter of type R
	 * @param args Parameters to pass to the method, excluding the callback
	 */
	template <typename Obj, typename Fun, typename... Args>
#ifndef _DOXYGEN_
	requires std::invocable<Fun, Obj, Args..., std::function<void(Reference)>>
#endif
	explicit async(Obj &&obj, Fun &&fun, Args&&... args) : async{std::make_shared<coro::single_promise<Reference>>()} {
		std::invoke(std::forward<Fun>(fun), std::forward<Obj>(obj), std::forward<Args>(args)..., base::state_ptr);
	}

	/**
	 * @brief Construct an async object wrapping an invokeable object, the call is made immediately by forwarding to <a href="https://en.cppreference.com/w/cpp/utility/functional/invoke">std::invoke</a> and can be awaited later to retrieve the result.
	 *
	 * @param fun The object to call using <a href="https://en.cppreference.com/w/cpp/utility/functional/invoke">std::invoke</a>. Its last parameter must be a callable taking a parameter of type R
	 * @param args Parameters to pass to the object, excluding the callback
	 */
	template <typename Fun, typename... Args>
#ifndef _DOXYGEN_
	requires std::invocable<Fun, Args..., std::function<void(Reference)>>
#endif
	explicit async(Fun &&fun, Args&&... args) : async{std::make_shared<coro::single_promise<Reference>>()} {
		std::invoke(std::forward<Fun>(fun), std::forward<Args>(args)..., base::state_ptr);
	}
};

inline constexpr auto foo = sizeof(async<>);

static_assert(is_placeholder_for<async<>, async_dummy>);

} // namespace shion
