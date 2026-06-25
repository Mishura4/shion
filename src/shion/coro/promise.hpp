//
// Created by miuna on 3/31/2026.
//

#ifndef SHION_CORO_PROMISE_HPP_
#define SHION_CORO_PROMISE_HPP_

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

inline namespace coro
{

}

namespace detail::coro
{

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

template <typename Controller, typename Value, typename ValueRef, typename Carry, typename CarryRef>
struct promise_storage_base : public basic_result<wrapper<Value, ValueRef>, wrapper<Carry, CarryRef>>
{
	SHION_NO_UNIQUE_ADDRESS Controller controller;
	
	using value_storage = wrapper<Value, ValueRef>;
	using carry_storage = wrapper<Carry, CarryRef>;
	
	constexpr bool ready() noexcept
	{
		return !this->empty();
	}
};

template <typename Controller, typename Value, typename ValueRef, typename Carry, typename CarryRef>
struct promise_value_layer : public promise_storage_base<Controller, Value, ValueRef, Carry, CarryRef>
{
	using typename promise_storage_base<Controller, Value, ValueRef, Carry, CarryRef>::value_storage;
	using value_type = typename value_storage::value_type;
	using value_reference = typename value_storage::reference;
	using value_expr_type = value_type;
	using value_expr_move = value_type&&;
	using value_expr_copy = std::add_const_t<value_type>&;
	
	template <typename... Args>
	inline static constexpr bool is_nothrow_set_value =
		noexcept(std::declval<Controller&>().acquire_set_value(std::declval<promise_value_layer&>()).finalize())
		&& std::is_nothrow_constructible_v<value_storage, Args...>;
	
	constexpr auto set_value(value_type&& value) noexcept(is_nothrow_set_value<value_type&&>)
		requires (std::is_move_constructible_v<value_type>)
	{
		return this->template emplace_value(std::move(value));
	}
	
	constexpr auto set_value(std::add_const_t<value_type>& value) noexcept(is_nothrow_set_value<std::add_const_t<value_type>&>)
		requires (std::is_copy_constructible_v<value_type>)
	{
		return this->template emplace_value(value);
	}
	
	constexpr auto return_value(value_expr_move value) noexcept(is_nothrow_set_value<value_expr_move>)
		requires (std::is_move_constructible_v<value_type>)
	{
		return this->template emplace_value(std::move(value));
	}
	
	constexpr auto return_value(value_expr_copy value) noexcept(is_nothrow_set_value<value_expr_copy>)
		requires (std::is_copy_constructible_v<value_type>)
	{
		return this->template emplace_value(value);
	}
	
	template <typename Expr>
	constexpr auto return_value(Expr&& value) noexcept(is_nothrow_set_value<Expr>)
		requires (std::is_constructible_v<value_type, Expr>)
	{
		return this->template emplace_value(std::forward<Expr>(value));
	}
	
	template <typename... Args>
	constexpr void emplace_value(Args&&... args) noexcept(is_nothrow_set_value<Args...>)
		requires (std::constructible_from<value_storage, Args...>)
	{
		auto handle = this->controller.acquire_set_value(*this);
		this->template emplace<0>(std::forward<Args>(args)...);
		handle.finalize();
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

template <typename Controller, typename Value, typename ValueRef, typename Carry, typename CarryRef>
	requires (std::is_reference_v<Value>)
struct promise_value_layer<Controller, Value, ValueRef, Carry, CarryRef> : public promise_storage_base<Controller, Value, ValueRef, Carry, CarryRef>
{
	using typename promise_storage_base<Controller, Value, ValueRef, Carry, CarryRef>::value_storage;
	using value_type = typename value_storage::value_type;
	using value_reference = typename value_storage::reference;
	using value_expr_type = value_type;
	
	template <typename... Args>
	inline static constexpr bool is_nothrow_set_value =
		noexcept(std::declval<Controller&>().acquire_set_value(std::declval<promise_value_layer&>()).finalize())
		&& std::is_nothrow_constructible_v<value_storage, Args...>;
	
	constexpr auto set_value(value_expr_type value) noexcept(is_nothrow_set_value<value_expr_type>)
	{
		return this->template emplace_value(std::forward<value_expr_type>(value));
	}
	
	constexpr auto return_value(value_expr_type value) noexcept(is_nothrow_set_value<value_expr_type>)
	{
		return this->template emplace_value(std::forward<value_expr_type>(value));
	}
	
	template <typename Expr>
	constexpr auto return_value(Expr&& value) noexcept(is_nothrow_set_value<Expr>)
		requires (std::is_convertible_v<Expr, value_expr_type>)
	{
		return this->template emplace_value(std::forward<Expr>(value));
	}
	
	template <typename... Args>
	constexpr void emplace_value(Args&&... args) noexcept(is_nothrow_set_value<Args...>)
		requires (std::constructible_from<value_storage, Args...>)
	{
		auto handle = this->controller.acquire_set_value(*this);
		this->template emplace<0>(std::forward<Args>(args)...);
		handle.finalize();
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

template <typename Controller, typename ValueRef, typename Carry, typename CarryRef>
struct promise_value_layer<Controller, void, ValueRef, Carry, CarryRef> : public promise_storage_base<Controller, void, ValueRef, Carry, CarryRef>
{
	using value_type = void;
	using value_reference = void;
	using value_expr_type = void;
	
	inline static constexpr bool is_nothrow_set_value = noexcept(std::declval<Controller&>().acquire_set_value(std::declval<promise_value_layer&>()).finalize());
	
	constexpr void set_value() noexcept(noexcept(emplace_value()))
	{
		emplace_value();
	}
	
	constexpr void emplace_value() noexcept(is_nothrow_set_value)
	{
		auto handle = Controller::acquire_set_value();
		this->template emplace<0>();
		handle.finalize();
	}
};

template <typename Controller, typename Value, typename ValueRef, typename Carry, typename CarryRef>
struct promise_carry_layer : public promise_value_layer<Controller, Value, ValueRef, Carry, CarryRef>
{
	using typename promise_storage_base<Controller, Value, ValueRef, Carry, CarryRef>::carry_storage;
	using carry_type = typename carry_storage::value_type;
	using carry_reference = typename carry_storage::reference;
	using carry_expr_type = carry_type;
	using carry_expr_move = carry_expr_type&&;
	using carry_expr_copy = std::add_const_t<carry_type>&;
	
	template <typename... Args>
	inline static constexpr bool is_nothrow_set_carry =
		noexcept(std::declval<Controller&>().acquire_set_carry(std::declval<promise_carry_layer&>()).finalize())
		&& std::is_nothrow_constructible_v<carry_storage, Args...>;
	
	constexpr auto set_carry(carry_type&& value) noexcept(is_nothrow_set_carry<carry_type&&>)
		requires (std::is_move_constructible_v<carry_type>)
	{
		return this->template emplace_carry(std::move(value));
	}
	
	constexpr auto set_carry(std::add_const_t<carry_type>& value) noexcept(is_nothrow_set_carry<std::add_const_t<carry_type>&>)
		requires (std::is_copy_constructible_v<carry_type>)
	{
		return this->template emplace_carry(value);
	}
	
	constexpr auto yield_value(carry_type&& value) noexcept(is_nothrow_set_carry<carry_type&&>)
		requires (std::is_move_constructible_v<carry_type>)
	{
		return this->template emplace_carry(std::move(value));
	}
	
	constexpr auto yield_value(std::add_const_t<carry_type>& value) noexcept(is_nothrow_set_carry<std::add_const_t<carry_type>&>)
		requires (std::is_copy_constructible_v<carry_type>)
	{
		return this->template emplace_carry(value);
	}
	
	template <typename Expr>
	constexpr auto yield_value(Expr&& value) noexcept(is_nothrow_set_carry<Expr>)
		requires (std::is_constructible_v<carry_type, Expr>)
	{
		return this->template emplace_carry(std::forward<Expr>(value));
	}
	
	template <typename... Args>
	constexpr auto emplace_carry(Args&&... args) noexcept(is_nothrow_set_carry<Args...>)
		requires (std::constructible_from<carry_storage, Args...>)
	{
		auto handle = this->controller.acquire_set_carry();
		this->template emplace<1>(std::forward<Args>(args)...);
		handle.finalize();
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
		requires requires (carry_storage&& s) { *std::move(s); }
	{
		return *shion::get<1>(std::move(*this));
	}
	
	constexpr auto get_carry() const&& -> decltype(auto)
		requires requires (carry_storage const&& s) { *std::move(s); }
	{
		return *shion::get<1>(std::move(*this));
	}
};

template <typename Controller, typename Value, typename ValueRef, typename Carry, typename CarryRef>
	requires (std::is_reference_v<Carry>)
struct promise_carry_layer<Controller, Value, ValueRef, Carry, CarryRef> : public promise_value_layer<Controller, Value, ValueRef, Carry, CarryRef>
{
	using typename promise_storage_base<Controller, Value, ValueRef, Carry, CarryRef>::carry_storage;
	using carry_type = typename carry_storage::value_type;
	using carry_reference = typename carry_storage::reference;
	using carry_expr_type = carry_type;
	
	template <typename... Args>
	inline static constexpr bool is_nothrow_set_carry =
		noexcept(std::declval<Controller&>().acquire_set_carry(std::declval<promise_carry_layer&>()).finalize())
		&& std::is_nothrow_constructible_v<carry_storage, Args...>;
	
	constexpr auto set_carry(carry_reference value) noexcept(is_nothrow_set_carry<carry_reference>)
	{
		return this->template emplace_carry(std::forward<carry_reference>(value));
	}
	
	constexpr auto yield_value(carry_reference value) noexcept(is_nothrow_set_carry<carry_reference>)
	{
		return this->template emplace_carry(std::forward<carry_reference>(value));
	}
	
	template <typename Expr>
	constexpr auto yield_value(Expr&& value) noexcept(is_nothrow_set_carry<Expr&&>)
		requires (std::convertible_to<Expr, carry_reference>)
	{
		return this->template emplace_carry(std::forward<Expr>(value));
	}
	
	template <typename... Args>
	constexpr auto emplace_carry(Args&&... args) noexcept(is_nothrow_set_carry<Args&&...>)
		requires (std::constructible_from<carry_storage, Args...>)
	{
		auto handle = this->controller.acquire_set_carry();
		this->template emplace<1>(std::forward<Args>(args)...);
		handle.finalize();
		return handle;
	}
	
	constexpr bool has_carry() const noexcept
	{
		return this->index() == 1;
	}
	
	constexpr auto get_carry() const -> decltype(auto)
		requires requires (carry_storage const& s) { *s; }
	{
		return *shion::get<1>(*this);
	}
};

template <typename Controller, typename Value, typename ValueRef, typename CarryRef>
struct promise_carry_layer<Controller, Value, ValueRef, void, CarryRef> : public promise_value_layer<Controller, Value, ValueRef, void, CarryRef>
{
	using carry_type = void;
	using carry_reference = void;
	using carry_expr_type = make_complete<void>;
};

template <typename Controller, typename ReturnRef, typename Return, typename YieldRef, typename Yield>
struct promise_storage : promise_carry_layer<Controller, ReturnRef, Return, YieldRef, Yield>
{
};

template <typename Controller, typename Value, typename YieldRef, typename Yield>
requires (!std::is_void_v<YieldRef>)
struct promise_storage : promise_carry_layer<Controller, void, Value, YieldRef, Yield>
{
private:
	using carry_layer = promise_carry_layer<Controller, void, Value, YieldRef, Yield>;

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

template <typename T>
using promise_type = typename std::remove_cvref<decltype(get_promise(std::declval<T&>()))>::type;

class simple_coro_controller
{
protected:
	/**
	 * @brief Coroutine handle currently awaiting the completion of this promise.
	 */
	coro_handle<> awaiter = nullptr;

public:
	/**
	 * @brief Construct a new promise, with empty result.
	 */
	constexpr simple_coro_controller() = default;

	constexpr bool attach_awaiter(coro_handle<> handle)
	{
		awaiter = handle;
		return true;
	}

	constexpr auto release_awaiter() -> detail::std_coroutine::coroutine_handle<>
	{
		return std::exchange(awaiter, nullptr);
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
	constexpr bool notify_awaiter()
	{
		if (has_awaiter())
		{
			awaiter.resume();
			return true;
		}
		return false;
	}

	struct suspend_and_continue
	{
		simple_coro_controller* self;

		constexpr suspend_and_continue(simple_coro_controller& handler) noexcept : self(&handler) {}

		constexpr static bool await_ready() noexcept { return false; }
		constexpr auto await_suspend(coro_handle<>) const noexcept -> coro_handle<>
		{
			return self->awaiter ? self->awaiter : std::noop_coroutine();
		}
		constexpr static void await_resume() noexcept {}
		constexpr static void finalize() noexcept {}
	};
	
	constexpr auto acquire_set_carry(auto&&...) noexcept
	{
		return suspend_and_continue(*this);
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

class atomic_continuation_controller : protected simple_coro_controller
{
protected:
	using flags = detail::coro::state_flags;

	/**
	 * @brief State of the awaitable tied to this promise.
	 */
	std::atomic<uint8> state = flags::sf_none;

public:
	using simple_coro_controller::has_awaiter;

	bool attach_awaiter(detail::std_coroutine::coroutine_handle<> handle)
	{
		auto previous_flags = state.fetch_or(flags::sf_awaited);
		if (previous_flags & flags::sf_awaited) {
			throw logic_exception("awaitable is already being awaited");
		}
		simple_coro_controller::attach_awaiter(handle);
		return !(previous_flags & flags::sf_ready);
	}

	/**
	 * @brief Notify a currently awaiting coroutine that the result is ready.
	 */
	void notify_awaiter()
	{
		if (state.load(std::memory_order_acquire) & flags::sf_awaited)
		{
			SHION_ASSUME(this->has_awaiter());
			simple_coro_controller::notify_awaiter();
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
};

template <typename Storage, typename Logic>
class promise_state : public Logic, protected Storage
{
	using logic = Logic;

public:
	using storage = Storage;
	using typename storage::value_reference;
	using typename storage::carry_reference;
	using storage::has_exception;
	using logic::logic;
	using logic::operator=;
	using logic::has_awaiter;

	constexpr bool ready() const noexcept
	{
		return logic::ready() && storage::ready();
	}

	constexpr void throw_if_not_empty() const
	{
		if (!storage::empty())
			throw logic_exception("promise already has a result");
	}

	/**
	 * @brief Set this promise to an exception and resume any awaiter.
	 *
	 * @tparam Notify Whether to resume any awaiter or not.
	 * @throws logic_exception if the promise is not empty.
	 */
	template <bool Notify = true>
	constexpr void set_exception(std::exception_ptr ptr) {
		throw_if_not_empty();
		storage::set_exception(std::move(ptr));
		logic::mark_ready(Notify);
	}

	/**
	 * @brief Construct the result in place by forwarding the arguments, and by default resume any awaiter.
	 *
	 * @tparam Notify Whether to resume any awaiter or not.
	 * @throws logic_exception if the promise is not empty.
	 */
	template <bool Notify = true>
	requires (!std::is_void<value_reference>::value)
	constexpr void set_value(non_void<value_reference> value) {
		this->template emplace_value<Notify>(std::forward<value_reference>(value));
	}

	/**
	 * @brief Construct the result in place by forwarding the arguments, and by default resume any awaiter.
	 *
	 * @tparam Notify Whether to resume any awaiter or not.
	 * @throws logic_exception if the promise is not empty.
	 */
	template <bool Notify = true>
	requires (std::is_void<value_reference>::value)
	constexpr void set_value() {
		this->template emplace_value<Notify>();
	}

	/**
	 * @brief Construct the result in place by forwarding the arguments, and by default resume any awaiter.
	 *
	 * @tparam Notify Whether to resume any awaiter or not.
	 * @throws logic_exception if the promise is not empty.
	 */
	template <bool Notify = true, typename... Args>
	requires (std::constructible_from<make_complete<value_reference>, Args...>)
	constexpr void emplace_value(Args&&... args) {
		throw_if_not_empty();
		storage::emplace_value(std::forward<value_reference>(args)...);
		logic::mark_ready(Notify);
	}

	/**
	 * @brief Construct the result in place by forwarding the arguments, and by default resume any awaiter.
	 *
	 * @tparam Notify Whether to resume any awaiter or not.
	 * @throws logic_exception if the promise is not empty.
	 */
	template <bool Notify = true>
	requires (!std::is_void<carry_reference>::value)
	constexpr void set_carry(non_void<carry_reference> value) {
		this->template emplace_value<Notify>(std::forward<carry_reference>(value));
	}

	/**
	 * @brief Construct the result in place by forwarding the arguments, and by default resume any awaiter.
	 *
	 * @tparam Notify Whether to resume any awaiter or not.
	 * @throws logic_exception if the promise is not empty.
	 */
	template <bool Notify = true, typename... Args>
	requires (!std::is_void<carry_reference>::value && std::constructible_from<make_complete_t<carry_reference>::type, Args...>)
	constexpr void emplace_carry(Args&&... args) {
		throw_if_not_empty();
		storage::emplace_carry(std::forward<carry_reference>(args)...);
		logic::mark_ready(Notify);
	}

	constexpr bool has_value() const noexcept
	{
		return storage::has_return();
	}
};

template <typename State>
struct simple_state_holder
{
	using state_type = std::remove_cvref_t<std::remove_pointer_t<State>>;

	State state_ptr{};

	constexpr bool valid() const noexcept
	{
		return static_cast<bool>(state_ptr);
	}

	constexpr auto get_promise() noexcept -> decltype(auto)
	{
		return state_ptr;
	}

	constexpr auto get_promise() const noexcept -> decltype(auto)
	{
		return state_ptr;
	}

	constexpr void release() noexcept
	{
		state_ptr = {};
	}
};

template <typename State>
struct simple_state_holder<State*>
{
	using state_type = State;

	State state_ptr{};

	constexpr bool valid() const noexcept
	{
		return static_cast<bool>(state_ptr);
	}

	constexpr auto get_promise() const noexcept -> decltype(auto)
	{
		return *state_ptr;
	}

	constexpr void release() noexcept
	{
		state_ptr = {};
	}

	static constexpr void destroy()
	{
		SHION_ASSERT(false, "Not implemented");
	}
};

template <typename State>
struct simple_state_holder<std::unique_ptr<State>>
{
	using state_type = State;

	State state_ptr{};

	constexpr bool valid() const noexcept
	{
		return static_cast<bool>(state_ptr);
	}

	constexpr auto get_promise() const noexcept -> decltype(auto)
	{
		return *state_ptr;
	}

	constexpr void release() noexcept
	{
		state_ptr = {};
	}
};

template <typename State>
struct simple_state_holder<std::shared_ptr<State>>
{
	using state_type = State;

	State state_ptr{};

	constexpr bool valid() const noexcept
	{
		return static_cast<bool>(state_ptr);
	}

	constexpr auto get_promise() const noexcept -> decltype(auto)
	{
		return *state_ptr;
	}

	constexpr void release() noexcept
	{
		state_ptr = {};
	}
};

template <typename T>
struct simple_state_holder<coro_handle<T>>
{
	using state_type = T;

	coro_handle<T> state_ptr{};

	constexpr simple_state_holder() noexcept = default;
	constexpr simple_state_holder(const simple_state_holder&) noexcept = delete;
	constexpr simple_state_holder(simple_state_holder&& other) noexcept :
		state_ptr(std::exchange(other.state_ptr, nullptr))
	{}

	simple_state_holder(coro_handle<T> handle) noexcept :
		state_ptr(handle)
	{

	}

	constexpr auto operator=(const simple_state_holder&) noexcept -> simple_state_holder& = delete;
	constexpr auto operator=(simple_state_holder&& other) noexcept -> simple_state_holder&
	{
		release();
		state_ptr = std::exchange(other.state_ptr, nullptr);
		return *this;
	}

	~simple_state_holder() noexcept
	{
		release();
	}

	constexpr bool valid() const noexcept
	{
		return static_cast<bool>(state_ptr);
	}

	constexpr auto get_promise() const noexcept -> decltype(auto)
	{
		return state_ptr.promise();
	}

	constexpr bool release() noexcept
	{
		if (state_ptr)
		{
			on_scope_exit on_exit = [this]() noexcept
			{
				state_ptr = nullptr;
			};

			if (this->get_promise().abandon())
			{
				state_ptr.destroy();
				return true;
			}
		}
		return false;
	}
};

template <typename StateHolder>
class awaitable_impl : protected StateHolder
{
protected:
	using shared_state_holder = StateHolder;
	using shared_state = StateHolder::state_type;
	using storage_type = shared_state::storage;
	friend shared_state;

	using shared_state_holder::shared_state_holder;

public:
	using reference = shared_state::value_reference;
	using value_type = shared_state::value_type;

	/**
	 * @brief Construct an empty awaitable.
	 *
	 * Such an awaitable must be assigned a promise before it can be awaited.
	 */
	constexpr awaitable_impl() = default;

	/**
	 * @brief Copy construction is disabled.
	 */
	constexpr awaitable_impl(const awaitable_impl&) = delete;

	/**
	 * @brief Move from another awaitable.
	 *
	 * @param rhs The awaitable to move from, left in an unspecified state after this.
	 */
	constexpr awaitable_impl(awaitable_impl&& rhs) noexcept = default;

	/**
	 * @brief Destructor.
	 *
	 * May signal to the promise that it was destroyed.
	 */
	constexpr ~awaitable_impl() = default;

	/**
	 * @brief Copy assignment is disabled.
	 */
	constexpr awaitable_impl& operator=(const awaitable_impl&) = delete;

	/**
	 * @brief Move from another awaitable.
	 *
	 * @param rhs The awaitable to move from, left in an unspecified state after this.
	 * @return *this
	 */
	constexpr awaitable_impl& operator=(awaitable_impl&& rhs) noexcept = default;

	/**
	 * @brief Check whether this awaitable refers to a valid promise.
	 *
	 * @return bool Whether this awaitable refers to a valid promise or not
	 */
	constexpr bool valid() const noexcept
	{
		return shared_state_holder::valid();
	}

	/**
	 * @brief Check whether or not co_await-ing this would suspend the caller, i.e. if we have the result or not
	 *
	 * @return bool Whether we already have the result or not
	 */
	constexpr bool await_ready() const
	{
		if (!shared_state_holder::valid()) {
			throw logic_exception("cannot co_await an empty awaitable");
		}
		return this->get_promise().ready();
	}

	/**
	 * @brief Second function called by the standard library when co_await-ing this object.
	 *
	 * @throws logic_exception If the awaitable's valid() would return false.
	 * At this point the coroutine frame was allocated and suspended.
	 *
	 * @return bool Whether we do need to suspend or not
	 */
	constexpr bool await_suspend(detail::std_coroutine::coroutine_handle<> handle)
	{
		auto &promise = this->get_promise();
		return promise.attach_awaiter(handle);
	}

	/**
	 * @brief Third and final function called by the standard library when co_await-ing this object, after resuming.
	 *
	 * @throw ? Any exception that occured during the retrieval of the value will be thrown
	 * @return The result.
	 */
	constexpr auto await_resume() -> reference
	{
		SHION_ASSERT(this->valid());
		return this->get_promise().get();
	}
};

template <typename State>
using awaitable = awaitable_impl<simple_state_holder<State>>;

template <typename Reference, typename Value>
using simple_promise_state = promise_state<
	promise_storage<Reference, Value, void, void>,
	simple_coro_controller
>;

template <typename Reference, typename Value>
using async_promise_state = promise_state<
	promise_storage<Reference, Value, void, void>,
	atomic_continuation_controller
>;

template <typename StateHolder>
class simple_promise : protected StateHolder
{
protected:
	using state = StateHolder::state_type;
	using state_holder = StateHolder::state_holder;

public:
	template <typename... Args>
	requires std::constructible_from<typename state::value_type, Args...>
	void set_value(Args&&... args)
	{
		state_holder::get_promise().set_value(std::forward<Args>(args)...);
	}

	void set_exception(std::exception_ptr exception)
	{
		state_holder::get_promise().set_exception(std::move(exception));
	}
};

} // namespace detail::coro

inline namespace coro
{

template <typename Reference, typename Value = void>
class basic_awaitable : public detail::coro::awaitable_impl<detail::coro::simple_promise_state<Reference, Value>>
{
	using base = detail::coro::awaitable_impl<detail::coro::simple_promise_state<Reference, Value>>;
};

template <typename Reference, typename Value = void>
class async_awaitable : public detail::coro::awaitable_impl<detail::coro::async_promise_state<Reference, Value>>
{
	using base = detail::coro::awaitable_impl<detail::coro::simple_promise_state<Reference, Value>>;
};

template <typename Reference, typename Value = void>
class async_promise : public detail::coro::async_promise_state<Reference, Value>
{
	using async_promise_state = detail::coro::async_promise_state<Reference, Value>;

public:
	using async_promise_state::async_promise_state;
	using async_promise_state::operator=;
};

template <typename Reference, typename Value = void>
class simple_promise : public detail::coro::simple_promise_state<Reference, Value>
{
	using base = detail::coro::simple_promise_state<Reference, Value>;

public:
	using base::base;
	using base::operator=;
};

}

}

#endif
