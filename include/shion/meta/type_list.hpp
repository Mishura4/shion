#ifndef SHION_TYPE_LIST_H_
#define SHION_TYPE_LIST_H_

#include "shion/common/defines.hpp"

#if !SHION_BUILDING_MODULES

#include <utility>
#include <type_traits>

#include "../shion_essentials.hpp"

#endif

namespace shion {

SHION_EXPORT template <typename T, typename... Args>
inline constexpr bool pack_contains = false || (std::is_same_v<T, Args> || ...);

SHION_EXPORT template <typename... Args>
inline constexpr bool is_unique = false;

template <>
inline constexpr bool is_unique<> = true;

template <typename T, typename... Args>
inline constexpr bool is_unique<T, Args...> = !pack_contains<T, Args...> && is_unique<Args...>;

namespace detail {

template <size_t N, typename T>
struct type_list_node {
	template <typename U>
	requires (std::is_same_v<T, U>)
	static consteval size_t _index_of() noexcept {
		return N;
	}

	template <size_t I>
	requires (I == N)
	static consteval T _make_t();
};

template <typename... Args>
struct type_list_impl;

template <typename... Args, size_t... Ns>
struct type_list_impl<std::index_sequence<Ns...>, Args...> : type_list_node<Ns, Args>... {
	using type_list_node<Ns, Args>::_index_of...;
	using type_list_node<Ns, Args>::_make_t...;
};

}

SHION_EXPORT template <typename... Args>
struct type_list : private detail::type_list_impl<std::make_index_sequence<sizeof...(Args)>, Args...> {
private:
	using base = detail::type_list_impl<std::make_index_sequence<sizeof...(Args)>, Args...>;

public:
	template <typename T>
	static constexpr size_t index_of = base::template _index_of<T>();

	template <size_t N>
	using type = decltype(base::template _make_t<N>());

	static constexpr bool is_unique = ::shion::is_unique<Args...>;
};

template <>
struct type_list<> {};

SHION_EXPORT template <typename T, typename... Args>
requires (is_unique<T, Args...>)
inline constexpr size_t index_of = type_list<Args...>::template index_of<T>;

#if defined(_cpp_pack_indexing)
SHION_EXPORT template <size_t N, typename... Args>
struct pack_element {
	using type = Args...[N];
};
#else
SHION_EXPORT template <size_t N, typename... Args>
struct pack_element {
	using type = typename type_list<Args...>::template type<N>;
};
#endif

SHION_EXPORT template <size_t N, typename... Args>
using pack_element_t = typename pack_element<N, Args...>::type;

SHION_EXPORT template <typename... Args>
using pack_first_t = typename pack_element<0, Args...>::type;

SHION_EXPORT template <typename... Args>
using pack_last_t = typename pack_element<sizeof...(Args) - 1, Args...>::type;

SHION_EXPORT template <size_t N, typename... Args>
using pack_type [[deprecated("use pack_element_t")]] = pack_element_t<N, Args...>;

}

#endif /* SHION_TYPE_LIST_H_ */
