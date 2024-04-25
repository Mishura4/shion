#ifndef SHION_UNIQUE_HANDLE_H_
#define SHION_UNIQUE_HANDLE_H_

#include <utility>
#include <optional>
#include <type_traits>

#include "../shion_essentials.hpp"

#include "optional.hpp"

namespace shion {

inline namespace utility {

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
	constexpr unique_handle(Ts&&... args) noexcept(std::is_nothrow_constructible_v<T, Ts...>) :
		_handle{std::forward<Ts>(args)...} {
	}

	constexpr ~unique_handle() {
		_destroy();
	}

	constexpr unique_handle& operator=(const unique_handle&) = delete;

	constexpr unique_handle& operator=(unique_handle&& handle) noexcept {
		_destroy();
		_handle = std::exchange(handle._handle, T{});
		return *this;
	}

	explicit operator bool() const noexcept {
		return has_value();
	}

	bool has_value() const noexcept {
		return _handle;
	}

	constexpr T* get() const noexcept {
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

	constexpr void swap(unique_handle& other) noexcept {
		_destroy();
		_handle = std::exchange(other._handle, T{});
	}

	template <typename... Ts>
	constexpr void reset(T value) noexcept {
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

	T _handle;
};

template <typename T>
using unique_value = unique_handle<T, std::nullopt>;

using unique_flag = unique_value<bool>;


}

}

#endif
