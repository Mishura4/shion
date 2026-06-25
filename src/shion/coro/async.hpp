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

namespace detail
{

template <typename Reference, typename Value>
using async_value_t = std::conditional_t<std::is_void_v<Value>, Reference, Value>;

template <typename Value>
using async_state = detail::coro::promise_state<detail::coro::atomic_continuation_controller, Value, void>;

template <typename Value>
using async_state_ptr = std::shared_ptr<async_state<Value>>;

template <typename Value>
using async_state_accessor = coro::promise_state_accessor<async_state_ptr<Value>>;

template <typename Value>
struct async_callback : async_state_accessor<Value>
{
	using async_state_accessor<Value>::async_state_accessor;
	using signature_t = void(Value);
	
	async_callback(async_state_accessor<Value> accessor) : async_state_accessor<Value>(std::move(accessor))
	{}

	template <std::convertible_to<Value> Arg>
	void operator()(Arg&& arg) noexcept(std::is_nothrow_convertible_v<Arg, Value>)
	{
		this->get_promise_state().emplace_value(std::forward<Arg>(arg));
	}
};

template <>
struct async_callback<void> : async_state_accessor<void>
{
	using async_state_accessor<void>::async_state_accessor;
	using signature_t = void();
	
	async_callback(async_state_accessor<void> accessor) : async_state_accessor<void>(std::move(accessor))
	{}

	void operator()() noexcept
	{
		this->get_promise_state().emplace_value();
	}
};

}

/**
 * @class async async.h coro/async.h
 * @brief A co_await-able object handling an async call in parallel with the caller.
 */
template <typename Reference, typename Value>
class async : public basic_awaitable<
	Reference, detail::async_state_ptr<detail::async_value_t<Reference, Value>>
>
{
	using value_t = detail::async_value_t<Reference, Value>;
	using state_type = detail::async_state<value_t>;
	using state_ptr = detail::async_state_ptr<value_t>;
	using state_accessor = detail::async_state_accessor<value_t>;
	using base = basic_awaitable<Reference, state_ptr>;
	using callback = detail::async_callback<value_t>;
	using callback_signature = callback::signature_t;
	
	template <typename Fun, typename... Args>
	auto _invoke(Fun&& fun, Args&&... args) -> decltype(auto)
	{
		return std::invoke(
			std::forward<Fun>(fun),
			std::forward<Args>(args)...,
			callback(*static_cast<state_accessor*>(this))
		);
	}

public:
	async() = default;
	async(const async&) = delete;
	async(async&&) = default;
	~async() = default;
	
	auto operator=(const async&) -> async& = delete;
	auto operator=(async&&) -> async& = default;
	
	/**
	 * @brief Construct an async object wrapping an object method, the call is made immediately by forwarding to <a href="https://en.cppreference.com/w/cpp/utility/functional/invoke">std::invoke</a> and can be awaited later to retrieve the result.
	 *
	 * @param obj The object to call the method on
	 * @param fun The method of the object to call. Its last parameter must be a callback taking a parameter of type R
	 * @param args Parameters to pass to the method, excluding the callback
	 */
	template <typename Obj, typename Fun, typename... Args>
#ifndef _DOXYGEN_
	requires std::invocable<Fun, Obj, Args..., callback_signature>
#endif
	explicit async(Obj &&obj, Fun &&fun, Args&&... args) : async{ std::make_shared<state_type>() } {
		this->_invoke(std::forward<Fun>(fun), std::forward<Obj>(obj), std::forward<Args>(args)...);
	}

	/**
	 * @brief Construct an async object wrapping an invokeable object, the call is made immediately by forwarding to <a href="https://en.cppreference.com/w/cpp/utility/functional/invoke">std::invoke</a> and can be awaited later to retrieve the result.
	 *
	 * @param fun The object to call using <a href="https://en.cppreference.com/w/cpp/utility/functional/invoke">std::invoke</a>. Its last parameter must be a callable taking a parameter of type R
	 * @param args Parameters to pass to the object, excluding the callback
	 */
	template <typename Fun, typename... Args>
#ifndef _DOXYGEN_
	requires std::invocable<Fun, Args..., callback_signature>
#endif
	explicit async(Fun &&fun, Args&&... args) : async{ std::make_shared<state_type>() } {
		this->_invoke(std::forward<Fun>(fun), std::forward<Args>(args)...);
	}
	
private:
	explicit async(state_ptr &&promise) :
		base{ state_accessor{ std::move(promise) } }
	{
	}
};

static_assert(is_placeholder_for<async<>, async_dummy>);

} // namespace shion
