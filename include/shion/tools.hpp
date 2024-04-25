#ifndef SHION_TOOLS_TOOLS_H_
#define SHION_TOOLS_TOOLS_H_

#include <utility>
#include <functional>
#include <optional>
#include <memory>
#include <cassert>
#include <ranges>
#include <optional>

#include "shion_essentials.hpp"

namespace shion {

template <typename T, template<typename ...> class Of>
inline constexpr bool is_specialization_v = false;

template <typename T, template<typename ...> class Of>
inline constexpr bool is_specialization_v<const T, Of> = is_specialization_v<T, Of>;

template <template<typename ...> class Of, typename ...Ts>
inline constexpr bool is_specialization_v<Of<Ts...>, Of> = true;

template <typename T, template<typename ...> class Of>
using is_specialization_of = std::bool_constant<is_specialization_v<T, Of>>;

template <typename T>
inline constexpr bool is_optional = false;

template <typename T>
inline constexpr bool is_optional<std::optional<T>> = true;

template <typename Key, typename Value, typename Hasher = std::hash<Key>, typename Equal = std::equal_to<>>
class cache;

/**
 * @brief Simple type constructor, forwarding its argument.
 *
 * Meant to be passed to standard library functions like std::ranges::transform.
 */
template <typename T>
constexpr auto construct = []<typename... Ts>(Ts&&... args) constexpr noexcept(std::is_nothrow_constructible_v<T, Ts...>) -> T requires (std::is_constructible_v<T, Ts...>) {
	return T(std::forward<Ts>(args)...);
};

/**
 * @brief Simple address getter.
 *
 * Meant to be passed to standard library functions like std::ranges::transform.
 */
constexpr auto address_of = [](auto&& value) constexpr noexcept {
	return &value;
};

template <template <class...> class Container>
constexpr auto to_impl = []<typename Rng, typename... Args>(Rng&& range, Args&&... args) {
	namespace stdr = std::ranges;
	using range_t = std::remove_cvref_t<Rng>;
	using value_t = stdr::range_value_t<range_t>;
	using iter_t = std::ranges::iterator_t<Rng>;

	auto begin = stdr::begin(range);
	auto end = stdr::end(range);
	if constexpr (requires { Container(std::declval<iter_t>(), std::declval<iter_t>(), args...); })  {
		return Container(begin, end, std::forward<Args>(args)...);
	} else if constexpr (stdr::contiguous_range<range_t> && stdr::sized_range<range_t>) {
		if constexpr ( requires { Container(std::declval<std::add_pointer_t<value_t>>(), std::declval<size_t>(), args...); } ) {
			return Container(&(*begin), std::ranges::size(range), std::forward<Args>(args)...);
		}
	}
	// idk, pray
};

template <typename T, typename U>
constexpr bool same_type_value(T&& lhs, U&& rhs) noexcept {
	if constexpr (!std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>>) {
		return false;
	} else {
		return lhs == rhs;
	}
}

/**
 * An attempt at https://en.cppreference.com/w/cpp/ranges/to
 */
template <template <class...> class Container, typename Rng, typename... Args>
requires (std::ranges::range<std::remove_cvref_t<Rng>>)
constexpr auto to(Rng&& range, Args&&... args) {
	return to_impl<Container>(std::forward<Rng>(range), std::forward<Args>(args)...);
}

template <template <typename...> typename Container, typename... Args>
struct to_closure {
	std::tuple<Args...> args;

	template <typename Lhs>
	requires (std::ranges::input_range<std::remove_cvref_t<Lhs>>)
	friend constexpr auto operator|(Lhs&& lhs, to_closure<Container, Args...> const& rhs) {
		return []<size_t... Ns>(Lhs rng, const std::tuple<Args...>& args_, std::index_sequence<Ns...>) {
			return to_impl<Container>(std::forward<Lhs>(rng), std::get<Ns>(args_)...);
		}(static_cast<Lhs>(lhs), rhs.args, std::make_index_sequence<sizeof...(Args)>{});
	}
};

template <template <typename...> typename Container>
struct to_closure<Container> {
	template <typename Lhs>
	requires (std::ranges::input_range<std::remove_cvref_t<Lhs>>)
	friend constexpr auto operator|(Lhs&& lhs, to_closure<Container> const&) {
		return to_impl<Container>(std::forward<Lhs>(lhs));
	}
};

template <template <typename...> typename Container, typename... Args>
constexpr auto to(Args&&... args) {
	if constexpr (sizeof...(Args) > 0) {
		return to_closure<Container, Args...>{std::forward_as_tuple<Args...>(args...)};
	} else {
		return to_closure<Container, Args...>{};
	}
}

template <typename T>
class storage {
public:
	constexpr storage() noexcept(std::is_nothrow_constructible_v<T>) requires (std::is_default_constructible_v<T>) {
		std::construct_at(reinterpret_cast<T*>(_data));
	}

	template <typename... Args>
	requires (requires { T(std::declval<Args>()...); })
	constexpr storage(Args&&... args) noexcept (std::is_nothrow_constructible_v<T, Args...>) {
		std::construct_at(reinterpret_cast<T*>(_data), std::forward<Args>(args)...);
	}

	storage& operator=(const storage&) = delete;
	storage& operator=(storage&&) = delete;

	T* get() noexcept {
		return std::launder<T>(reinterpret_cast<T*>(_data));
	}

	T const* get() const noexcept {
		return std::launder<T>(reinterpret_cast<T*>(_data));
	}

	T& operator*() & noexcept {
		return *get();
	}

	T&& operator*() && noexcept {
		return static_cast<T&&>(*get());
	}

	T const& operator*() const& noexcept {
		return *get();
	}

	T* operator->() noexcept {
		return get();
	}

	T* operator->() const noexcept {
		return get();
	}

private:
	alignas(T) byte _data[sizeof(T)];
};

template <typename T>
class storage<T&> {
public:
	storage() = delete;

	constexpr storage(T& obj) noexcept : _ptr{&obj} {}

	storage& operator=(const storage&) = delete;
	storage& operator=(storage&&) = delete;

	T& operator*() noexcept {
		return *_ptr;
	}

	T& operator*() const noexcept {
		return *_ptr;
	}

	T* operator->() noexcept {
		return _ptr;
	}

	T* operator->() const noexcept {
		return _ptr;
	}

private:
	T* _ptr;
};

template <typename T>
class storage<T&&> {
public:
	storage() = delete;

	constexpr storage(T& obj) noexcept : _ptr{&obj} {}

	storage& operator=(const storage&) = delete;
	storage& operator=(storage&&) = delete;

	T&& operator*() noexcept {
		return *static_cast<T&&>(_ptr);
	}

	T&& operator*() const noexcept {
		return *static_cast<T&&>(_ptr);
	}

	T* operator->() noexcept {
		return _ptr;
	}

	T* operator->() const noexcept {
		return _ptr;
	}

private:
	T* _ptr;
};

}

#endif /* SHION_TOOLS_TOOLS_H_ */
