#pragma once

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#  if !SHION_IMPORT_STD
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
#  endif

#  include <shion/coro/coro.hpp>
#  include <shion/common.hpp>

#endif

namespace SHION_NAMESPACE {

SHION_EXPORT struct awaitable_dummy {
	int *promise_dummy = nullptr;
};

namespace detail::promise {

template <typename T>
using result_t = std::variant<std::monostate, std::conditional_t<std::is_void_v<T>, empty, T>, std::exception_ptr>;

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

template <typename T>
class promise_base;

/**
 * @brief Empty result from void-returning awaitable
 */
struct empty{};

template <typename T>
void spawn_sync_wait_job(auto* awaitable, std::condition_variable &cv, auto&& result);

} /* namespace detail::promise */

SHION_EXPORT template <awaitable_type Derived>
class basic_awaitable {
protected:
	template <bool Timed>
	auto sync_wait_impl(auto&& do_wait) {
		using result_type = decltype(detail::co_await_resolve(std::declval<Derived>()).await_resume());
		using storage_type = std::conditional_t<std::is_void_v<result_type>, detail::promise::empty, result_type>;
		using variant_type = std::variant<std::monostate, storage_type, std::exception_ptr>;
		variant_type result;
		std::condition_variable cv;

		detail::promise::spawn_sync_wait_job<result_type>(static_cast<Derived*>(this), cv, result);
		do_wait(cv, result);
		if (result.index() == 2) {
			std::rethrow_exception(std::get<2>(result));
		}
		if constexpr (!Timed) { // no timeout
			if constexpr (!std::is_void_v<result_type>) {
				return std::get<1>(result);
			}
		} else { // timeout
			if constexpr (std::is_void_v<result_type>) {
				return result.index() == 1 ? true : false;
			} else {
				return result.index() == 1 ? std::optional<result_type>{std::get<1>(result)} : std::nullopt;
			}
		}
	}

public:
	/**
	 * @brief Blocks this thread and waits for the awaitable to finish.
	 *
	 * @attention This will BLOCK THE THREAD. It is likely you want to use co_await instead.
	 */
	auto sync_wait() {
		return sync_wait_impl<false>([](std::condition_variable &cv, auto&& result) {
			std::mutex m{};
			std::unique_lock lock{m};
			cv.wait(lock, [&result] { return result.index() != 0; });
		});
	}

	/**
	 * @brief Blocks this thread and waits for the awaitable to finish.
	 *
	 * @attention This will BLOCK THE THREAD. It is likely you want to use co_await instead.
	 * @param duration Maximum duration to wait for
	 * @retval If T is void, returns a boolean for which true means the awaitable completed, false means it timed out.
	 * @retval If T is non-void, returns a std::optional<T> for which an absense of value means timed out.
	 */
	template <class Rep, class Period>
	auto sync_wait_for(const std::chrono::duration<Rep, Period>& duration) {
		return sync_wait_impl<true>([duration](std::condition_variable &cv, auto&& result) {
			std::mutex m{};
			std::unique_lock lock{m};
			cv.wait_for(lock, duration, [&result] { return result.index() != 0; });
		});
	}

	/**
	 * @brief Blocks this thread and waits for the awaitable to finish.
	 *
	 * @attention This will BLOCK THE THREAD. It is likely you want to use co_await instead.
	 * @param time Maximum time point to wait for
	 * @retval If T is void, returns a boolean for which true means the awaitable completed, false means it timed out.
	 * @retval If T is non-void, returns a std::optional<T> for which an absense of value means timed out.
	 */
	template <class Clock, class Duration>
	auto sync_wait_until(const std::chrono::time_point<Clock, Duration> &time) {
		return sync_wait_impl<true>([time](std::condition_variable &cv, auto&& result) {
			std::mutex m{};
			std::unique_lock lock{m};
			cv.wait_until(lock, time, [&result] { return result.index() != 0; });
		});
	}
};

/**
 * @brief Generic awaitable class, represents a future value that can be co_await-ed on.
 *
 * Roughly equivalent of std::future for coroutines, with the crucial distinction that the future does not own a reference to a "shared state".
 * It holds a non-owning reference to the promise, which must be kept alive for the entire lifetime of the awaitable.
 *
 * @tparam T Type of the asynchronous value
 * @see promise
 */
SHION_EXPORT template <typename T>
class awaitable : public basic_awaitable<awaitable<T>> {
protected:
	friend class detail::promise::promise_base<T>;

	using shared_state = detail::promise::promise_base<T>;
	using state_flags = detail::promise::state_flags;

	/**
	 * @brief Non-owning pointer to the promise, which must be kept alive for the entire lifetime of the awaitable.
	 */
	shared_state *state_ptr = nullptr;

	/**
	 * @brief Construct from a promise.
	 *
	 * @param promise The promise to refer to.
	 */
	awaitable(shared_state *promise) noexcept : state_ptr{promise} {}

	/**
	 * @brief Abandons the promise.
	 *
	 * Set the promise's state to broken and unlinks this awaitable.
	 *
	 * @return uint8_t Flags previously held before setting them to broken
	 */
	uint8_t abandon();

	/**
	 * @brief Awaiter returned by co_await.
	 *
	 * Contains the await_ready, await_suspend and await_resume functions required by the C++ standard.
	 * This class is CRTP-like, in that it will refer to an object derived from awaitable.
	 *
	 * @tparam Derived Type of reference to refer to the awaitable.
	 */
	template <typename Derived>
	struct awaiter {
		Derived awaitable_obj;

		/**
		 * @brief First function called by the standard library when co_await-ing this object.
		 *
		 * @throws logic_exception If the awaitable's valid() would return false.
		 * @return bool Whether the result is ready, in which case we don't need to suspend
		 */
		bool await_ready() const;

		/**
		 * @brief Second function called by the standard library when co_await-ing this object.
		 *
		 * @throws logic_exception If the awaitable's valid() would return false.
		 * At this point the coroutine frame was allocated and suspended.
		 *
		 * @return bool Whether we do need to suspend or not
		 */
		bool await_suspend(detail::std_coroutine::coroutine_handle<> handle);

		/**
		 * @brief Third and final function called by the standard library when co_await-ing this object, after resuming.
		 *
		 * @throw ? Any exception that occured during the retrieval of the value will be thrown
		 * @return T The result.
		 */
		T await_resume();
	};

public:
	/**
	 * @brief Construct an empty awaitable.
	 *
	 * Such an awaitable must be assigned a promise before it can be awaited.
	 */
	awaitable() = default;

	/**
	 * @brief Copy construction is disabled.
	 */
	awaitable(const awaitable&) = delete;

	/**
	 * @brief Move from another awaitable.
	 *
	 * @param rhs The awaitable to move from, left in an unspecified state after this.
	 */
	awaitable(awaitable&& rhs) noexcept : state_ptr(std::exchange(rhs.state_ptr, nullptr)) {
	}

	/**
	 * @brief Destructor.
	 *
	 * May signal to the promise that it was destroyed.
	 */
	~awaitable();

	/**
	 * @brief Copy assignment is disabled.
	 */
	awaitable& operator=(const awaitable&) = delete;

	/**
	 * @brief Move from another awaitable.
	 *
	 * @param rhs The awaitable to move from, left in an unspecified state after this.
	 * @return *this
	 */
	awaitable& operator=(awaitable&& rhs) noexcept {
		state_ptr = std::exchange(rhs.state_ptr, nullptr);
		return *this;
	}

	/**
	 * @brief Check whether this awaitable refers to a valid promise.
	 *
	 * @return bool Whether this awaitable refers to a valid promise or not
	 */
	bool valid() const noexcept;

	/**
	 * @brief Check whether or not co_await-ing this would suspend the caller, i.e. if we have the result or not
	 *
	 * @return bool Whether we already have the result or not
	 */
	bool await_ready() const;

	/**
	 * @brief Overload of the co_await operator.
	 *
	 * @return Returns an @ref awaiter referencing this awaitable.
	 */
	template <typename Derived>
	requires (std::is_base_of_v<awaitable, std::remove_cv_t<Derived>>)
	friend awaiter<Derived&> operator co_await(Derived& obj) noexcept {
		return {obj};
	}

	/**
	 * @brief Overload of the co_await operator. Returns an @ref awaiter referencing this awaitable.
	 *
	 * @return Returns an @ref awaiter referencing this awaitable.
	 */
	template <typename Derived>
	requires (std::is_base_of_v<awaitable, std::remove_cv_t<Derived>>)
	friend awaiter<Derived&&> operator co_await(Derived&& obj) noexcept {
		return {std::move(obj)};
	}
};

namespace detail::promise {

template <typename T>
class promise_base {
protected:
	friend class awaitable<T>;

	/**
	 * @brief Variant representing one of either 3 states of the result value : empty, result, exception.
	 */
	using storage_type = std::variant<std::monostate, std::conditional_t<std::is_void_v<T>, empty, T>, std::exception_ptr>;

	/**
	 * @brief State of the result value.
	 *
	 * @see storage_type
	 */
	storage_type value = std::monostate{};

	/**
	 * @brief State of the awaitable tied to this promise.
	 */
	std::atomic<uint8_t> state = sf_none;

	/**
	 * @brief Coroutine handle currently awaiting the completion of this promise.
	 */
	std_coroutine::coroutine_handle<> awaiter = nullptr;

	/**
	 * @brief Check if the result is empty, throws otherwise.
	 *
	 * @throw logic_exception if the result isn't empty.
	 */
	void throw_if_not_empty() {
		if (value.index() != 0) [[unlikely]] {
			throw internal_exception("cannot set a value on a promise that already has one");
		}
	}

	std_coroutine::coroutine_handle<> release_awaiter() {
		return std::exchange(awaiter, nullptr);
	}

	/**
	 * @brief Construct a new promise, with empty result.
	 */
	promise_base() = default;

	/**
	 * @brief Copy construction is disabled.
	 */
	promise_base(const promise_base&) = delete;

	/**
	 * @brief Move construction is disabled.
	 */
	promise_base(promise_base&& rhs) = delete;

public:
	/**
	 * @brief Copy assignment is disabled.
	 */
	promise_base &operator=(const promise_base&) = delete;

	/**
	 * @brief Move assignment is disabled.
	 */
	promise_base &operator=(promise_base&& rhs) = delete;

	/**
	 * @brief Set this promise to an exception and resume any awaiter.
	 *
	 * @tparam Notify Whether to resume any awaiter or not.
	 * @throws logic_exception if the promise is not empty.
	 */
	template <bool Notify = true>
	void set_exception(std::exception_ptr ptr) {
		throw_if_not_empty();
		value.template emplace<2>(std::move(ptr));
		[[maybe_unused]] auto previous_value = this->state.fetch_or(sf_ready);
		if constexpr (Notify) {
			if (previous_value & sf_awaited) {
				this->awaiter.resume();
			}
		}
	}

	/**
	 * @brief Notify a currently awaiting coroutine that the result is ready.
	 */
	void notify_awaiter() {
		if (state.load() & sf_awaited) {
			awaiter.resume();
		}
	}

	/**
	 * @brief Get an awaitable object for this promise.
	 *
	 * @throws logic_exception if get_awaitable has already been called on this object.
	 * @return awaitable<T> An object that can be co_await-ed to retrieve the value of this promise.
	 */
	awaitable<T> get_awaitable() {
		uint8_t previous_flags = state.fetch_or(sf_has_awaitable);
		if (previous_flags & sf_has_awaitable) [[unlikely]] {
			throw logic_exception{"an awaitable was already created from this promise"};
		}
		return {this};
	}
};

/**
 * @brief Generic promise class, represents the owning potion of an asynchronous value.
 *
 * This class is roughly equivalent to std::promise, with the crucial distinction that the promise *IS* the shared state.
 * As such, the promise needs to be kept alive for the entire time a value can be retrieved.
 *
 * @tparam T Type of the asynchronous value
 * @see awaitable
 */
template <typename T>
class promise : public promise_base<T> {
public:
	using promise_base<T>::promise_base;
	using promise_base<T>::operator=;

	/**
	 * @brief Construct the result in place by forwarding the arguments, and by default resume any awaiter.
	 *
	 * @tparam Notify Whether to resume any awaiter or not.
	 * @throws logic_exception if the promise is not empty.
	 */
	template <bool Notify = true, typename... Args>
	requires (std::constructible_from<T, Args...>)
	void emplace_value(Args&&... args) {
		this->throw_if_not_empty();
		try {
			this->value.template emplace<1>(std::forward<Args>(args)...);
		} catch (...) {
			this->value.template emplace<2>(std::current_exception());
		}
		[[maybe_unused]] auto previous_value = this->state.fetch_or(sf_ready);
		if constexpr (Notify) {
			if (previous_value & sf_awaited) {
				this->awaiter.resume();
			}
		}
	}

	/**
	 * @brief Construct the result by copy, and resume any awaiter.
	 *
	 * @tparam Notify Whether to resume any awaiter or not.
	 * @throws logic_exception if the promise is not empty.
	 */
	template <bool Notify = true>
	void set_value(const T& v) requires (std::copy_constructible<T>) {
		emplace_value<Notify>(v);
	}

	/**
	 * @brief Construct the result by move, and resume any awaiter.
	 *
	 * @tparam Notify Whether to resume any awaiter or not.
	 * @throws logic_exception if the promise is not empty.
	 */
	template <bool Notify = true>
	void set_value(T&& v) requires (std::move_constructible<T>) {
		emplace_value<Notify>(std::move(v));
	}
};

template <>
class promise<void> : public promise_base<void> {
public:
	using promise_base::promise_base;
	using promise_base::operator=;

	/**
	 * @brief Set the promise to completed, and resume any awaiter.
	 *
	 * @throws logic_exception if the promise is not empty.
	 */
	template <bool Notify = true>
	void set_value() {
		throw_if_not_empty();
		this->value.emplace<1>();
		[[maybe_unused]] auto previous_value = this->state.fetch_or(sf_ready);
		if constexpr (Notify) {
			if (previous_value & sf_awaited) {
				this->awaiter.resume();
			}
		}
	}
};

}

SHION_EXPORT template <typename T>
using basic_promise = detail::promise::promise<T>;

/**
 * @brief Base class for a promise type.
 *
 * Contains the base logic for @ref promise, but does not contain the set_value methods.
 */
SHION_EXPORT template <typename T>
class moveable_promise {
	std::unique_ptr<basic_promise<T>> shared_state = std::make_unique<basic_promise<T>>();

public:
	/**
	 * @copydoc basic_promise<T>::emplace_value
	 */
	template <bool Notify = true, typename... Args>
	requires (std::constructible_from<T, Args...>)
	void emplace_value(Args&&... args) {
		shared_state->template emplace_value<Notify>(std::forward<Args>(args)...);
	}

	/**
	 * @copydoc basic_promise<T>::set_value(const T&)
	 */
	template <bool Notify = true>
	void set_value(const T& v) requires (std::copy_constructible<T>) {
		shared_state->template set_value<Notify>(v);
	}

	/**
	 * @copydoc basic_promise<T>::set_value(T&&)
	 */
	template <bool Notify = true>
	void set_value(T&& v) requires (std::move_constructible<T>) {
		shared_state->template set_value<Notify>(std::move(v));
	}

	/**
	 * @copydoc basic_promise<T>::set_value(T&&)
	 */
	template <bool Notify = true>
	void set_exception(std::exception_ptr ptr) {
		shared_state->template set_exception<Notify>(std::move(ptr));
	}

	/**
	 * @copydoc basic_promise<T>::notify_awaiter
	 */
	void notify_awaiter() {
		shared_state->notify_awaiter();
	}

	/**
	 * @copydoc basic_promise<T>::get_awaitable
	 */
	awaitable<T> get_awaitable() {
		return shared_state->get_awaitable();
	}
};

template <>
class moveable_promise<void> {
	std::unique_ptr<basic_promise<void>> shared_state = std::make_unique<basic_promise<void>>();

public:
	/**
	 * @copydoc basic_promise<void>::set_value
	 */
	template <bool Notify = true>
	void set_value() {
		shared_state->set_value<Notify>();
	}

	/**
	 * @copydoc basic_promise<T>::set_exception
	 */
	template <bool Notify = true>
	void set_exception(std::exception_ptr ptr) {
		shared_state->set_exception<Notify>(std::move(ptr));
	}

	/**
	 * @copydoc basic_promise<T>::notify_awaiter
	 */
	void notify_awaiter() {
		shared_state->notify_awaiter();
	}

	/**
	 * @copydoc basic_promise<T>::get_awaitable
	 */
	awaitable<void> get_awaitable() {
		return shared_state->get_awaitable();
	}
};

SHION_EXPORT template <typename T>
using promise = moveable_promise<T>;

template <typename T>
auto awaitable<T>::abandon() -> uint8_t {
	auto previous_state = state_ptr->state.fetch_or(state_flags::sf_broken);
	state_ptr = nullptr;
	return previous_state;
}

template <typename T>
awaitable<T>::~awaitable() {
	if (state_ptr) {
		state_ptr->state.fetch_or(state_flags::sf_broken);
	}
}

template <typename T>
bool awaitable<T>::valid() const noexcept {
	return state_ptr != nullptr;
}

template <typename T>
bool awaitable<T>::await_ready() const {
	if (!this->valid()) {
		throw logic_exception("cannot co_await an empty awaitable");
	}
	uint8_t state = this->state_ptr->state.load(std::memory_order_relaxed);
	return state & detail::promise::sf_ready;
}

template <typename T>
template <typename Derived>
bool awaitable<T>::awaiter<Derived>::await_suspend(std::coroutine_handle<> handle) {
	auto &promise = *awaitable_obj.state_ptr;

	promise.awaiter = handle;
	auto previous_flags = promise.state.fetch_or(detail::promise::sf_awaited);
	if (previous_flags & detail::promise::sf_awaited) {
		throw logic_exception("awaitable is already being awaited");
	}
	return !(previous_flags & detail::promise::sf_ready);
}

template <typename T>
template <typename Derived>
T awaitable<T>::awaiter<Derived>::await_resume() {
	auto &promise = *std::exchange(awaitable_obj.state_ptr, nullptr);

	promise.state.fetch_and(~detail::promise::sf_awaited);
	if (std::holds_alternative<std::exception_ptr>(promise.value)) {
		std::rethrow_exception(std::get<2>(promise.value));
	}
	if constexpr (!std::is_void_v<T>) {
		return std::get<1>(std::move(promise.value));
	} else {
		return;
	}
}

template <typename T>
template <typename Derived>
bool awaitable<T>::awaiter<Derived>::await_ready() const {
	return static_cast<Derived>(awaitable_obj).await_ready();
}

}
