#ifndef SHION_UTILITY_OPTIONAL_H_
#define SHION_UTILITY_OPTIONAL_H_

#include <optional> // std::nullopt_t, std::bad_optional_access
#include <type_traits>
#include <concepts>
#include <utility>
#include <memory>

#include "../shion_essentials.hpp"
#include "../tools.hpp"

namespace shion {

inline namespace utility {



/**
 * @brief std::optional, but can hold references
 */
template <typename T>
class optional {
public:
	constexpr optional() : _ptr{nullptr} {};
	constexpr optional(std::nullopt_t) : optional() {}

	constexpr optional(const optional& other) noexcept (std::is_nothrow_copy_constructible_v<T>)
	requires (std::is_copy_constructible_v<T>) {
		if (other.has_value()) {
			_storage._data = storage<T>{*other._ptr};
			_ptr = _storage.data.get();
		}
	}

	constexpr optional(optional&& other) noexcept (std::is_nothrow_move_constructible_v<T>)
	requires (std::is_move_constructible_v<T>) {
		if (other.has_value()) {
			_storage._data = storage<T>{std::move(*other._ptr)};
			_ptr = _storage.data.get();
		}
	}

	template <typename U>
	requires (std::is_constructible_v<T, std::add_lvalue_reference_t<std::add_const_t<U>>>)
	explicit(!std::is_convertible_v<T, std::add_lvalue_reference_t<std::add_const_t<U>>>)
	constexpr optional(const optional<U>& other) noexcept (std::is_nothrow_constructible_v<T, std::add_lvalue_reference_t<std::add_const_t<U>>>) {
		if (other.has_value()) {
			_storage = { ._data = storage<T>{*other._ptr} };
			_ptr = _storage.data.get();
		}
	}

	template <typename U>
	requires (std::is_constructible_v<T, std::add_rvalue_reference_t<U>>)
	explicit(!std::is_convertible_v<std::add_rvalue_reference_t<U>, T>)
	constexpr optional(optional<U>&& other) noexcept (std::is_nothrow_constructible_v<T, std::add_rvalue_reference_t<U>>) {
		if (other.has_value()) {
			_storage = { ._data = storage<T>{std::move(*other._ptr)} };
			_ptr = _storage.data.get();
		}
	}

	template <typename... Args>
	requires (std::is_constructible_v<T, Args...>)
	explicit constexpr optional(std::in_place_t, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) :
		_storage{._data{std::forward<Args>(args)...}},
		_ptr{_storage._data.get()} {
	}

	constexpr ~optional() {
		_destroy();
	}

	constexpr optional& operator=(std::nullopt_t) noexcept {
		_destroy();
		_ptr = nullptr;
		return *this;
	}

	constexpr optional& operator=(const optional &other) noexcept (std::is_nothrow_copy_constructible_v<T>)
	requires (std::is_copy_constructible_v<T>) {
		_destroy();
		if (other.has_value()) {
			_storage = { ._data = storage<T>{*other} };
			_ptr = _storage._data.get();
		} else {
			_ptr = nullptr;
		}
		return *this;
	}

	constexpr optional& operator=(optional&& other) noexcept (std::is_nothrow_move_constructible_v<T>)
	requires (std::is_move_constructible_v<T>) {
		_destroy();
		if (other.has_value()) {
			_storage = { ._data = storage<T>{*std::move(other)} };
			_ptr = _storage._data.get();
		} else {
			_ptr = nullptr;
		}
		return *this;
	}

	template <typename U>
	constexpr optional& operator=(const optional<U> &other) noexcept (std::is_nothrow_constructible_v<T, U>)
	requires (std::is_constructible_v<T, U>) {
		_destroy();
		if (other.has_value()) {
			_storage = { ._data = storage<T>{*other} };
			_ptr = _storage._data.get();
		} else {
			_ptr = nullptr;
		}
		return *this;
	}

	template <typename U>
	constexpr optional& operator=(optional<U>&& other) noexcept (std::is_nothrow_constructible_v<T, U>)
	requires (std::is_constructible_v<T, U>) {
		_destroy();
		if (other.has_value()) {
			_storage = { ._data = storage<T>{*std::move(other)} };
			_ptr = _storage._data.get();
		} else {
			_ptr = nullptr;
		}
		return *this;
	}

	constexpr decltype(auto) operator*() & noexcept {
		SHION_ASSERT(has_value());

		return *_storage._data;
	}

	constexpr decltype(auto) operator*() const& noexcept {
		SHION_ASSERT(has_value());

		return *_storage._data;
	}

	constexpr decltype(auto) operator*() && noexcept {
		SHION_ASSERT(has_value());

		return *std::move(_storage._data);
	}

	constexpr decltype(auto) operator*() const&& noexcept {
		SHION_ASSERT(has_value());

		return *std::move(_storage._data);
	}

	constexpr auto operator->() noexcept {
		SHION_ASSERT(has_value());

		return _storage._data.operator->();
	}

	constexpr auto operator->() const noexcept {
		SHION_ASSERT(has_value());

		return _storage._data.operator->();
	}

	explicit constexpr operator bool() const noexcept {
		return _ptr != nullptr;
	}

	constexpr bool has_value() const noexcept {
		return _ptr != nullptr;
	}

	constexpr decltype(auto) value() & {
		if (!has_value())
			throw std::bad_optional_access{};

		return operator*();
	}

	constexpr decltype(auto) value() const& {
		if (!has_value())
			throw std::bad_optional_access{};

		return operator*();
	}

	constexpr decltype(auto) value() && {
		if (!has_value())
			throw std::bad_optional_access{};

		return operator*();
	}

	constexpr decltype(auto) value() const&& {
		if (!has_value())
			throw std::bad_optional_access{};

		return operator*();
	}

	template <typename U>
	constexpr T value_or(U&& default_value) const& noexcept(std::is_constructible_v<T, U>) {
		if (has_value()) {
			return *_storage._data;
		}
		return T(std::forward<U>(default_value));
	};

	template <typename U>
	constexpr T value_or(U&& default_value) && noexcept(std::is_constructible_v<T, U>) {
		if (has_value()) {
			return *std::move(_storage._data);
		}
		return T(std::forward<U>(default_value));
	};

	template <typename U>
	requires (std::is_invocable_v<U> && std::is_constructible_v<T, std::invoke_result_t<U>>)
	constexpr T or_else(U&& supplier) const& noexcept(std::is_constructible_v<T, std::invoke_result_t<U>>) {
		if (has_value()) {
			return *_storage._data;
		}
		return T(std::invoke(std::forward<U>(supplier)));
	};

	template <typename U>
	requires (std::is_invocable_v<U> && std::is_constructible_v<T, std::invoke_result_t<U>>)
	constexpr T or_else(U&& supplier) && noexcept(std::is_constructible_v<T, std::invoke_result_t<U>>) {
		if (has_value()) {
			return *_storage._data;
		}
		return T(std::invoke(std::forward<U>(supplier)));
	};

	template <typename U>
	requires (std::is_invocable_v<U> && std::is_constructible_v<T, std::invoke_result_t<U>>)
	constexpr optional<T>& load_if_absent(U&& supplier) & noexcept(std::is_constructible_v<T, std::invoke_result_t<U>>) {
		if (has_value()) {
			return *this;
		}
		*this = std::invoke(std::forward<U>(supplier));
		return *this;
	};

	template <typename U>
	requires (std::is_invocable_v<U> && std::is_constructible_v<T, std::invoke_result_t<U>>)
	constexpr optional<T>&& load_if_absent(U&& supplier) && noexcept(std::is_constructible_v<T, std::invoke_result_t<U>>) {
		if (has_value()) {
			return static_cast<T&&>(*this);
		}
		*this = std::invoke(std::forward<U>(supplier));
		return static_cast<T&&>(*this);
	};

private:
	constexpr void _destroy() {
		if (_ptr)
			std::destroy_at(_storage._data);
	}

	union {
		storage<T>      _data;
	} _storage;
	T *_ptr{nullptr};
};

template <typename T, std::equality_comparable_with<T> U>
constexpr bool operator==(optional<T> const &lhs, optional<U> const &rhs) noexcept (noexcept(std::declval<T>() == std::declval<U>())) {
	if (lhs.has_value()) {
		if (rhs.has_value()) {
			return *lhs == *rhs;
		}
		return false;
	}
	return rhs.has_value();
}

template <typename T, std::three_way_comparable_with<T> U>
constexpr auto operator<=>(optional<T> const &lhs, optional<U> const &rhs) noexcept (noexcept(std::declval<T>() <=> std::declval<U>())) {
	if (auto result = lhs.has_value() <=> rhs.has_value(); result != std::strong_ordering::equal) {
		return result;
	}
	return *lhs <=> *rhs;
}

template <typename T>
constexpr bool operator==(optional<T> const &lhs, std::nullopt_t) noexcept {
	return lhs.has_value();
}

template <typename T>
constexpr auto operator<=>(optional<T> const &lhs, std::nullopt_t) noexcept {
	return lhs.has_value() <=> false;
}

template <typename T, std::equality_comparable_with<T> U>
constexpr bool operator==(optional<T> const &lhs, U const &rhs) noexcept (noexcept(std::declval<T>() == std::declval<U>())) {
	if (!lhs.has_value()) {
		return false;
	}
	return *lhs == rhs;
}

template <typename T, std::three_way_comparable_with<T> U>
constexpr auto operator<=>(optional<T> const &lhs, U const &rhs) noexcept (noexcept(std::declval<T>() <=> std::declval<U>())) {
	if (!lhs.has_value()) {
		return false <=> true;
	}
	return *lhs <=> rhs;
}

template <typename T, std::equality_comparable_with<T> U>
constexpr bool operator==(T const &rhs, optional<U> const &lhs) noexcept (noexcept(std::declval<U>() == std::declval<T>())) {
	if (!lhs.has_value()) {
		return false;
	}
	return *lhs == rhs;
}

template <typename T, std::three_way_comparable_with<T> U>
constexpr auto operator<=>(T const &rhs, optional<U> const &lhs) noexcept (noexcept(std::declval<U>() <=> std::declval<T>())) {
	if (!rhs.has_value()) {
		return true <=> false;
	}
	return *lhs <=> rhs;
}

}

}

#endif


