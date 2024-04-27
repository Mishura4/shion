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

struct empty {};

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

template <typename T, typename U>
inline constexpr bool is_compatible_reference = false;

template <typename T, typename U>
inline constexpr bool is_compatible_reference<T&, U&> = true;

template <typename T, typename U>
inline constexpr bool is_compatible_reference<const T&, U&> = true;

template <typename T, typename U>
inline constexpr bool is_compatible_reference<T&&, U&&> = true;

template <typename T, typename U>
inline constexpr bool is_compatible_reference<const T&&, U&&> = true;

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

	empty dummy;
	T     data;
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

	empty dummy;
	T     data;
};

template <typename T>
union storage<T&> {
	constexpr storage() : dummy{} {}

	constexpr storage(T& value) noexcept : data(&value) {
	}

	template <typename U, typename... Args>
	requires (std::invocable<U, Args...> && is_compatible_reference<T&, std::invoke_result<U, Args...>>)
	constexpr storage(U&& fun, Args&&... args) noexcept(std::is_nothrow_invocable_v<U, Args...>) : data(std::invoke(std::forward<U>(fun), std::forward<Args>(args)...)) {
	}

	constexpr void emplace(T&& value) noexcept {
		std::construct_at(&data, &value);
	}

	template <typename U, typename... Args>
	requires (std::invocable<U, Args...> && is_compatible_reference<T&&, std::invoke_result<U, Args...>>)
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

	empty                                          dummy;
	std::add_pointer_t<std::remove_reference_t<T>> data;
};

template <typename T>
union storage<T&&> {
	constexpr storage() : dummy{} {}

	constexpr storage(T&& value) noexcept : data(&value) {
	}

	template <typename U, typename... Args>
	requires (std::invocable<U, Args...> && is_compatible_reference<T&&, std::invoke_result<U, Args...>>)
	constexpr storage(U&& fun, Args&&... args) noexcept(std::is_nothrow_invocable_v<U, Args...>) : data(&std::invoke(std::forward<U>(fun), std::forward<Args>(args)...)) {
	}

	constexpr void emplace(T&& value) noexcept {
		std::construct_at(&data, &value);
	}

	template <typename U, typename... Args>
	requires (std::invocable<U, Args...> && is_compatible_reference<T&&, std::invoke_result<U, Args...>>)
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

	empty                                          dummy;
	std::add_pointer_t<std::remove_reference_t<T>> data;
};

}

template <typename T>
class and_then {
public:
	template <typename U>
	constexpr and_then(U&& fun) noexcept(std::is_nothrow_constructible_v<T, U>) : _fun{std::forward<U>(fun)} {}
	constexpr and_then(and_then const&) = default;
	constexpr and_then(and_then&&) = default;

	constexpr and_then &operator=(and_then const&) = default;
	constexpr and_then &operator=(and_then&&) = default;

	constexpr ~and_then() {
		std::invoke(_fun);
	}
private:
	T _fun;
};

template <typename T>
and_then(T t) -> and_then<T>;

}

#endif /* SHION_TOOLS_TOOLS_H_ */
