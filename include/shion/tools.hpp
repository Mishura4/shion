#ifndef SHION_TOOLS_TOOLS_H_
#define SHION_TOOLS_TOOLS_H_

#include <utility>
#include <functional>
#include <optional>
#include <memory>
#include <cassert>
#include <ranges>

#include "shion_essentials.hpp"

namespace shion {

#ifndef NDEBUG
#  define SHION_ASSERT(a) assert(a)
#else
#  define SHION_ASSERT(a) if (!(a)) std::unreachable();
#endif

template <typename T, template<typename ...> class Of>
inline constexpr bool is_specialization_v = false;

template <typename T, template<typename ...> class Of>
inline constexpr bool is_specialization_v<const T, Of> = is_specialization_v<T, Of>;

template <template<typename ...> class Of, typename ...Ts>
inline constexpr bool is_specialization_v<Of<Ts...>, Of> = true;

template <typename T, template<typename ...> class Of>
using is_specialization_of = std::bool_constant<is_specialization_v<T, Of>>;

template <typename T, auto Deleter>
struct unique_ptr_deleter {
	constexpr void operator()(T* ptr) const noexcept(std::is_nothrow_invocable_v<decltype(Deleter), T*>) {
		Deleter(ptr);
	}
};

template <typename T, auto Deleter, template <typename, typename> typename PtrType = std::unique_ptr>
using managed_ptr = PtrType<T, unique_ptr_deleter<T, Deleter>>;

template <typename T>
inline constexpr bool is_optional = false;

template <typename T>
inline constexpr bool is_optional<std::optional<T>> = true;

template <typename Key, typename Value, typename Hasher = std::hash<Key>, typename Equal = std::equal_to<>>
class cache;

/**
 * @brief Casts an integer to another type, raising an assertion failure in debug if the target type cannot hold the value.
 */
template <std::integral To, std::integral From>
constexpr To lossless_cast(From v) noexcept {
	if constexpr (std::is_unsigned_v<From>) {
		SHION_ASSERT(v <= static_cast<std::make_unsigned_t<To>>(std::numeric_limits<To>::max()));
	} else {
		if constexpr (std::is_unsigned_v<To>) {
			SHION_ASSERT(v >= 0);
		} else {
			SHION_ASSERT(v >= std::numeric_limits<To>::min());
		}
	}
	return static_cast<To>(v);
}

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

/**
 * An attempt at https://en.cppreference.com/w/cpp/ranges/to
 */
template <template <class...> class Container, typename Rng, typename... Args>
requires (std::ranges::range<std::remove_cvref_t<Rng>>)
constexpr auto to(Rng&& range, Args&&... args) {
	return to_impl<Container>(std::forward<Rng>(range), std::forward<Args>(args)...);
}

template <template <typename...> typename Container, typename... Args>
struct to_closure : std::ranges::range_adaptor_closure<to_closure<Container, Args...>> {
	std::tuple<Args...> args;

	template <typename Range>
	constexpr auto operator()(Range&& rng) const {
		return []<size_t... Ns>(Range rng, const std::tuple<Args...>& args_, std::index_sequence<Ns...>) {
			return to_impl<Container>(std::forward<Range>(rng), std::get<Ns>(args_)...);
		}(static_cast<Range>(rng), args, std::make_index_sequence<sizeof...(Args)>{});
	}
};

template <template <typename...> typename Container>
struct to_closure<Container> : std::ranges::range_adaptor_closure<to_closure<Container>> {
	template <typename Range>
	constexpr auto operator()(Range&& rng) const {
		return to_impl<Container>(std::forward<Range>(rng));
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

}

#endif /* SHION_TOOLS_TOOLS_H_ */
