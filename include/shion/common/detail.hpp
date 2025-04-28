#ifndef SHION_COMMON_DETAIL_H_
#define SHION_COMMON_DETAIL_H_

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#include <type_traits>
#include <concepts>
#include <functional>
#endif

namespace SHION_NAMESPACE
{

namespace detail {

template <typename T>
union storage {
	constexpr storage() : dummy{} {}

	template <typename... Args>
	constexpr storage(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) : data(std::forward<Args>(args)...) {
	}

	template <typename U, typename... Args>
	requires (std::invocable<U, Args...> && std::constructible_from<T, std::invoke_result_t<U, Args...>>)
	constexpr storage(U&& fun, Args&&... args)
	noexcept(std::is_nothrow_invocable_v<U, Args...> && std::is_nothrow_constructible_v<U, std::invoke_result_t<U, Args...>>) :
		data(std::invoke(std::forward<U>(fun), std::forward<Args>(args)...)) {
	}

	template <typename... Args>
	constexpr void emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
		 std::construct_at(&data, std::forward<Args>(args)...);
	}

	template <typename U, typename... Args>
	requires (std::invocable<U, Args...> && std::constructible_from<T, std::invoke_result_t<U, Args...>>)
	constexpr void emplace(U&& fun, Args&&... args)
	noexcept(std::is_nothrow_invocable_v<U, Args...> && std::is_nothrow_constructible_v<U, std::invoke_result_t<U, Args...>>) {
		 std::construct_at(&data, std::invoke(std::forward<U>(fun), std::forward<Args>(args)...));
	}

	void destroy() noexcept(std::is_nothrow_destructible_v<T>) {
		std::destroy_at(&data);
	}

	T& get() & noexcept {
		return data;
	}

	T&& get() && noexcept {
		return std::move(data);
	}

	const T& get() const& noexcept {
		return data;
	}

	const T&& get() const&& noexcept {
		return data;
	}

	~storage() {} // Required if T has a non-trivial destructor

	struct {} dummy;
	T         data;
};

template <typename T>
requires (std::is_trivially_destructible_v<T> && !std::is_reference_v<T>)
union storage<T> {
	constexpr storage() : dummy{} {}

	template <typename... Args>
	constexpr storage(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) : data(std::forward<Args>(args)...) {
	}

	template <typename U, typename... Args>
	requires (std::invocable<U, Args...> && std::constructible_from<T, std::invoke_result_t<U, Args...>>)
	constexpr storage(U&& fun, Args&&... args)
	noexcept(std::is_nothrow_invocable_v<U, Args...> && std::is_nothrow_constructible_v<U, std::invoke_result_t<U, Args...>>) :
		data(std::invoke(std::forward<U>(fun), std::forward<Args>(args)...)) {
	}

	template <typename... Args>
	constexpr void emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
		 std::construct_at(&data, std::forward<Args>(args)...);
	}

	template <typename U, typename... Args>
	requires (std::invocable<U, Args...> && std::constructible_from<T, std::invoke_result_t<U, Args...>>)
	constexpr void emplace(U&& fun, Args&&... args)
	noexcept(std::is_nothrow_invocable_v<U, Args...> && std::is_nothrow_constructible_v<U, std::invoke_result_t<U, Args...>>) {
		 std::construct_at(&data, std::invoke(std::forward<U>(fun), std::forward<Args>(args)...));
	}

	void destroy() noexcept(std::is_nothrow_destructible_v<T>) {
		std::destroy_at(&data);
	}

	T& get() & noexcept {
		return data;
	}

	T&& get() && noexcept {
		return std::move(data);
	}

	const T& get() const& noexcept {
		return data;
	}

	const T&& get() const&& noexcept {
		return data;
	}

	struct {} dummy;
	T     data;
};

template <typename T>
union storage<T&> {
	constexpr storage() : dummy{} {}

	constexpr storage(T& value) noexcept : data(&value) {
	}

	template <typename U, typename... Args>
	requires (std::invocable<U, Args...> && std::is_constructible_v<T&, std::invoke_result<U, Args...>>)
	constexpr storage(U&& fun, Args&&... args) noexcept(std::is_nothrow_invocable_v<U, Args...>) : data(std::invoke(std::forward<U>(fun), std::forward<Args>(args)...)) {
	}

	constexpr void emplace(T&& value) noexcept {
		std::construct_at(&data, &value);
	}

	template <typename U, typename... Args>
	requires (std::invocable<U, Args...> && std::is_constructible_v<T&&, std::invoke_result<U, Args...>>)
	constexpr void emplace(U&& fun, Args&&... args) noexcept(std::is_nothrow_invocable_v<U, Args...>) {
		std::construct_at(&data, &std::invoke(std::forward<U>(fun), std::forward<Args>(args)...));
	}

	void destroy() noexcept(std::is_nothrow_destructible_v<T>) {
		std::destroy_at(&data);
	}

	T& get() noexcept {
		return *data;
	}

	T& get() const noexcept {
		return *data;
	}

	struct {}                                      dummy;
	std::add_pointer_t<std::remove_reference_t<T>> data;
};

template <typename T>
union storage<T&&> {
	constexpr storage() : dummy{} {}

	constexpr storage(T&& value) noexcept : data(&value) {
	}

	template <typename U, typename... Args>
	requires (std::invocable<U, Args...> && std::is_constructible_v<T&&, std::invoke_result<U, Args...>>)
	constexpr storage(U&& fun, Args&&... args) noexcept(std::is_nothrow_invocable_v<U, Args...>) : data(&std::invoke(std::forward<U>(fun), std::forward<Args>(args)...)) {
	}

	constexpr void emplace(T&& value) noexcept {
		std::construct_at(&data, &value);
	}

	template <typename U, typename... Args>
	requires (std::invocable<U, Args...> && std::is_constructible_v<T&&, std::invoke_result<U, Args...>>)
	constexpr void emplace(U&& fun, Args&&... args) noexcept(std::is_nothrow_invocable_v<U, Args...>) {
		std::construct_at(&data, &std::invoke(std::forward<U>(fun), std::forward<Args>(args)...));
	}

	void destroy() noexcept(std::is_nothrow_destructible_v<T>) {
		std::destroy_at(&data);
	}

	T&& get() noexcept {
		return std::move(*data);
	}

	T&& get() const noexcept {
		return std::move(*data);
	}

	struct {}                                      dummy;
	std::add_pointer_t<std::remove_reference_t<T>> data;
};

}

}

#endif
