#ifndef SHION_TOOLS_TOOLS_H_
#define SHION_TOOLS_TOOLS_H_

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#  if !SHION_IMPORT_STD
#    include <utility>
#    include <functional>
#    include <optional>
#    include <memory>
#    include <cassert>
#    include <ranges>
#    include <optional>
#  endif
#endif

namespace shion {

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

SHION_EXPORT template <typename T, typename U>
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
SHION_EXPORT template <template <class...> class Container, typename Rng, typename... Args>
requires (std::ranges::range<std::remove_cvref_t<Rng>>)
constexpr auto to(Rng&& range, Args&&... args) {
	return to_impl<Container>(std::forward<Rng>(range), std::forward<Args>(args)...);
}

SHION_EXPORT template <template <typename...> typename Container, typename... Args>
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

SHION_EXPORT template <template <typename...> typename Container>
struct to_closure<Container> {
	template <typename Lhs>
	requires (std::ranges::input_range<std::remove_cvref_t<Lhs>>)
	friend constexpr auto operator|(Lhs&& lhs, to_closure<Container> const&) {
		return to_impl<Container>(std::forward<Lhs>(lhs));
	}
};

SHION_EXPORT template <template <typename...> typename Container, typename... Args>
constexpr auto to(Args&&... args) {
	if constexpr (sizeof...(Args) > 0) {
		return to_closure<Container, Args...>{std::forward_as_tuple<Args...>(args...)};
	} else {
		return to_closure<Container, Args...>{};
	}
}

SHION_EXPORT template <typename T, typename U>
inline constexpr bool is_compatible_reference = false;

SHION_EXPORT template <typename T, typename U>
inline constexpr bool is_compatible_reference<T&, U&> = true;

SHION_EXPORT template <typename T, typename U>
inline constexpr bool is_compatible_reference<const T&, U&> = true;

SHION_EXPORT template <typename T, typename U>
inline constexpr bool is_compatible_reference<T&&, U&&> = true;

SHION_EXPORT template <typename T, typename U>
inline constexpr bool is_compatible_reference<const T&&, U&&> = true;

SHION_EXPORT template <std::signed_integral T = int>
inline constexpr T bool_to_sign(bool value) noexcept
{
	return (static_cast<T>(value) * 2 - 1);
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

/**
 * @brief In release, this returns a "default value". In debug, this calls std::unreachable.
 */
template <typename T = void>
T safe_unreachable() {
#ifndef NDEBUG
	unreachable();
#else
	if constexpr (!std::is_void_v<T>) {
		return T{};
	} else {
		return;
	}
#endif
}

}

#endif /* SHION_TOOLS_TOOLS_H_ */
