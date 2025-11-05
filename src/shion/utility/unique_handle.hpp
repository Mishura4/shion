#ifndef SHION_UNIQUE_HANDLE_H_
#define SHION_UNIQUE_HANDLE_H_

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#	include <utility>
#	include <optional>
#	include <type_traits>
#	include <functional>

#	include <shion/common.hpp>
#	include <shion/utility/optional.hpp>
#endif

SHION_EXPORT namespace SHION_NAMESPACE {

inline namespace utility {

/**
 * @brief Holds a "maybe" unique object, similarly to std::unique_ptr but can handle non-pointers.
 *
 * Upon moving this object, its state will be set to non-owning.
 */
template <typename T, auto Release>
class unique_handle {
public:
	constexpr unique_handle() = default;
	constexpr unique_handle(const unique_handle&) = delete;
	constexpr unique_handle(unique_handle&& handle) noexcept :
		_handle{std::exchange(handle._handle, std::nullopt)} {
	}

	template <typename... Ts>
	constexpr unique_handle(Ts&&... args) noexcept(std::is_nothrow_constructible_v<T, Ts...>) :
		_handle{std::forward<Ts>(args)...} {
	}

	constexpr ~unique_handle() {
		_destroy();
	}

	explicit operator bool() const noexcept {
		return has_value();
	}

	bool has_value() const noexcept {
		return _handle.has_value();
	}

	constexpr unique_handle& operator=(const unique_handle&) = delete;

	constexpr unique_handle& operator=(unique_handle&& handle) noexcept {
		_destroy();
		_handle = std::exchange(handle._handle, std::nullopt);
		return *this;
	}

	constexpr T* get() const noexcept {
		return has_value() ? &(*_handle) : nullptr;
	}

	constexpr decltype(auto) operator*() & noexcept {
		return *_handle;
	}

	constexpr decltype(auto) operator*() const& noexcept {
		return *_handle;
	}

	constexpr decltype(auto) operator*() && noexcept {
		return *std::move(_handle);
	}

	constexpr decltype(auto) operator*() const&& noexcept {
		return *std::move(_handle);
	}

	constexpr T* operator->() const noexcept {
		return std::addressof(_handle);
	}

	constexpr void swap(unique_handle& other) noexcept {
		_destroy();
		_handle = std::exchange(other._handle, std::nullopt);
	}

	template <typename... Ts>
	constexpr void reset(Ts&&... args) noexcept(std::is_nothrow_constructible_v<T, Ts...>) {
		_destroy();
		_handle = optional<T>{std::forward<Ts>(args)...};
	}

private:
	void _destroy() {
		if constexpr (!std::is_same_v<decltype(Release), std::nullopt_t>) {
			if (has_value())
				std::invoke(Release, *_handle);
		}
	}

	optional<T> _handle{};
};

/**
 * @brief Specialization of unique_handle for pointers. Essentially the same as std::unique_ptr, but the deleter is a constant expression template parameter.
 */
template <typename T, auto Release>
requires (std::is_pointer_v<T> || std::is_same_v<T, bool>)
class unique_handle<T, Release> {
public:
	constexpr unique_handle() = default;
	constexpr unique_handle(const unique_handle&) = delete;
	constexpr unique_handle(unique_handle&& handle) noexcept :
		_handle{std::exchange(handle._handle, {})} {
	}

	template <typename... Ts>
	constexpr unique_handle(Ts&&... args) noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, Ts...>) :
		_handle{std::forward<Ts>(args)...} {
	}

	constexpr ~unique_handle() {
		_destroy();
	}

	constexpr unique_handle& operator=(const unique_handle&) = delete;

	constexpr unique_handle& operator=(unique_handle&& handle) noexcept {
		_destroy();
		_handle = std::exchange(handle._handle, std::decay_t<T>{});
		return *this;
	}

	explicit operator bool() const noexcept {
		return has_value();
	}

	bool has_value() const noexcept {
		return _handle;
	}

	constexpr std::decay_t<T> get() const noexcept {
		return _handle;
	}

	constexpr decltype(auto) operator*() & noexcept {
		return *_handle;
	}

	constexpr decltype(auto) operator*() const& noexcept {
		return *_handle;
	}

	constexpr decltype(auto) operator*() && noexcept {
		return std::move(*_handle);
	}

	constexpr decltype(auto) operator*() const&& noexcept {
		return std::move(*_handle);
	}

	constexpr auto operator->() const noexcept {
		if constexpr (std::is_pointer_v<T>) {
			return _handle;
		} else {
			return std::addressof(_handle);
		}
	}

	constexpr void swap(unique_handle& other) noexcept {
		_destroy();
		_handle = std::exchange(other._handle, std::decay_t<T>{});
	}

	template <typename... Ts>
	constexpr void reset(std::decay_t<T> value) noexcept {
		_destroy();
		_handle = value;
	}

private:
	void _destroy() {
		if constexpr (!std::is_same_v<decltype(Release), std::nullopt_t>) {
			if (has_value())
				std::invoke(Release, _handle);
		}
	}

	std::decay_t<T> _handle;
};

template <typename T, auto Deleter = std::default_delete<T>{}>
using unique_ptr = unique_handle<std::add_pointer_t<T>, Deleter>;

template <typename T>
using unique_value = unique_handle<T, std::nullopt>;

using unique_flag = unique_value<bool>;

template <typename T, std::equality_comparable_with<T> U, auto R1, auto R2>
requires (!std::is_pointer_v<T> && !std::is_pointer_v<U>)
constexpr bool operator==(unique_handle<T, R1> const &lhs, unique_handle<U, R2> const &rhs) noexcept (noexcept(std::declval<T>() == std::declval<U>())) {
	if (lhs.has_value()) {
		if (rhs.has_value()) {
			return *lhs == *rhs;
		}
		return false;
	}
	return rhs.has_value();
}

template <typename T, std::three_way_comparable_with<T> U, auto R1, auto R2>
requires (!std::is_pointer_v<T> && !std::is_pointer_v<U>)
constexpr auto operator<=>(unique_handle<T, R1> const &lhs, unique_handle<U, R2> const &rhs) noexcept (noexcept(std::declval<T>() <=> std::declval<U>())) {
	if (auto result = lhs.has_value() <=> rhs.has_value(); result != std::strong_ordering::equal) {
		return result;
	}
	return *lhs <=> *rhs;
}

template <typename T, typename U, auto R1, auto R2>
requires (std::is_pointer_v<T> && std::is_pointer_v<U>)
constexpr bool operator==(unique_handle<T*, R1> const &lhs, unique_handle<U*, R2> const &rhs) {
	return lhs.get() == rhs.get();
}

template <typename T, typename U, auto R1, auto R2>
constexpr auto operator<=>(unique_handle<T*, R1> const &lhs, unique_handle<U*, R2> const &rhs) {
	return lhs.get() <=> rhs.get();
}

template <typename T, auto R>
constexpr bool operator==(unique_handle<T, R> const &lhs, std::nullopt_t) noexcept {
	return !lhs.has_value();
}

template <typename T, auto R>
constexpr auto operator<=>(unique_handle<T, R> const &lhs, std::nullopt_t) noexcept {
	return lhs.has_value() <=> false;
}

template <typename T, auto R>
constexpr bool operator==(std::nullopt_t, unique_handle<T, R> const &rhs) noexcept {
	return !rhs.has_value();
}

template <typename T, auto R>
constexpr auto operator<=>(std::nullopt_t, unique_handle<T, R> const &rhs) noexcept {
	return false <=> rhs.has_value();
}

template <typename T, auto R>
constexpr bool operator==(unique_handle<T, R> const &lhs, std::nullptr_t) noexcept {
	return !lhs.has_value();
}

template <typename T, auto R>
constexpr auto operator<=>(unique_handle<T, R> const &lhs, std::nullptr_t) noexcept {
	return lhs.has_value() <=> false;
}

template <typename T, auto R>
constexpr bool operator==(std::nullptr_t, unique_handle<T, R> const &rhs) noexcept {
	return !rhs.has_value();
}

template <typename T, auto R>
constexpr auto operator<=>(std::nullptr_t, unique_handle<T, R> const &rhs) noexcept {
	return false <=> rhs.has_value();
}

}

}

#endif
