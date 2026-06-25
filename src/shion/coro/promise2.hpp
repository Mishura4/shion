//
// Created by miuna on 3/31/2026.
//

#ifndef SHION_PROMISE_HPP_
#define SHION_PROMISE_HPP_

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#include <type_traits>
#include <bit>
#include <list>
#include <coroutine>

#include <shion/meta/macros.hpp>
#include <shion/common.hpp>
#include <shion/common/detail.hpp>
#include <shion/meta/type_traits.hpp>
#include <shion/utility/optional.hpp>
#include <shion/coro/coro.hpp>
#include <shion/coro/state_machine.hpp>
#endif

namespace SHION_NAMESPACE
{

SHION_EXPORT template <typename Controller, typename Value, typename Carry, template <typename> typename StateHolder = std::in_place_type_t>
struct basic_promise;

namespace coro
{

SHION_EXPORT template <typename Controller, typename Ref, typename Value, typename YieldRef = void, typename YieldValue = void>
class basic_coro_promise;

SHION_EXPORT template <typename Controller>
class suspend_and_continue
{
public:
	constexpr suspend_and_continue(Controller& handler) noexcept : self(&handler)
	{
	}
	
	constexpr ~suspend_and_continue()
	{
		if (auto awaiter = self->release_awaiter())
		{
			awaiter.resume(); // terminates if it throws, intended
		}
	}

	constexpr static bool await_ready() noexcept { return false; }
	constexpr auto        await_suspend(detail::coro_handle<>) const noexcept -> detail::coro_handle<>
	{
		if (auto awaiter = self->release_awaiter())
			return awaiter;
		else
			return std::noop_coroutine();
	}
	constexpr static void await_resume() noexcept {}
	constexpr static void finalize() noexcept {}

private:
	Controller* self;
};

SHION_EXPORT template <typename Ref, typename Value = void>
class co_awaitable;

}

namespace detail::coro
{

using namespace SHION_NAMESPACE::coro;

/**
 * @brief State of a promise
 */
enum state_flags {
	/**
	 * @brief Promise is empty
	 */
	sf_none = 0b0000000,

	/**
	 * @brief Promise has spawned an awaitable
	 */
	sf_has_awaitable = 0b00000001,

	/**
	 * @brief Promise is being awaited
	 */
	sf_awaited = 0b00000010,

	/**
	 * @brief Promise has a result
	 */
	sf_ready = 0b00000100,

	/**
	 * @brief Promise has completed, no more results are expected
	 */
	sf_done = 0b00001000,

	/**
	 * @brief Promise was broken - future or promise is gone
	 */
	sf_broken = 0b0010000
};

template <typename Controller, typename Value, typename Carry>
struct promise_state_base : public Controller, public basic_result<shion::storage<Value>, shion::storage<Carry>>
{
	using result_type = basic_result<shion::storage<Value>, shion::storage<Carry>>;
	using value_storage = shion::storage<Value>;
	using carry_storage = shion::storage<Carry>;
	
	constexpr auto emplace_exception(std::exception_ptr ex) noexcept
	{
		if constexpr (requires { this->acquire_set_exception(*this); })
		{
			auto handle = this->acquire_set_exception(*this);
			this->result_type::set_exception(std::move(ex));
			return handle;
		}
		else
		{
			auto handle = this->acquire_set_value(*this);
			this->result_type::set_exception(std::move(ex));
			return handle;
		}
	}
	
	constexpr bool ready() const noexcept
	{
		return Controller::ready() && !this->empty();
	}
};

template <typename Controller, typename Value, typename Carry>
struct promise_value_layer : public promise_state_base<Controller, Value, Carry>
{
	using typename promise_state_base<Controller, Value, Carry>::value_storage;
	using value_type = typename value_storage::value_type;
	using value_reference = typename value_storage::reference;
	using value_expr_type = value_type;
	using value_expr_move = value_type&&;
	using value_expr_copy = std::add_const_t<value_type>&;
	
	template <typename... Args>
	inline static constexpr bool is_nothrow_set_value =
		noexcept(std::declval<Controller&>().acquire_set_value(std::declval<promise_value_layer&>()))
		&& std::is_nothrow_constructible_v<value_storage, Args...>;

	/*
	constexpr auto set_value(value_type&& value) noexcept(is_nothrow_set_value<value_type&&>)
		requires (std::is_move_constructible_v<value_type>)
	{
		return this->emplace_value(std::move(value));
	}
	
	constexpr auto set_value(std::add_const_t<value_type>& value) noexcept(is_nothrow_set_value<std::add_const_t<value_type>&>)
		requires (std::is_copy_constructible_v<value_type>)
	{
		return this->emplace_value(value);
	}
	
	constexpr auto return_value(value_expr_move value) noexcept(is_nothrow_set_value<value_expr_move>)
		requires (std::is_move_constructible_v<value_type>)
	{
		return this->emplace_value(std::move(value));
	}
	
	constexpr auto return_value(value_expr_copy value) noexcept(is_nothrow_set_value<value_expr_copy>)
		requires (std::is_copy_constructible_v<value_type>)
	{
		return this->emplace_value(value);
	}
	
	template <typename Expr>
	constexpr auto return_value(Expr&& value) noexcept(is_nothrow_set_value<Expr>)
		requires (std::is_constructible_v<value_type, Expr>)
	{
		return this->emplace_value(std::forward<Expr>(value));
	}
	*/
	
	template <typename... Args>
	constexpr auto emplace_value(Args&&... args) noexcept(is_nothrow_set_value<Args...>)
		requires (std::constructible_from<Value, Args...>)
	{
		auto handle = this->acquire_set_value(*this);
		this->template emplace<0>(std::forward<Args>(args)...);
		return handle;
	}
	
	constexpr bool has_value() const noexcept
	{
		return this->index() == 0;
	}
	
	constexpr auto get_value() & -> decltype(auto)
		requires requires (value_storage& s) { *s; }
	{
		return *shion::get<0>(*this);
	}
	
	constexpr auto get_value() const& -> decltype(auto)
		requires requires (value_storage const& s) { *s; }
	{
		return *shion::get<0>(*this);
	}
	
	constexpr auto get_value() && -> decltype(auto)
		requires requires (value_storage&& s) { *s; }
	{
		return *shion::get<0>(std::move(*this));
	}
	
	constexpr auto get_value() const&& -> decltype(auto)
		requires requires (value_storage const&& s) { *s; }
	{
		return *shion::get<0>(std::move(*this));
	}
};

template <typename Controller, typename Carry>
struct promise_value_layer<Controller, void, Carry> : public promise_state_base<Controller, void, Carry>
{
	using value_type = void;
	using value_reference = void;
	using value_expr_type = void;
	
	inline static constexpr bool is_nothrow_set_value = noexcept(std::declval<Controller&>().acquire_set_value(std::declval<promise_value_layer&>()));

	constexpr auto emplace_value() noexcept(is_nothrow_set_value)
	{
		auto handle = Controller::acquire_set_value();
		this->template emplace<0>();
		return handle;
	}
};

template <typename Controller, typename Value, typename Carry>
struct promise_carry_layer : public promise_value_layer<Controller, Value, Carry>
{
	using typename promise_value_layer<Controller, Value, Carry>::carry_storage;
	using carry_type = typename carry_storage::value_type;
	using carry_reference = typename carry_storage::reference;
	using carry_expr_type = carry_type;
	using carry_expr_move = carry_expr_type&&;
	using carry_expr_copy = std::add_const_t<carry_type>&;
	
	template <typename... Args>
	inline static constexpr bool is_nothrow_set_carry =
		noexcept(std::declval<Controller&>().acquire_set_carry(std::declval<promise_carry_layer&>()))
		&& std::is_nothrow_constructible_v<carry_storage, Args...>;
	
	/*
	constexpr auto set_carry(carry_type&& value) noexcept(is_nothrow_set_carry<carry_type&&>)
		requires (std::is_move_constructible_v<carry_type>)
	{
		return this->emplace_carry(std::move(value));
	}
	
	constexpr auto set_carry(std::add_const_t<carry_type>& value) noexcept(is_nothrow_set_carry<std::add_const_t<carry_type>&>)
		requires (std::is_copy_constructible_v<carry_type>)
	{
		return this->emplace_carry(value);
	}
	
	constexpr auto yield_value(carry_type&& value) noexcept(is_nothrow_set_carry<carry_type&&>)
		requires (std::is_move_constructible_v<carry_type>)
	{
		return this->emplace_carry(std::move(value));
	}
	
	constexpr auto yield_value(std::add_const_t<carry_type>& value) noexcept(is_nothrow_set_carry<std::add_const_t<carry_type>&>)
		requires (std::is_copy_constructible_v<carry_type>)
	{
		return this->emplace_carry(value);
	}
	
	template <typename Expr>
	constexpr auto yield_value(Expr&& value) noexcept(is_nothrow_set_carry<Expr>)
		requires (std::is_constructible_v<carry_type, Expr>)
	{
		return this->emplace_carry(std::forward<Expr>(value));
	}
	*/
	
	template <typename... Args>
	constexpr auto emplace_carry(Args&&... args) noexcept(is_nothrow_set_carry<Args...>)
		requires (std::constructible_from<carry_storage, Args...>)
	{
		auto handle = Controller::acquire_set_carry();
		this->template emplace<1>(std::forward<Args>(args)...);
		return handle;
	}

	constexpr bool has_carry() const noexcept
	{
		return this->index() == 1;
	}
	
	constexpr auto get_carry() & -> decltype(auto)
		requires requires (carry_storage& s) { *s; }
	{
		return *shion::get<1>(*this);
	}
	
	constexpr auto get_carry() const& -> decltype(auto)
		requires requires (carry_storage const& s) { *s; }
	{
		return *shion::get<1>(*this);
	}
	
	constexpr auto get_carry() && -> decltype(auto)
		requires requires (carry_storage&& s) { *s; }
	{
		return *shion::get<1>(std::move(*this));
	}
	
	constexpr auto get_carry() const&& -> decltype(auto)
		requires requires (carry_storage const&& s) { *s; }
	{
		return *shion::get<1>(std::move(*this));
	}
};

template <typename Controller, typename Value>
struct promise_carry_layer<Controller, Value, void> : public promise_value_layer<Controller, Value, void>
{
	using carry_type = void;
	using carry_reference = void;
	using carry_expr_type = make_complete<void>;
};

template <typename Controller, typename Return, typename YieldRef>
struct promise_state : promise_carry_layer<Controller, Return, YieldRef>
{
};

template <typename Controller, typename Yield>
requires (!std::is_void_v<Yield>)
struct promise_state<Controller, void, Yield> : promise_carry_layer<Controller, void, Yield>
{
private:
	using carry_layer = promise_carry_layer<Controller, void, Yield>;

public:
	constexpr auto get_value() & -> decltype(auto)
		requires requires (carry_layer& s) { s.get_carry(); }
	{
		return this->get_carry();
	}
	
	constexpr auto get_value() const& -> decltype(auto)
		requires requires (carry_layer const& s) { s.get_carry(); }
	{
		return this->get_carry();
	}
	
	constexpr auto get_value() && -> decltype(auto)
		requires requires (carry_layer&& s) { std::move(s).get_carry(); }
	{
		return std::move(*this).get_carry();
	}
	
	constexpr auto get_value() const&& -> decltype(auto)
		requires requires (carry_layer const&& s) { std::move(s).get_carry(); }
	{
		return std::move(*this).get_carry();
	}
};

struct inert_controller
{
	constexpr static auto acquire_set_value(auto&&...) -> inert_controller { return {}; }
	constexpr static auto acquire_set_carry(auto&&...) -> inert_controller { return {}; }
	constexpr static void finalize() noexcept {}
	constexpr static bool await_ready() noexcept { return true; }
	constexpr static void await_suspend(std::coroutine_handle<>) noexcept {}
	constexpr static void await_resume() noexcept {}
};

class simple_continuation_controller
{
public:
	/**
	 * @brief Coroutine handle currently awaiting the completion of this promise.
	 */
	coro_handle<> awaiter = nullptr;

public:
	/**
	 * @brief Construct a new promise, with empty result.
	 */
	constexpr simple_continuation_controller() = default;

	constexpr auto await(coro_handle<> suspended_coroutine) noexcept -> coro_handle<>
	{
		auto prev = std::exchange(awaiter, suspended_coroutine);
		return prev == nullptr ? std::noop_coroutine() : prev;
	}

	constexpr auto release_awaiter(coro_handle<> exchange = {}) noexcept -> detail::std_coroutine::coroutine_handle<>
	{
		return std::exchange(awaiter, exchange);
	}

	constexpr void mark_ready(bool notify)
	{
		if (notify)
			notify_awaiter();
	}

	static constexpr void mark_awaited()
	{
	}

	/**
	 * @brief Notify a currently awaiting coroutine that the result is ready.
	 */
	constexpr bool notify_awaiter() const
	{
		if (has_awaiter())
		{
			awaiter.resume();
			return true;
		}
		return false;
	}
	
	constexpr auto acquire_set_carry(auto&&...) noexcept -> suspend_and_continue<simple_continuation_controller>
	{
		return { *this };
	}
	
	constexpr auto acquire_set_value(auto&&...) noexcept -> suspend_and_continue<simple_continuation_controller>
	{
		return { *this };
	}

	constexpr bool has_awaiter() const noexcept
	{
		return awaiter.address() != nullptr;
	}

	static constexpr bool abandon() noexcept
	{
		return true;
	}

	static constexpr bool ready() noexcept
	{
		return true;
	}
};

class atomic_continuation_controller : protected simple_continuation_controller
{
protected:
	using flags = detail::coro::state_flags;

	/**
	 * @brief State of the awaitable tied to this promise.
	 */
	std::atomic<uint8> state = flags::sf_none;

public:
	using simple_continuation_controller::has_awaiter;

	auto await(detail::std_coroutine::coroutine_handle<> handle)
	{
		auto previous_flags = state.fetch_or(flags::sf_awaited);
		if (previous_flags & flags::sf_awaited) {
			throw logic_exception("awaitable is already being awaited");
		}
		// TODO: RACE CONDITION ON AWAITER?
		if (previous_flags & flags::sf_ready) [[unlikely]] {
			return handle;
		}
		return simple_continuation_controller::await(handle);
	}

	/**
	 * @brief Notify a currently awaiting coroutine that the result is ready.
	 */
	void notify_awaiter()
	{
		if (state.load(std::memory_order_acquire) & flags::sf_awaited)
		{
			SHION_ASSUME(this->has_awaiter());
			simple_continuation_controller::notify_awaiter();
		}
	}

	void mark_ready(bool notify)
	{
		[[maybe_unused]] auto previous_value = this->state.fetch_or(flags::sf_ready, std::memory_order_seq_cst);
		if (notify && (previous_value & flags::sf_awaited))
		{
			this->notify_awaiter();
		}
	}

	void mark_awaited()
	{
		[[maybe_unused]] auto previous_value = this->state.fetch_or(flags::sf_ready, std::memory_order_seq_cst);
		auto previous_flags = state.fetch_or(flags::sf_has_awaitable, std::memory_order_acq_rel);
		if (previous_flags & flags::sf_has_awaitable) [[unlikely]] {
			throw logic_exception{"an awaitable was already created from this promise"};
		}
	}

	bool ready() const noexcept
	{
		auto value = state.load(std::memory_order_relaxed);
		return value & flags::sf_ready;
	}

	bool abandon() noexcept
	{
		auto previous = state.fetch_or(flags::sf_broken, std::memory_order_relaxed);
		return previous & sf_done;
	}

	bool done(std::memory_order order = std::memory_order_relaxed) const noexcept
	{
		return state.load(order) & flags::sf_done;
	}
	
	constexpr auto acquire_set_carry(auto&&...) noexcept -> suspend_and_continue<simple_continuation_controller>
	{
		return { *this };
	}
	
	class set_value
	{
	public:
		constexpr set_value(atomic_continuation_controller& handler) noexcept : self(&handler)
		{
		}
	
		constexpr ~set_value()
		{
			finalize(); // terminates if resume throws, intended
		}
		
		constexpr void finalize()
		{
			if (!self)
				return;
			
			auto controller = std::exchange(self, nullptr);
			auto prev = controller->state.fetch_or(flags::sf_ready, std::memory_order_acq_rel);
			SHION_ASSERT(!(prev & flags::sf_ready));
			if ((prev & flags::sf_awaited) == flags::sf_awaited) // we have an awaiter
			{
				if (auto handle = controller->release_awaiter())
				{
					handle.resume();
				}
				else
				{
					SHION_ASSERT(handle, "we have awaited in flags but no awaiter, something went very wrong");
				}
			}
		}

	private:
		atomic_continuation_controller* self;
	};
	
	constexpr auto acquire_set_value(auto&&...) noexcept -> set_value
	{
		return { *this };
	}
};

template <typename T>
struct promise_state_accessor_base;

template <typename T>
	requires requires (T const &t) { *t; }
struct promise_state_accessor_base<T>
{
	using promise_state = T;
	using propagate_type = promise_state_accessor_base<std::conditional_t<std::is_copy_constructible_v<T>, T, T&>>;

	T state;
	
	constexpr promise_state_accessor_base() = default;
	constexpr promise_state_accessor_base(const promise_state_accessor_base&) = default;
	constexpr promise_state_accessor_base(promise_state_accessor_base&&) = default;
	constexpr promise_state_accessor_base(T value) noexcept(std::is_nothrow_constructible_v<T>) requires (std::move_constructible<T>) :
		state(std::move(value))
	{
	}
	constexpr ~promise_state_accessor_base() = default;

	constexpr auto operator=(const promise_state_accessor_base&) -> promise_state_accessor_base& = default;
	constexpr auto operator=(promise_state_accessor_base&&) -> promise_state_accessor_base& = default;

	constexpr bool is_valid_state() const noexcept
	{
		if constexpr (requires { static_cast<bool>(state); })
		{
			return static_cast<bool>(state);
		}
		return true;
	}
	
	constexpr auto get_promise_state() & noexcept -> decltype(auto)
	{
		SHION_ASSERT(is_valid_state());
		return *state;
	}
	
	constexpr auto get_promise_state() && noexcept -> decltype(auto)
	{
		SHION_ASSERT(is_valid_state());
		return *std::move(state);
	}
	
	constexpr auto get_promise_state() const& noexcept -> decltype(auto)
	{
		SHION_ASSERT(is_valid_state());
		return *state;
	}
	
	constexpr auto get_promise_state() const&& noexcept -> decltype(auto)
	{
		SHION_ASSERT(is_valid_state());
		return *std::move(state);
	}
};

template <typename T>
struct promise_state_accessor_base<T&>
{
	using promise_state = T;
	using propagate_type = promise_state_accessor_base&;

	T& state;
	
	constexpr promise_state_accessor_base() = default;
	constexpr promise_state_accessor_base(T& promise) : state(promise) {}
	constexpr promise_state_accessor_base(promise_state_accessor_base<T> other) : state(other.state) {}

	constexpr static bool is_valid_state() noexcept
	{
		return true;
	}
	
	constexpr auto get_promise_state() const noexcept -> decltype(auto)
	{
		return state;
	}
};

template <typename T>
struct promise_state_accessor_base<std::in_place_type_t<T>>
{
	using promise_state = T;
	using propagate_type = promise_state_accessor_base<std::in_place_type_t<T&>>;

	T state;
	
	static constexpr bool is_valid_state() noexcept { return true; }
	
	constexpr auto get_promise_state() & noexcept -> promise_state&
	{
		return state;
	}
	
	constexpr auto get_promise_state() && noexcept -> promise_state&&
	{
		return std::move(state);
	}
	
	constexpr auto get_promise_state() const& noexcept -> promise_state const&
	{
		return state;
	}
	
	constexpr auto get_promise_state() const&& noexcept -> promise_state const&&
	{
		return std::move(state);
	}
};

template <typename T>
struct promise_state_accessor_base<std::in_place_type_t<T&>>
{
	using promise_state = T;
	using propagate_type = promise_state_accessor_base;

	T* state;
	
	constexpr promise_state_accessor_base() = default;
	constexpr promise_state_accessor_base(T& promise) : state(&promise) {}
	constexpr promise_state_accessor_base(promise_state_accessor_base<T> other) : state(&other.state) {}
	constexpr promise_state_accessor_base(promise_state_accessor_base<T&> other) : state(other.state) {}
	constexpr promise_state_accessor_base(promise_state_accessor_base<std::in_place_type_t<T>> other) : state(&other.state) {}

	constexpr bool is_valid_state() const noexcept
	{
		return state != nullptr;
	}
	
	constexpr auto get_promise_state() & noexcept -> decltype(auto)
	{
		SHION_ASSERT(is_valid_state());
		return *state;
	}
	
	constexpr auto get_promise_state() && noexcept -> decltype(auto)
	{
		SHION_ASSERT(is_valid_state());
		return *std::move(state);
	}
	
	constexpr auto get_promise_state() const& noexcept -> decltype(auto)
	{
		SHION_ASSERT(is_valid_state());
		return *state;
	}
	
	constexpr auto get_promise_state() const&& noexcept -> decltype(auto)
	{
		SHION_ASSERT(is_valid_state());
		return *std::move(state);
	}
};

template <typename T>
struct promise_state_accessor_base<std::coroutine_handle<T>>
{
	using promise_state = std::remove_cvref_t<decltype(std::declval<T>().get_promise_state())>;
	using propagate_type = promise_state_accessor_base;
	using handle_type = std::coroutine_handle<T>;

	handle_type handle = {};
	
	constexpr promise_state_accessor_base() noexcept = default;

	constexpr promise_state_accessor_base(const promise_state_accessor_base&) = delete;
	constexpr promise_state_accessor_base(promise_state_accessor_base&& other) noexcept :
		handle(std::exchange(other.handle, nullptr))
	{
	}

	constexpr promise_state_accessor_base(handle_type h) noexcept : handle(h) {}

	constexpr ~promise_state_accessor_base()
	{
		abandon();
	}

	constexpr auto operator=(const promise_state_accessor_base&) -> promise_state_accessor_base& = delete;
	constexpr auto operator=(promise_state_accessor_base&& other) noexcept -> promise_state_accessor_base&
	{
		abandon();
		handle = std::exchange(other.handle, nullptr);
		return *this;
	}

	constexpr void abandon() noexcept
	{
		if (!is_valid_state())
			return;

		auto& state = get_promise_state();
		bool abandoned = true;
		if constexpr (requires { state.abandon(this); })
			abandoned = state.abandon(this);
		else
			abandoned = state.abandon();
		if (abandoned)
		{
			auto h = std::exchange(handle, nullptr);
			h.destroy();
		}
	}

	constexpr bool is_valid_state() const noexcept
	{
		return static_cast<bool>(handle);
	}

	constexpr auto get_promise_state() const noexcept -> auto&
	{
		return handle.promise().get_promise_state();
	}

	constexpr auto get_coroutine_handle() const noexcept -> handle_type
	{
		return handle;
	}
};

template <typename T>
struct promise_state_accessor_base
{
	using promise_state = T;
	using propagate_type = promise_state_accessor_base<std::in_place_type_t<T&>>;

	T state;
	
	static constexpr bool is_valid_state() noexcept { return true; }
	
	constexpr auto get_promise_state() & noexcept -> promise_state&
	{
		return state;
	}
	
	constexpr auto get_promise_state() && noexcept -> promise_state&&
	{
		return std::move(state);
	}
	
	constexpr auto get_promise_state() const& noexcept -> promise_state const&
	{
		return state;
	}
	
	constexpr auto get_promise_state() const&& noexcept -> promise_state const&&
	{
		return std::move(state);
	}
};

template <typename T>
class promise_state_accessor : public promise_state_accessor_base<T>
{
private:
	using base = promise_state_accessor_base<T>;

public:
	using typename base::promise_state;
	using base::base;
	using base::operator=;
	
	template <typename U>
	constexpr auto await(std::coroutine_handle<U> handle) -> decltype(auto)
	{
		SHION_ASSERT(this->is_valid_state());
		return this->get_promise_state().await(std::move(handle));
	}

	constexpr bool ready() const noexcept
	{
		SHION_ASSERT(this->is_valid_state());
		return this->get_promise_state().ready();
	}
};

}

template <typename Controller, typename Value, typename Carry, template <typename> typename StateHolder>
struct basic_promise : protected detail::coro::promise_state_accessor<StateHolder<detail::coro::promise_state<Controller, Value, Carry>>>
{
public:
	using state = detail::coro::promise_state<Controller, Value, Carry>;
	using state_holder = StateHolder<state>;

protected:
	using accessor = detail::coro::promise_state_accessor<state_holder>;
	using propagate_accessor = typename accessor::propagate_type;
	using default_awaitable = basic_awaitable<Value, propagate_accessor>;
	using typename accessor::promise_state;
	
	friend accessor;

public:
	using value_type = Value;
	using awaitable = default_awaitable;
	using accessor::accessor;
	using accessor::get_promise_state;
	
	constexpr void emplace_exception(std::exception_ptr ex) noexcept(noexcept(this->get_promise_state().emplace_exception(std::move(ex)).finalize()))
	{
		auto handle = this->get_promise_state().emplace_exception(std::move(ex));
		handle.finalize();
	}

	template <typename... Args>
	constexpr auto emplace_value(Args&&... args) noexcept(noexcept(this->get_promise_state().emplace_value(std::declval<Args>()...).finalize()))
		requires (std::constructible_from<typename state::value_storage, Args...>)
	{
		auto handle = this->get_promise_state().emplace_value(std::forward<Args>(args)...);
		handle.finalize();
		return handle;
	}

	template <typename... Args>
	constexpr auto emplace_carry(Args&&... args) noexcept(noexcept(this->get_promise_state().emplace_carry(std::declval<Args>()...).finalize()))
		requires (std::constructible_from<typename state::carry_storage, Args...>)
	{
		auto handle = this->get_promise_state().emplace_carry(std::forward<Args>(args)...);
		handle.finalize();
		return handle;
	}

	template <typename Cast = propagate_accessor, typename Self>
	constexpr auto get_awaitable(this Self&& self)
	{
		using awaitable_type = typename std::remove_cvref_t<Self>::awaitable;
		std::forward<Self>(self).get_promise_state().mark_awaited();
		return awaitable_type{ static_cast<Cast>(std::forward<Self>(self)) };
	}
};

namespace detail::coro
{

template <typename Controller, typename Ref, typename Value, typename YieldRef, typename YieldValue>
class basic_coro_promise_base : public basic_promise<
	Controller,
	std::conditional_t<std::is_void_v<Value>, Ref, Value>,
	std::conditional_t<std::is_void_v<YieldValue>, YieldRef, YieldValue>
>
{
private:
	using base = basic_promise<
		Controller,
		std::conditional_t<std::is_void_v<Value>, Ref, Value>,
		std::conditional_t<std::is_void_v<YieldValue>, YieldRef, YieldValue>
	>;
	
public:
	using base::base;
	using base::get_promise_state;
	using typename base::value_type;
	using typename base::promise_state; // Expose this as public ; because coroutine stuff often needs it

	constexpr void unhandled_exception() noexcept
	{
		this->emplace_exception(std::current_exception());
	}

	static constexpr auto initial_suspend() noexcept -> std::suspend_never { return {}; }

	constexpr auto final_suspend() noexcept -> suspend_and_continue<Controller>
	{
		return suspend_and_continue<Controller>{ this->get_promise_state() };
	}

	template <typename Self>
	constexpr auto get_return_object(this Self& self) noexcept
	{
		return typename Self::awaitable{ std::coroutine_handle<Self>::from_promise(self) };
	}
};

template <typename Controller, typename Ref, typename Value, typename YieldRef, typename YieldValue>
class basic_coro_promise_return : public basic_coro_promise_base<Controller, Ref, Value, YieldRef, YieldValue>
{
	using base = basic_coro_promise_base<Controller, Ref, Value, YieldRef, YieldValue>;

public:
	using base::base;
	using return_type = Ref;
	using return_reference = Ref;

	constexpr void return_value(Ref&& value) noexcept(noexcept(this->emplace_value(std::declval<Ref&&>())))
		requires (std::constructible_from<Ref, Ref&&>)
	{
		this->emplace_value(std::forward<Ref>(value));
	}

	constexpr void return_value(const Ref& value) noexcept(noexcept(this->emplace_value(std::declval<const Ref&>())))
		requires (!std::is_reference_v<Ref> && std::copy_constructible<Ref>)
	{
		this->emplace_value(value);
	}
};

template <typename Controller, typename Value, typename YieldRef, typename YieldValue>
class basic_coro_promise_return<Controller, void, Value, YieldRef, YieldValue> :
	public basic_coro_promise_base<Controller, void, Value, YieldRef, YieldValue>
{
	using base = basic_coro_promise_base<Controller, void, Value, YieldRef, YieldValue>;

public:
	using base::base;
	using return_type = void;
	using return_reference = void;
	
	constexpr void return_void() noexcept(noexcept(this->emplace_value()))
	{
		this->emplace_value();
	}
};

template <typename Controller, typename Ref, typename Value, typename YieldRef, typename YieldValue>
class basic_coro_promise_yield : public basic_coro_promise_return<Controller, Ref, Value, YieldRef, YieldValue>
{
	using base = basic_coro_promise_return<Controller, Ref, Value, YieldRef, YieldValue>;

public:
	using base::base;
	using yield_type = YieldRef;
	using yield_reference = yield_type;

	constexpr auto yield_value(YieldRef&& value) noexcept(noexcept(this->emplace_carry(std::declval<YieldRef&&>())))
		-> decltype(auto) requires (std::constructible_from<YieldRef, YieldRef&&>)
	{
		return this->emplace_carry(std::forward<YieldRef>(value));
	}

	constexpr auto yield_value(const YieldRef& value) noexcept(noexcept(this->emplace_carry(std::declval<const YieldRef&>())))
		-> decltype(auto) requires (!std::is_reference_v<YieldRef> && std::copy_constructible<YieldRef>)
	{
		return this->emplace_carry(value);
	}
};

template <typename Controller, typename Ref, typename Value, typename YieldValue>
class basic_coro_promise_yield<Controller, Ref, Value, void, YieldValue> :
	public basic_coro_promise_return<Controller, Ref, Value, void, YieldValue>
{
	using base = basic_coro_promise_return<Controller, Ref, Value, void, YieldValue>;

public:
	using base::base;
	using yield_type = void;
	using yield_reference = empty;

	constexpr auto yield_value(empty = {}) noexcept(noexcept(this->emplace_carry()))
		-> decltype(auto)
	{
		return this->emplace_carry();
	}
};

template <typename Controller, typename Ref, typename Value>
class basic_coro_promise_yield<Controller, Ref, Value, void, void> :
	public basic_coro_promise_return<Controller, Ref, Value, void, void>
{
	using base = basic_coro_promise_return<Controller, Ref, Value, void, void>;

public:
	using base::base;
};

}

namespace coro
{

template <typename Controller, typename Ref, typename Value, typename YieldRef, typename YieldValue>
class basic_coro_promise : public detail::coro::basic_coro_promise_yield<
	Controller, Ref, Value, YieldRef, YieldValue
>
{
private:
	using base = detail::coro::basic_coro_promise_yield<
		Controller, Ref, Value, YieldRef, YieldValue
	>;

public:
	using base::base;
	using base::get_promise_state;
	using typename base::value_type;
	using awaitable = co_awaitable<Ref, Value>;

protected:
	using base::get_awaitable;
};

template <typename Ref, typename Value>
class co_awaitable : public basic_coroutine<Ref, basic_coro_promise<detail::coro::simple_continuation_controller, Ref, Value>>
{
private:
	using base = basic_coroutine<Ref, basic_coro_promise<detail::coro::simple_continuation_controller, Ref, Value>>;
	
public:
	using base::base;
	using base::operator=;
};

template <typename Value, template <typename> typename StateHolder = std::shared_ptr>
class async_promise : private basic_promise<detail::coro::atomic_continuation_controller, Value, void, StateHolder>
{
};

}

}

#endif
