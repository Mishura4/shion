#ifndef SHION_CORO_STATE_MACHINE_H_
#define SHION_CORO_STATE_MACHINE_H_

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#  include <shion/coro/coro.hpp>
#  include <shion/coro/awaitable.hpp>

#  include <variant>
#endif

namespace SHION_NAMESPACE
{

SHION_EXPORT template <typename YieldRef, typename ReturnRef = YieldRef, typename Yield = void, typename Return = void>
class basic_state_machine;

SHION_EXPORT template <typename YieldRef, typename ReturnRef = YieldRef>
class state_machine;

namespace detail
{
	
template <typename YieldRef, typename ReturnRef, typename Yield, typename Return, typename Allocator>
struct state_machine_promise;

template <typename YieldRef, typename ReturnRef, typename Yield, typename Return, typename Allocator>
class state_machine_base
{
public:
	using yield_value_t = std::conditional_t<std::is_void_v<Yield>, std::remove_cvref_t<YieldRef>, Yield>;
	using return_value_t = std::conditional_t<std::is_void_v<Return>, std::remove_cvref_t<ReturnRef>, Return>;
	using yield_reference_t = std::conditional_t<std::is_void_v<Yield>, YieldRef&&, YieldRef>;
	using return_reference_t = std::conditional_t<std::is_void_v<Return>, ReturnRef&&, ReturnRef>;

	using promise_type = state_machine_promise<YieldRef, ReturnRef, Yield, Return, Allocator>;
	
	constexpr state_machine_base() = default;
	constexpr state_machine_base(const state_machine_base&) = delete;
	constexpr state_machine_base(state_machine_base&& rhs) noexcept :
		_handle(std::exchange(rhs._handle, {}))
	{}

	constexpr ~state_machine_base()
	{
		if (_handle)
			_handle.destroy();
	}

	constexpr auto operator=(const state_machine_base &) -> state_machine_base& = delete;
	constexpr auto operator=(state_machine_base &&rhs) noexcept -> state_machine_base&
	{
		_handle = std::exchange(rhs._handle, std::move(_handle));
		return *this;
	}

	constexpr bool valid() const noexcept
	{
		return static_cast<bool>(_handle);
	}

	constexpr bool done() const noexcept
	{
		SHION_ASSERT(valid());
		return _handle.done();
	}

	constexpr operator bool() const noexcept
	{
		return valid();
	}

	constexpr bool advance()
	{
		SHION_ASSERT(valid());
		if (!done())
		{
			_handle.resume();
			return true;
		}
		return false;
	}
	
protected:
	std::coroutine_handle<promise_type> _handle{};

	auto get_promise() noexcept -> promise_type& { return _handle.promise(); }
	auto get_promise() const noexcept -> promise_type const& { return _handle.promise(); }
	
	state_machine_base(std::coroutine_handle<promise_type> handle) noexcept :
		_handle(std::move(handle))
	{}
};

template <typename YieldRef, typename ReturnRef, typename Yield, typename Return, typename Allocator>
class state_machine_resume;

template <typename YieldRef, typename Yield, typename Allocator>
class state_machine_resume<YieldRef, YieldRef, Yield, Yield, Allocator> : public detail::state_machine_base<YieldRef, YieldRef, Yield, Yield, Allocator>
{
	using my_base = detail::state_machine_base<YieldRef, YieldRef, Yield, Yield, Allocator>;

public:
	using typename my_base::promise_type;
	using typename my_base::yield_value_t;
	using typename my_base::yield_reference_t;
	using typename my_base::return_value_t;
	using typename my_base::return_reference_t;
	using value_type = yield_value_t;
	using reference_type = yield_reference_t;

	constexpr state_machine_resume() = default;
	constexpr state_machine_resume(state_machine_resume const&) = default;
	constexpr state_machine_resume(state_machine_resume&&) = default;
	
	constexpr auto operator=(state_machine_resume const&) -> state_machine_resume& = delete;
	constexpr auto operator=(state_machine_resume&&) -> state_machine_resume& = default;

	constexpr auto get_yielded_value() -> reference_type
	{
		return static_cast<yield_reference_t>(std::move(*std::get_if<1>(&this->get_promise().value)));
	}

	constexpr auto get_return_value() -> reference_type
	{
		return get_yielded_value();
	}

	constexpr auto get() -> reference_type
	{
		return get_yielded_value();
	}

	constexpr auto operator()() -> reference_type
	{
		SHION_ASSERT(!this->done());
		this->advance();
		return static_cast<reference_type>(std::move(*std::get_if<1>(&this->get_promise().value)));
	}

protected:
	state_machine_resume(std::coroutine_handle<promise_type> handle) noexcept :
		my_base(std::move(handle))
	{}
};

template <typename Yield, typename Return, typename UniRef, typename Allocator>
class state_machine_resume<UniRef, UniRef, Yield, Return, Allocator> : public detail::state_machine_base<UniRef, UniRef, Yield, Return, Allocator>
{
	using my_base = detail::state_machine_base<UniRef, UniRef, Yield, Return, Allocator>;

public:
	using typename my_base::promise_type;
	using typename my_base::yield_value_t;
	using typename my_base::yield_reference_t;
	using typename my_base::return_value_t;
	using typename my_base::return_reference_t;
	using reference_type = yield_reference_t;
	
	constexpr state_machine_resume() = default;
	constexpr state_machine_resume(state_machine_resume const&) = default;
	constexpr state_machine_resume(state_machine_resume&&) = default;
	
	constexpr auto operator=(state_machine_resume const&) -> state_machine_resume& = delete;
	constexpr auto operator=(state_machine_resume&&) -> state_machine_resume& = default;

	constexpr auto get_yielded_value() -> yield_reference_t
	{
		SHION_ASSERT(!this->done());
		return static_cast<yield_reference_t>(std::move(*std::get_if<1>(&this->get_promise().value)));
	}

	constexpr auto get_return_value() -> return_reference_t
	{
		SHION_ASSERT(this->done());
		return static_cast<return_reference_t>(std::move(*std::get_if<2>(&this->get_promise().value)));
	}

	constexpr auto get() -> reference_type
	{
		if (!this->done())
		{
			return get_yielded_value();
		}
		else
		{
			return get_return_value();
		}
	}

	constexpr auto operator()() -> reference_type
	{
		SHION_ASSERT(!this->done());
		this->advance();
		return get();
	}

protected:
	state_machine_resume(std::coroutine_handle<promise_type> handle) noexcept :
		my_base(std::move(handle))
	{}
};

template <typename YieldRef, typename ReturnRef, typename Yield, typename Return, typename Allocator>
class state_machine_resume : public state_machine_base<YieldRef, ReturnRef, Yield, Return, Allocator>
{
	using my_base = detail::state_machine_base<YieldRef, ReturnRef, Yield, Return, Allocator>;

public:
	using typename my_base::promise_type;
	using typename my_base::yield_value_t;
	using typename my_base::yield_reference_t;
	using typename my_base::return_value_t;
	using typename my_base::return_reference_t;
	using yield_pointer_t = std::add_pointer_t<std::remove_reference_t<YieldRef>>;
	using yield_alternative_t = std::conditional_t<std::is_lvalue_reference_v<yield_reference_t&&>, yield_pointer_t, std::remove_reference_t<yield_reference_t>>;
	using return_pointer_t = std::add_pointer_t<std::remove_reference_t<ReturnRef>>;
	using return_alternative_t = std::conditional_t<std::is_lvalue_reference_v<return_reference_t&&>, return_pointer_t, std::remove_reference_t<return_reference_t>>;
	using reference_type = std::variant<yield_alternative_t, return_alternative_t>;
	
	constexpr state_machine_resume() = default;
	constexpr state_machine_resume(state_machine_resume const&) = default;
	constexpr state_machine_resume(state_machine_resume&&) = default;
	
	constexpr auto operator=(state_machine_resume const&) -> state_machine_resume& = delete;
	constexpr auto operator=(state_machine_resume&&) -> state_machine_resume& = default;

	constexpr auto get_yielded_value() -> yield_reference_t
	{
		return static_cast<yield_reference_t>(std::move(*std::get_if<1>(&this->get_promise().value)));
	}

	constexpr auto get_return_value() -> return_reference_t
	{
		SHION_ASSERT(this->done());
		return static_cast<return_reference_t>(std::move(*std::get_if<2>(&this->get_promise().value)));
	}

	constexpr auto get() -> reference_type
	{
		if (!this->done())
		{
			if constexpr (std::is_lvalue_reference_v<yield_reference_t&&>)
			{
				return reference_type(std::in_place_index<0>, &get_yielded_value());
			}
			else
			{
				return reference_type(std::in_place_index<0>, get_yielded_value());
			}
		}
		else
		{
			if constexpr (std::is_lvalue_reference_v<return_reference_t&&>)
			{
				return reference_type(std::in_place_index<1>, &get_return_value());
			}
			else
			{
				return reference_type(std::in_place_index<1>, get_return_value());
			}
		}
	}

	constexpr auto operator()() -> reference_type
	{
		SHION_ASSERT(!this->done());
		this->advance();
		return get();
	}

protected:
	state_machine_resume(std::coroutine_handle<promise_type> handle) noexcept :
		my_base(std::move(handle))
	{}
};

}

SHION_EXPORT template <typename YieldRef, typename ReturnRef, typename Yield, typename Return>
class SHION_COROLIFETIMEBOUND basic_state_machine : public detail::state_machine_resume<YieldRef, ReturnRef, Yield, Return, void>
{
	using my_base = detail::state_machine_resume<YieldRef, ReturnRef, Yield, Return, void>;

public:
	using my_base::my_base;
	using my_base::operator=;
	using typename my_base::yield_value_t;
	using typename my_base::yield_reference_t;
	using typename my_base::return_value_t;
	using typename my_base::return_reference_t;
	using typename my_base::promise_type;

	constexpr bool await_ready() const noexcept
	{
		return false;
	}

	constexpr auto await_suspend(std::coroutine_handle<> coro) noexcept
	{
		SHION_ASSERT(!this->done());
		this->get_promise().continuation = std::move(coro);
		return this->_handle;
	}

	constexpr auto await_resume()
	{
		return this->get();
	}

protected:
	friend promise_type;

	constexpr basic_state_machine(std::coroutine_handle<promise_type> handle) noexcept :
		my_base(std::move(handle))
	{}
};

SHION_EXPORT template <typename Yield, typename Return>
class SHION_COROLIFETIMEBOUND state_machine : public basic_state_machine<Yield, Return>
{
public:
	using my_base = basic_state_machine<Yield, Return>;
	using typename my_base::promise_type;
	using my_base::my_base;
	using typename my_base::yield_value_t;
	using typename my_base::yield_reference_t;
	using typename my_base::return_value_t;
	using typename my_base::return_reference_t;

	constexpr state_machine(basic_state_machine<Yield, Return>&& base) :
		my_base(std::move(base))
	{}
};

namespace detail
{

struct state_machine_common
{
	std::coroutine_handle<>             continuation{ std::noop_coroutine() };

	struct awaiter
	{
		constexpr bool await_ready() const noexcept
		{
			return false;
		}

		template <typename T>
		constexpr auto await_suspend(std::coroutine_handle<T> self) const noexcept -> std::coroutine_handle<>
		{
			return std::exchange(self.promise().continuation, std::noop_coroutine());
		}

		constexpr void await_resume() const noexcept {}
	};

	void unhandled_exception()
	{
		std::rethrow_exception(std::current_exception());
	}

	constexpr auto initial_suspend() noexcept -> std::suspend_always { return {}; }
	constexpr auto final_suspend() noexcept -> awaiter { return {}; }
};

template <typename YieldRef, typename ReturnRef, typename Yield, typename Return>
struct state_machine_meta
{
	using yield_value_t = std::conditional_t<std::is_void_v<Yield>, std::remove_cvref_t<YieldRef>, Yield>;
	using return_value_t = std::conditional_t<std::is_void_v<Return>, std::remove_cvref_t<ReturnRef>, Return>;
	using yield_reference_t = std::conditional_t<std::is_void_v<Yield>, YieldRef&&, YieldRef>;
	using return_reference_t = std::conditional_t<std::is_void_v<Return>, ReturnRef&&, ReturnRef>;
};

template <typename YieldRef, typename ReturnRef, typename Yield, typename Return>
struct state_machine_storage : state_machine_common, state_machine_meta<YieldRef, ReturnRef, Yield, Return>
{
	using meta_t = state_machine_meta<YieldRef, ReturnRef, Yield, Return>;
	using typename meta_t::yield_value_t;
	using typename meta_t::return_value_t;
	using typename meta_t::yield_reference_t;
	using typename meta_t::return_reference_t;

	std::variant<std::monostate, yield_value_t, return_value_t> value;

	void return_value(return_value_t ret) noexcept(std::is_nothrow_move_constructible_v<return_value_t>)
	requires (std::is_move_constructible_v<return_value_t> || std::is_copy_constructible_v<return_value_t>)
	{
		value.template emplace<2>(std::move(ret));
	}

	template <std::convertible_to<return_value_t> U>
	void return_value(U&& ret) noexcept(std::is_nothrow_constructible_v<return_value_t, U>)
	{
		value.template emplace<2>(std::forward<U>(ret));
	}

	auto yield_value(yield_value_t ret) noexcept(std::is_nothrow_move_constructible_v<yield_value_t>) -> awaiter
	requires (std::is_move_constructible_v<yield_value_t> || std::is_copy_constructible_v<yield_value_t>)
	{
		value.template emplace<1>(std::move(ret));
		return {};
	}

	template <std::convertible_to<yield_value_t> U>
	auto yield_value(U&& ret) noexcept(std::is_nothrow_constructible_v<yield_value_t, U>) -> awaiter
	{
		value.template emplace<1>(std::forward<U>(ret));
		return {};
	}
};

template <typename YieldRef, typename Yield>
struct state_machine_storage <YieldRef, YieldRef, Yield, Yield> : state_machine_common, state_machine_meta<YieldRef, YieldRef, Yield, Yield>
{
	using meta_t = state_machine_meta<YieldRef, YieldRef, Yield, Yield>;
	using typename meta_t::yield_value_t;
	using typename meta_t::return_value_t;
	using typename meta_t::yield_reference_t;
	using typename meta_t::return_reference_t;

	std::variant<std::monostate, yield_value_t> value;

	constexpr void return_value(yield_value_t ret) noexcept(std::is_nothrow_move_constructible_v<yield_value_t>)
	requires (std::is_move_constructible_v<yield_value_t> || std::is_copy_constructible_v<yield_value_t>)
	{
		value.template emplace<1>(std::move(ret));
	}

	template <std::convertible_to<yield_value_t> U>
	constexpr void return_value(U&& ret) noexcept(std::is_nothrow_constructible_v<yield_value_t, U>)
	{
		value.template emplace<1>(std::forward<U>(ret));
	}

	constexpr auto yield_value(yield_value_t ret) noexcept(std::is_nothrow_move_constructible_v<yield_value_t>) -> awaiter
	requires (std::is_move_constructible_v<yield_value_t> || std::is_copy_constructible_v<yield_value_t>)
	{
		value.template emplace<1>(std::move(ret));
		return {};
	}

	template <std::convertible_to<yield_value_t> U>
	constexpr auto yield_value(U&& ret) noexcept(std::is_nothrow_constructible_v<yield_value_t, U>) -> awaiter
	{
		value.template emplace<1>(std::forward<U>(ret));
		return {};
	}
};


template <typename YieldRef, typename Yield>
struct state_machine_storage<YieldRef, void, Yield, void> : state_machine_common, state_machine_meta<YieldRef, void, Yield, void>
{
	using meta_t = state_machine_meta<YieldRef, void, Yield, void>;
	using typename meta_t::yield_value_t;
	using typename meta_t::return_value_t;
	using typename meta_t::yield_reference_t;
	using typename meta_t::return_reference_t;

	std::variant<std::monostate, yield_value_t> value;

	constexpr auto yield_value(yield_value_t ret) noexcept(std::is_nothrow_move_constructible_v<yield_value_t>) -> std::suspend_always
	requires (std::is_move_constructible_v<yield_value_t> || std::is_copy_constructible_v<yield_value_t>)
	{
		value.template emplace<1>(std::move(ret));
		return {};
	}

	template <std::convertible_to<yield_value_t> U>
	constexpr auto yield_value(U&& ret) noexcept(std::is_nothrow_constructible_v<yield_value_t, U>) -> std::suspend_always
	{
		value.template emplace<1>(std::forward<U>(ret));
		return {};
	}

	constexpr void return_void() const noexcept {}
};

template <typename YieldRef, typename ReturnRef, typename Yield, typename Return, typename Allocator>
struct state_machine_promise : state_machine_storage<YieldRef, ReturnRef, Yield, Return>
{
	/*
	alignas(__STDCPP_DEFAULT_NEW_ALIGNMENT__) struct U // https://en.cppreference.com/w/cpp/coroutine/generator/promise_type/operator_new
	{
		std::byte dummy[__STDCPP_DEFAULT_NEW_ALIGNMENT__];
	};
	
	using bllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<U>;

	alignas(__STDCPP_DEFAULT_NEW_ALIGNMENT__) union allocator_storage {
		empty e;
		bllocator b;
	} alloc;

	void* operator new(size_t size) requires (std::default_initializable<Allocator>)
	{
		allocator_storage alloc;

		std::construct_at(&alloc.b);
		auto sz = (size + sizeof(allocator_storage) + sizeof(U) - 1) / sizeof(U);
		std::allocator_traits<bllocator>::allocate(sz);
	}
	*/

	constexpr auto get_return_object()
	{
		return basic_state_machine<YieldRef, ReturnRef, Yield, Return> { std::coroutine_handle<state_machine_promise>::from_promise(*this) };
	}
};

}

}

#endif /* SHION_CORO_STATE_MACHINE_H_ */
