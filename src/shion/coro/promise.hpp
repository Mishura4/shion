//
// Created by miuna on 3/31/2026.
//

#ifndef SHION_CORO_PROMISE_HPP_
#define SHION_CORO_PROMISE_HPP_

#include <shion/common/defines.hpp>

#include "promise.hpp"

#include "shion/coro.hpp"

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

using namespace SHION_NAMESPACE::coro;

template <typename Value>
class promise_value_storage_impl
{
public:
	using value_type = typename std::remove_reference<Value>::type;
	using const_value_type = typename std::add_const<typename std::remove_reference<Value>::type>::type;

	constexpr promise_value_storage_impl() = default;
	template <typename... Args>
	constexpr promise_value_storage_impl(Args&&... args) noexcept(std::is_nothrow_constructible_v<Value, Args...>) :
		_value(std::forward<Args>(args)...)
	{
	}

	template <typename T>
	requires (std::assignable_from<Value, T>)
	constexpr auto operator=(T&& arg) noexcept(std::is_nothrow_assignable_v<Value, T>)
	{
		_value = std::forward<T>(arg);
	}

	constexpr auto operator<=>(promise_value_storage_impl const&) const = default;

protected:
	SHION_INTRINSIC constexpr auto _get() noexcept -> value_type& { return _value; }
	SHION_INTRINSIC constexpr auto _get() const noexcept -> value_type const& { return _value; }

	value_type _value;
};

template <typename Value>
	requires (std::is_reference_v<Value>)
class promise_value_storage_impl<Value>
{
public:
	using value_type = typename std::remove_reference<Value>::type;
	using const_value_type = typename std::add_const<typename std::remove_reference<Value>::type>::type;
	using pointer_type = typename std::add_pointer<value_type>::type;
	using const_pointer_type = const typename std::add_pointer<value_type>::type;
	using storage_type = typename std::add_pointer<value_type>::type;

	constexpr promise_value_storage_impl() = default;
	constexpr promise_value_storage_impl(Value&& ref) noexcept :
		_value(&ref)
	{
	}

	constexpr auto operator=(Value&& ref) noexcept
	{
		_value = &ref;
	}

	constexpr auto operator<=>(promise_value_storage_impl const&) const = default;

protected:
	SHION_INTRINSIC constexpr auto _get() const noexcept -> value_type& { return *_value; }

	pointer_type _value;
};

template <>
class promise_value_storage_impl<void>
{
public:
	using value_type = void;
	using const_value_type = void;
	using storage_type = void;

	constexpr auto operator<=>(promise_value_storage_impl const&) const = default;
};

template <typename Reference, typename Value>
using promise_value_storage = promise_value_storage_impl<
	typename std::conditional<std::is_void_v<Value>, Reference, Value
>::type>;

}

inline namespace coro
{

template <typename Reference, typename Value = void>
class promise_value;

template <typename Value>
class promise_value<void, Value> : public detail::coro::promise_value_storage<void, Value>
{
	using storage = detail::coro::promise_value_storage<void, Value>;

public:
	using typename storage::value_type;
	using typename storage::const_value_type;
	using storage::storage;
	using storage::operator=;
	using storage::operator<=>;
	using reference = void;
	using pointer_type = void;
};

template <>
class promise_value<void, void> : public detail::coro::promise_value_storage<void, void>
{
	using storage = detail::coro::promise_value_storage<void, void>;

public:
	using typename storage::value_type;
	using typename storage::const_value_type;
	using storage::storage;
	using storage::operator=;
	using storage::operator<=>;
	using reference = void;
	using pointer_type = void;
};

template <typename Reference>
	requires (!std::is_reference_v<Reference>)
class promise_value<Reference, void> : public detail::coro::promise_value_storage<Reference, void>
{
	using storage = detail::coro::promise_value_storage<Reference, void>;

public:
	using typename storage::value_type;
	using typename storage::const_value_type;
	using storage::storage;
	using storage::operator=;
	using storage::operator<=>;
	using reference = value_type&;
	using rvalue_reference = value_type&&;
	using const_reference = const value_type&;
	using const_rvalue_reference = const value_type&&;
	using pointer_type = value_type*;
	using const_pointer_type = value_type const*;

	constexpr auto operator*() & noexcept -> value_type& { return this->_get(); }
	constexpr auto operator*() && noexcept -> value_type&& { return std::move(this->_get()); }
	constexpr auto operator*() const& noexcept -> const value_type& { return this->_get(); }
	constexpr auto operator*() const&& noexcept -> const value_type&& { return std::move(this->_get()); }

	constexpr auto operator->()  noexcept -> value_type& { return this->_get(); }
	constexpr auto operator->() const noexcept -> const value_type& { return std::move(this->_get()); }
};

template <typename Reference, typename Value>
class promise_value : public detail::coro::promise_value_storage<Reference, Value>
{
	using storage = detail::coro::promise_value_storage<Reference, Value>;

public:
	using typename storage::value_type;
	using typename storage::const_value_type;
	using storage::storage;
	using storage::operator=;
	using storage::operator<=>;
	using reference = Reference;

	constexpr auto operator*() & noexcept(noexcept(static_cast<Reference>(std::declval<value_type&>()))) -> Reference
		requires requires (value_type& v) { static_cast<Reference>(v); }
	{
		return static_cast<Reference>(this->_get());
	}

	constexpr auto operator*() && noexcept(noexcept(static_cast<Reference>(std::declval<value_type&&>()))) -> Reference
		requires requires (value_type&& v) { static_cast<Reference>(static_cast<value_type&&>(v)); }
	{
		return static_cast<Reference>(this->_get());
	}

	constexpr auto operator*() const& noexcept(noexcept(static_cast<Reference>(std::declval<const_value_type&>()))) -> Reference
		requires requires (const_value_type& v) { static_cast<Reference>(v); }
	{
		return static_cast<Reference>(this->_get());
	}

	constexpr auto operator*() const&& noexcept(noexcept(static_cast<Reference>(std::declval<const_value_type&&>()))) -> Reference
		requires requires (const_value_type&& v) { static_cast<Reference>(static_cast<const_value_type&&>(v)); }
	{
		return static_cast<Reference>(std::move(this->_get()));
	}
};

}

namespace detail::coro
{

template <typename ReturnRef, typename Return, typename YieldRef, typename Yield>
struct promise_storage_base
{
	using return_storage = promise_value<ReturnRef, Return>;
	using yield_storage = promise_value<YieldRef, Yield>;
	using yield_expr_t = typename yield_storage::value_type;
	using yield_value_t = typename yield_storage::value_type;
	using yield_reference_t = typename yield_storage::reference;
	using return_expr_t = typename return_storage::value_type;
	using return_value_t = typename return_storage::value_type;
	using return_reference_t = typename return_storage::reference;

	/**
	 * @brief Variant representing one of either 3 states of the result value : empty, result, exception.
	*/
	using storage_type = std::variant<std::monostate, std::exception_ptr, return_storage, yield_storage>;

	inline static constexpr auto INDEX_EMPTY = 0zu;
	inline static constexpr auto INDEX_EXCEPTION = 1zu;
	inline static constexpr auto INDEX_RETURN = 2zu;
	inline static constexpr auto INDEX_YIELD = 3zu;

	/**
	 * @brief State of the result value.
	 *
	 * @see variant_type
	 */
	storage_type _storage;
};

template <typename ReturnRef, typename Return>
struct promise_storage_base<ReturnRef, Return, ReturnRef, Return>
{
	using return_storage = promise_value<ReturnRef, Return>;
	using yield_storage = void;
	using yield_expr_t = void;
	using yield_value_t = void;
	using yield_reference_t = void;
	using return_expr_t = typename return_storage::value_type;
	using return_value_t = typename return_storage::value_type;
	using return_reference_t = typename return_storage::reference;
	using storage_type = std::variant<std::monostate, std::exception_ptr, return_storage>;

	inline static constexpr auto INDEX_EMPTY = 0zu;
	inline static constexpr auto INDEX_EXCEPTION = 1zu;
	inline static constexpr auto INDEX_RETURN = 2zu;

	storage_type _storage;
};

template <typename ReturnRef, typename Return, typename YieldRef, typename Yield>
struct promise_storage_return_layer : promise_storage_base<ReturnRef, Return, YieldRef, Yield>
{
	using base = promise_storage_base<ReturnRef, Return, YieldRef, Yield>;
	using typename base::return_storage;
	using typename base::yield_storage;

	template <typename... Args>
	constexpr auto set_return(Args&&... args) noexcept(std::is_nothrow_constructible_v<return_storage, Args...>)
		-> decltype(auto)
	{
		this->clear();
		return base::_storage.template emplace<base::INDEX_RETURN>(std::forward<Args>(args)...);
	}

	constexpr auto set_exception(std::exception_ptr ptr) noexcept -> std::exception_ptr&
	{
		this->clear();
		return base::_storage.template emplace<base::INDEX_EXCEPTION>(std::move(ptr));
	}

	constexpr void clear() noexcept
	{
		base::_storage.template emplace<base::INDEX_EMPTY>();
	}

	constexpr auto get_return() & noexcept(noexcept(*std::declval<return_storage&>())) -> decltype(auto)
	{
		return *std::get<base::INDEX_RETURN>(base::_storage);
	}

	constexpr auto get_return() && noexcept(noexcept(*std::declval<return_storage&&>())) -> decltype(auto)
	{
		return *std::move(std::get<base::INDEX_RETURN>(base::_storage));
	}

	constexpr auto get_return() const& noexcept(noexcept(*std::declval<return_storage const&>())) -> decltype(auto)
	{
		return *std::get<base::INDEX_RETURN>(base::_storage);
	}

	constexpr auto get_return() const&& noexcept(noexcept(*std::declval<return_storage const&&>())) -> decltype(auto)
	{
		return *std::move(std::get<base::INDEX_RETURN>(base::_storage));
	}

	constexpr bool empty() const noexcept
	{
		return base::_storage.index() == base::INDEX_EMPTY;
	}

	constexpr bool ready() const noexcept
	{
		return !base::empty();
	}

	constexpr bool has_return() const noexcept
	{
		return base::_storage.index() == base::INDEX_RETURN;
	}

	constexpr bool has_exception() const noexcept
	{
		return base::_storage.index() == base::INDEX_EXCEPTION;
	}

	/**
	 * @brief Check if the result is empty, throws otherwise.
	 *
	 * @throw logic_exception if the result isn't empty.
	 */
	constexpr void throw_if_not_empty() const
	{
		if (empty())
		{
			throw internal_exception("cannot set a value on a promise that already has one");
		}
	}
};

template <typename ReturnRef, typename Return, typename YieldRef, typename Yield>
struct promise_storage : promise_storage_return_layer<ReturnRef, Return, YieldRef, Yield>
{
	using base = promise_storage_return_layer<ReturnRef, Return, YieldRef, Yield>;
	using typename base::yield_storage;
	using typename base::return_storage;

	template <typename... Args>
	constexpr auto set_yield(Args&&... args) noexcept(std::is_nothrow_constructible_v<yield_storage, Args...>)
		-> decltype(auto)
	{
		this->clear();
		return base::_storage.template emplace<base::INDEX_YIELD>(std::forward<Args>(args)...);
	}

	constexpr auto get_yield() & noexcept(noexcept(*std::declval<yield_storage&>())) -> decltype(auto)
	{
		return **std::get_if<base::INDEX_YIELD>(std::addressof(base::_storage));
	}

	constexpr auto get_yield() && noexcept(noexcept(*std::declval<yield_storage&&>())) -> decltype(auto)
	{
		return *std::move(*std::get_if<base::INDEX_YIELD>(std::addressof(base::_storage)));
	}

	constexpr auto get_yield() const& noexcept(noexcept(*std::declval<yield_storage const&>())) -> decltype(auto)
	{
		return **std::get_if<base::INDEX_YIELD>(std::addressof(base::_storage));
	}

	constexpr auto get_yield() const&& noexcept(noexcept(*std::declval<yield_storage const&&>())) -> decltype(auto)
	{
		return *std::move(*std::get_if<base::INDEX_YIELD>(std::addressof(base::_storage)));
	}

	constexpr bool has_yield() noexcept
	{
		return base::_storage.index() == base::INDEX_YIELD;
	}
};

template <typename ReturnRef, typename Return>
struct promise_storage<ReturnRef, Return, void, void> : promise_storage_return_layer<ReturnRef, Return, void, void>
{
	using base = promise_storage_return_layer<ReturnRef, Return, void, void>;
	using typename base::yield_storage;
	using typename base::return_storage;
};

template <typename T>
using promise_type = typename std::remove_cvref<decltype(get_promise(std::declval<T&>()))>::type;

class simple_coro_handler
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
	constexpr simple_coro_handler() = default;

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
		simple_coro_handler* self;

		constexpr suspend_and_continue(simple_coro_handler& handler) noexcept : self(&handler) {}

		constexpr static bool await_ready() noexcept { return false; }
		constexpr auto await_suspend(coro_handle<>) const noexcept -> coro_handle<>
		{
			return self->awaiter ? self->awaiter : std::noop_coroutine();
		}
		constexpr static void await_resume() noexcept {};
	};

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

class atomic_coro_handler : protected simple_coro_handler
{
protected:
	using flags = detail::coro::state_flags;

	/**
	 * @brief State of the awaitable tied to this promise.
	 */
	std::atomic<uint8> state = flags::sf_none;

public:
	using simple_coro_handler::has_awaiter;

	bool attach_awaiter(detail::std_coroutine::coroutine_handle<> handle)
	{
		auto previous_flags = state.fetch_or(flags::sf_awaited);
		if (previous_flags & flags::sf_awaited) {
			throw logic_exception("awaitable is already being awaited");
		}
		simple_coro_handler::attach_awaiter(handle);
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
			simple_coro_handler::notify_awaiter();
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
	using storage = Storage;
	using logic = Logic;

public:
	using typename storage::storage_type;
	using value_type = storage::return_value_t;
	using reference = storage::return_reference_t;
	using storage::has_exception;
	using logic::logic;
	using logic::operator=;
	using logic::has_awaiter;

	constexpr bool ready() const noexcept
	{
		return logic::ready() && storage::ready();
	}

	/**
	 * @brief Set this promise to an exception and resume any awaiter.
	 *
	 * @tparam Notify Whether to resume any awaiter or not.
	 * @throws logic_exception if the promise is not empty.
	 */
	template <bool Notify = true>
	constexpr void set_exception(std::exception_ptr ptr) {
		storage::throw_if_not_empty();
		storage::set_exception(std::move(ptr));
		logic::mark_ready(Notify);
	}

	/**
	 * @brief Construct the result in place by forwarding the arguments, and by default resume any awaiter.
	 *
	 * @tparam Notify Whether to resume any awaiter or not.
	 * @throws logic_exception if the promise is not empty.
	 */
	template <bool Notify = true, typename T = storage::storage_type, typename... Args>
	requires (std::constructible_from<T, Args...>)
	constexpr void set_value(Args&&... args) {
		storage::throw_if_not_empty();
		storage::set_return(std::forward<Args>(args)...);
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
	using storage_type = shared_state::storage_type;
	friend shared_state;

	using shared_state_holder::shared_state_holder;

public:
	using reference = shared_state::reference;
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
	simple_coro_handler
>;

template <typename Reference, typename Value>
using async_promise_state = promise_state<
	promise_storage<Reference, Value, void, void>,
	atomic_coro_handler
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
