#pragma once

#include <shion/common/defines.hpp>

#include "awaitable.hpp"

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

namespace detail::coro
{

struct awaitable_dummy
{
	int *promise_dummy = nullptr;
};

}

namespace detail::coro
{

template <typename T, bool = requires { requires std::is_void<typename T::reference>::value; }>
struct promise_reference_helper
{
	using type = typename T::reference;
};

template <typename T>
struct promise_reference_helper<T, false>
{
	using type = std::add_lvalue_reference_t<typename T::value_type>;
};

template <typename T, bool = requires { requires std::is_void<typename T::rvalue_reference>::value; }>
struct promise_rvalue_reference_helper
{
	using type = typename T::rvalue_reference;
};

template <typename T>
struct promise_rvalue_reference_helper<T, false>
{
	using type = std::add_rvalue_reference_t<typename T::value_type>;
};

template <typename T, bool = requires { requires std::is_void<typename T::const_reference>::value; }>
struct promise_const_reference_helper
{
	using type = typename T::const_reference;
};

template <typename T>
struct promise_const_reference_helper<T, false>
{
	using type = std::add_lvalue_reference_t<std::add_const_t<typename T::value_type>>;
};

template <typename T, bool = requires { requires std::is_void<typename T::const_rvalue_reference>::value; }>
struct promise_const_rvalue_reference_helper
{
	using type = typename T::const_rvalue_reference;
};

template <typename T>
struct promise_const_rvalue_reference_helper<T, false>
{
	using type = std::add_rvalue_reference_t<typename T::value_type>;
};

}

namespace coro
{

SHION_EXPORT template <typename T>
struct promise_traits;

template <typename>
struct promise_traits {};

template <typename T>
requires (!std::is_void<typename T::value_type>::value)
struct promise_traits<T>
{
	using value_type = std::is_void<typename T::value_type>::value;
	using reference = detail::coro::promise_reference_helper<T>::type;
	using const_reference = detail::coro::promise_const_reference_helper<T>::type;
	using rvalue_reference = detail::coro::promise_rvalue_reference_helper<T>::type;
	using const_rvalue_reference = detail::coro::promise_const_rvalue_reference_helper<T>::type;
};

template <typename Ref, typename PromiseAccessor>
class basic_awaitable : protected detail::coro::promise_state_accessor<PromiseAccessor>
{
	template <typename T>
	using internal_reference = decltype(std::declval<T>().get_promise_state().get_value());
	using state_holder = std::remove_cvref_t<PromiseAccessor>;
	using state_accessor = detail::coro::promise_state_accessor<PromiseAccessor>;
	using promise_state = typename state_accessor::promise_state;
	
public:
	using promise_type = state_holder;
	using value_type = std::remove_cvref_t<Ref>;
	using reference = std::conditional_t<
		std::is_reference_v<Ref>, Ref, std::add_lvalue_reference_t<Ref>
	>;
	using const_reference = std::conditional_t<
		std::is_reference_v<Ref>, Ref, std::add_lvalue_reference_t<std::add_const_t<Ref>>
	>;
	using rvalue_reference = std::conditional_t<
		std::is_reference_v<Ref>, Ref, std::add_rvalue_reference_t<Ref>
	>;
	using const_rvalue_reference = std::conditional_t<
		std::is_reference_v<Ref>, Ref, std::add_rvalue_reference_t<std::add_const_t<Ref>>
	>;

	constexpr basic_awaitable() noexcept = default;
	constexpr basic_awaitable(const basic_awaitable&) noexcept = delete;
	constexpr basic_awaitable(basic_awaitable&&) noexcept = default;
	constexpr ~basic_awaitable() = default;

	constexpr auto operator=(const basic_awaitable&) noexcept -> basic_awaitable& = delete;
	constexpr auto operator=(basic_awaitable&&) noexcept -> basic_awaitable& = default;

	static constexpr bool is_nothrow_ready = noexcept(std::declval<state_accessor&>().ready());
	static constexpr bool is_nothrow_suspend = is_nothrow_ready && noexcept(std::declval<state_accessor&>().await(detail::coro_handle{}));
	static constexpr bool is_nothrow_resume = false; // TODO

	explicit constexpr basic_awaitable(state_accessor&& accessor)
		noexcept(std::is_nothrow_constructible_v<state_accessor, state_accessor&&>) :
		state_accessor(std::forward<state_accessor>(accessor))
	{
	}

	explicit constexpr basic_awaitable(state_holder&& holder)
		noexcept(std::is_nothrow_constructible_v<state_accessor, state_holder&&>)
		requires (std::constructible_from<state_accessor, state_holder>) :
		state_accessor(std::forward<state_holder>(holder))
	{
	}

	constexpr bool valid() const noexcept
	{
		return state_accessor::is_valid_state();
	}
	
	constexpr bool await_ready() const noexcept(is_nothrow_ready)
	{
		return state_accessor::get_promise_state().ready();
	}
	
	template <typename Other>
	[[nodiscard]] constexpr auto await_suspend(std::coroutine_handle<Other> suspended_coroutine) noexcept(is_nothrow_suspend) -> decltype(auto)
	{
		return state_accessor::get_promise_state().await(suspended_coroutine);
	}

	constexpr auto get() & -> decltype(auto)
		requires (explicitly_convertible_to<internal_reference<state_accessor&>, reference>)
	{
		SHION_ASSERT(valid());
		if (!await_ready())
			throw std::bad_optional_access();
		return await_resume();
	}

	constexpr auto get() const& -> decltype(auto)
		requires (explicitly_convertible_to<internal_reference<state_accessor const&>, reference>)
	{
		SHION_ASSERT(this->valid());
		if (!this->await_ready())
			throw std::bad_optional_access();
		return await_resume();
	}

	constexpr auto get() && -> decltype(auto)
		requires (explicitly_convertible_to<internal_reference<state_accessor &&>, reference>)
	{
		SHION_ASSERT(valid());
		if (!await_ready())
			throw std::bad_optional_access();
		return await_resume();
	}

	constexpr auto get() const&& -> decltype(auto)
		requires (explicitly_convertible_to<internal_reference<state_accessor const&&>, reference>)
	{
		SHION_ASSERT(valid());
		if (!await_ready())
			throw std::bad_optional_access();
		return await_resume();
	}
	
	constexpr auto await_resume() & noexcept(is_nothrow_resume) -> reference
		requires (explicitly_convertible_to<internal_reference<state_accessor&>, reference>)
	{
		SHION_ASSERT(await_ready());
		return static_cast<reference>(_resume(state_accessor::get_promise_state()));
	}
	
	constexpr auto await_resume() const& noexcept(is_nothrow_resume) -> const_reference
		requires (explicitly_convertible_to<internal_reference<state_accessor const&>, reference>)
	{
		return static_cast<const_reference>(_resume(state_accessor::get_promise_state()));
	}
	
	constexpr auto await_resume() && noexcept(is_nothrow_resume) -> rvalue_reference
		requires (explicitly_convertible_to<internal_reference<state_accessor &&>, reference>)
	{
		return static_cast<rvalue_reference>(_resume(std::move(*this).state_accessor::get_promise_state()));
	}
	
	constexpr auto await_resume() const&& noexcept(is_nothrow_resume) -> const_rvalue_reference
		requires (explicitly_convertible_to<internal_reference<state_accessor const&&>, reference>)
	{
		return static_cast<const_rvalue_reference>(_resume(std::move(*this).state_accessor::get_promise_state()));
	}

private:
	template <typename T>
	static constexpr auto _get_value(T&& state) noexcept(is_nothrow_resume) -> decltype(auto)
	{
		if constexpr (requires { state.get_value(std::forward<T>(state)); })
		{
			return state.get_value(std::forward<T>(state));
		}
		else
		{
			return std::forward<T>(state).get_value();
		}
	}

	template <typename T>
	static constexpr auto _get_exception(T&& state) noexcept(is_nothrow_resume) -> decltype(auto)
	{
		if constexpr (requires { state.get_exception(std::forward<T>(state)); })
		{
			return state.get_exception(std::forward<T>(state));
		}
		else
		{
			return std::forward<T>(state).get_exception();
		}
	}

	template <typename T>
	static constexpr auto _resume(T&& state) noexcept(is_nothrow_resume) -> decltype(auto)
	{
		if constexpr (!is_nothrow_resume)
		{
			if (state.has_exception())
			{
				throw _get_exception(std::forward<T>(state));
			}
		}
		return _get_value(std::forward<T>(state));
	}
};

SHION_EXPORT template <typename Ref, typename Promise>
class basic_coroutine : public basic_awaitable<Ref, std::coroutine_handle<Promise>>
{
private:
	using base = basic_awaitable<Ref, std::coroutine_handle<Promise>>;

public:
	using promise_type = Promise;
	using base::base;
	using base::operator=;

	template <typename Other>
	[[nodiscard]] constexpr auto await_suspend(std::coroutine_handle<Other> suspended_coroutine) noexcept(base::is_nothrow_suspend) -> detail::coro_handle<>
	{
		if (this->await_ready())
			return suspended_coroutine;
		
		if (static_cast<bool>(base::await_suspend(suspended_coroutine)))
			return this->handle;

		return {};
	}
	
	constexpr bool done() const noexcept
	{
		SHION_ASSERT(this->is_valid_state());
		return this->get_coroutine_handle().done();
	}
};

static_assert(std::same_as<co_awaitable<int>::reference, int&>);

}

}
