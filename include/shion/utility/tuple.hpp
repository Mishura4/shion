#ifndef SHION_TUPLE_H_
#define SHION_TUPLE_H_

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#include <type_traits>
#include <utility>
#include <tuple>
#include <compare>

#include <shion/shion_essentials.hpp>
#include <shion/meta/type_list.hpp>
#endif

namespace SHION_NAMESPACE {

namespace detail {

template <typename T, typename Tuple, typename IdxSeq>
inline constexpr bool is_piecewise_constructible_impl = false;

template <typename T, typename Tuple, size_t... Ns>
inline constexpr bool is_piecewise_constructible_impl<T, Tuple, std::index_sequence<Ns...>> = std::constructible_from<T, decltype(get<Ns>(std::declval<Tuple>()))...>;

template <typename T, typename Tuple, typename IdxSeq>
inline constexpr bool is_nothrow_piecewise_constructible_impl = false;

#if defined(_MSC_VER) or SHION_HAS_BUILTIN(__is_nothrow_constructible)
template <typename T, typename Tuple, size_t... Ns>
inline constexpr bool is_nothrow_piecewise_constructible_impl<T, Tuple, std::index_sequence<Ns...>> = __is_nothrow_constructible(T, decltype(get<Ns>(std::declval<Tuple>()))...);
#else
template <typename T, typename Tuple, size_t... Ns>
requires (is_piecewise_constructible_impl<T, Tuple, Ns...>)
inline constexpr bool is_nothrow_piecewise_constructible_impl<T, Tuple, std::index_sequence<Ns...>> = noexcept(T(decltype(get<Ns>(std::declval<Tuple>()))...));
#endif

template <size_t N_, typename T_>
requires (std::is_default_constructible_v<T_>)
struct _tuple_node {
	using type = T_;
	
	T_ data;
	
	constexpr friend auto operator<=>(const _tuple_node& left, const _tuple_node &right) = default;
	constexpr friend auto operator<=>(const _tuple_node& left, const T_ &right)
		noexcept(noexcept(left.data <=> right))
		requires (std::three_way_comparable<T_>)
	{
		return left.data <=> right;
	}
	
	constexpr friend bool operator==(const _tuple_node& left, const _tuple_node &right) = default;
	constexpr friend bool operator==(const _tuple_node& left, const T_ &right)
		noexcept(noexcept(left.data == right))
		requires (std::equality_comparable<T_>)
	{
		return left.data == right;
	}
};

}

SHION_EXPORT template <typename T, typename Tuple>
constexpr inline bool is_piecewise_constructible_v = false;

SHION_EXPORT template <typename T, typename Tuple>
	requires requires { tuple_size_selector<Tuple>::type::value; }
constexpr inline bool is_piecewise_constructible_v<T, Tuple> = detail::is_piecewise_constructible_impl<T, Tuple, std::make_index_sequence<tuple_size_selector<Tuple>::type::value>>;

SHION_EXPORT template <typename T, typename Tuple>
constexpr inline bool is_nothrow_piecewise_constructible_v = false;

SHION_EXPORT template <typename T, typename Tuple>
	requires requires { tuple_size_selector<Tuple>::type::value; }
constexpr inline bool is_nothrow_piecewise_constructible_v<T, Tuple> = detail::is_nothrow_piecewise_constructible_impl<T, Tuple, std::make_index_sequence<tuple_size_selector<Tuple>::type::value>>;

SHION_EXPORT template <typename T, typename Tuple>
concept piecewise_constructible = requires { tuple_size_selector<Tuple>::type::value; }
								  && detail::is_piecewise_constructible_impl<T, Tuple, std::make_index_sequence<tuple_size_selector<Tuple>::type::value>>;

namespace detail {

template <size_t N, typename T>
constexpr auto get(_tuple_node<N, T>& tuple) noexcept -> T& {
	return tuple.data;
}

template <size_t N, typename T>
constexpr auto get(_tuple_node<N, T> const& tuple) noexcept -> std::add_const_t<T>& {
	return tuple.data;
}

template <size_t N, typename T>
constexpr auto get(_tuple_node<N, T>&& tuple) noexcept -> T&& {
	return static_cast<T&&>(tuple.data);
}

template <size_t N, typename T>
constexpr auto get(_tuple_node<N, T> const&& tuple) noexcept -> std::add_const_t<T>&& {
	return static_cast<T&&>(tuple.data);
}

template <typename T, size_t N>
constexpr auto get(_tuple_node<N, T>& tuple) noexcept -> std::add_lvalue_reference_t<T> {
	return tuple.data;
}

template <typename T, size_t N>
constexpr auto get(_tuple_node<N, T> const& tuple) noexcept -> std::add_lvalue_reference_t<std::add_const_t<T>> {
	return tuple.data;
}

template <typename T, size_t N>
constexpr auto get(_tuple_node<N, T>&& tuple) noexcept -> std::add_rvalue_reference_t<T> {
	return static_cast<std::add_rvalue_reference_t<T>>(tuple.data);
}

template <typename T, size_t N>
constexpr auto get(_tuple_node<N, T> const&& tuple) noexcept -> std::add_rvalue_reference_t<T> {
	return static_cast<std::add_rvalue_reference_t<T>>(tuple.data);
}

template <size_t N, typename T>
auto get_type(_tuple_node<N, T>& tuple) -> decltype(tuple.data); // NOLINT

template <size_t N, typename T>
auto get_type(const _tuple_node<N, T>& tuple) -> decltype(tuple.data); // NOLINT

template <size_t N, typename T>
auto get_type(_tuple_node<N, T>&& tuple) -> decltype(tuple.data); // NOLINT

template <typename... Args>
struct _tuple_impl;

template <size_t... Ns, typename... Args>
struct _tuple_impl<std::index_sequence<Ns...>, Args...> : _tuple_node<Ns, Args>... {
#if defined(_cpp_pack_indexing)
	template <size_t N>
	requires (N < sizeof...(Args))
	using element = Args...[N];
#else
	template <size_t N>
	requires (N < sizeof...(Args))
	using element = decltype(get_type<N>(std::declval<_tuple_impl>()));
#endif
private:
	template <size_t N, typename T>
	constexpr auto _make_node(T&& arg) noexcept(std::is_nothrow_constructible_v<element<N>, T>) {
		return _tuple_node<N, element<N>>{ .data{ std::forward<T>(arg) } };
	}

	template <size_t N>
	constexpr auto _make_node(uninitialized_t) noexcept(std::is_nothrow_default_constructible_v<element<N>>) {
		_tuple_node<N, element<N>> node;
		return node; // Here be lions - will break in constexpr
	}

	template <size_t N, typename T, size_t... Ns_>
	constexpr auto _make_node(std::piecewise_construct_t, T&& tuple, std::index_sequence<Ns_...>) noexcept(is_nothrow_piecewise_constructible_v<element<N>, T>) {
		return _tuple_node<N, element<N>>{ .data{ get<Ns_>(std::forward<T>(tuple))... } };
	}

	template <size_t N, typename T>
	constexpr auto _make_node(std::piecewise_construct_t, T&& tuple) noexcept(is_nothrow_piecewise_constructible_v<element<N>, T>) {
		return this->template _make_node<N, T>(std::piecewise_construct, std::forward<T>(tuple), std::make_index_sequence<tuple_size_selector<std::remove_cvref_t<T>>::type::value>{});
	}

	template <size_t N, typename T>
	constexpr auto _make_node(std::piecewise_construct_t, uninitialized_t) noexcept(is_nothrow_piecewise_constructible_v<element<N>, T>) {
		return this->template _make_node<N>(uninitialized);
	}

	template <size_t N>
	constexpr auto _make_empty_node() noexcept(std::is_nothrow_default_constructible_v<element<N>>) {
		return _tuple_node<N, element<N>>{ .data{} };
	}

public:
	constexpr _tuple_impl() = default;
	constexpr _tuple_impl(const _tuple_impl&) = default;
	constexpr _tuple_impl(_tuple_impl&&) = default;

	template <size_t... Ns_, typename... Args_, size_t... Extras>
		requires (((std::same_as<std::remove_cvref_t<Args_>, uninitialized_t> && std::is_default_constructible_v<element<Ns_>>) // Constructible with uninitialized_t
				   || std::constructible_from<element<Ns_>, Args_>) && ...) // Constructible with argument
				 && (std::constructible_from<element<sizeof...(Ns_) + Extras>> && ...) // Missing arguments are default-constructible
	constexpr _tuple_impl(std::index_sequence<Ns_...>, std::index_sequence<Extras...>, Args_&&... args)
		noexcept((std::is_nothrow_constructible_v<element<Ns_>, Args_> && ...) && (std::is_nothrow_constructible_v<element<sizeof...(Ns_) + Extras>> && ...)) :
		_tuple_node<Ns_, element<Ns_>> { _make_node<Ns_>(args) }...,
		_tuple_node<sizeof...(Ns_) + Extras, element<sizeof...(Ns_) + Extras>> { _make_empty_node<sizeof...(Ns_) + Extras>() }...
	{}

	template <size_t... Ns_, typename... Args_, size_t... Extras>
	requires (((std::same_as<std::remove_cvref_t<Args_>, uninitialized_t> && std::is_default_constructible_v<element<Ns_>>) // Constructible with uninitialized_t
			   || piecewise_constructible<element<Ns_>, Args_>) && ...) // Constructible with argument
			 && (std::constructible_from<element<sizeof...(Ns_) + Extras>> && ...) // Missing arguments are default-constructible
	constexpr _tuple_impl(std::piecewise_construct_t, std::index_sequence<Ns_...>, std::index_sequence<Extras...>, Args_&&... args)
		noexcept((is_nothrow_piecewise_constructible_v<element<Ns_>, Args_> && ...) && (std::is_nothrow_constructible_v<element<sizeof...(Ns_) + Extras>> && ...)) :
		_tuple_node<Ns_, element<Ns_>> { _make_node<Ns_>(std::piecewise_construct, args) }...,
		_tuple_node<sizeof...(Ns_) + Extras, element<sizeof...(Ns_) + Extras>> { _make_empty_node<sizeof...(Ns_) + Extras>() }...
	{}

	constexpr ~_tuple_impl() = default;
	
	constexpr _tuple_impl& operator=(const _tuple_impl&) = default;
	constexpr _tuple_impl& operator=(_tuple_impl&&) = default;
	
	constexpr friend auto operator<=>(const _tuple_impl& left, const _tuple_impl& right) = default;
	constexpr friend bool operator==(const _tuple_impl& left, const _tuple_impl& right) = default;
};

}

SHION_EXPORT template <typename... Args>
struct tuple : detail::_tuple_impl<std::make_index_sequence<sizeof...(Args)>, Args...> {
private:
	using my_base = detail::_tuple_impl<std::make_index_sequence<sizeof...(Args)>, Args...>;
	template <size_t N>
	using extra_idx_seq = std::make_index_sequence<sizeof...(Args) - N>;

	template <typename... Args_>
	constexpr static inline bool implicitly_constructible = requires () { [](Args... args) -> my_base { return { std::forward<Args>(args)... }; }; };
	
public:
	using my_base::element;

	constexpr tuple() = default;
	constexpr tuple(const tuple&) = default;
	constexpr tuple(tuple&&) = default;

	template <typename... Args_>
	requires (std::constructible_from<my_base, std::index_sequence_for<Args_...>, extra_idx_seq<sizeof...(Args_)>, Args_...>)
	constexpr tuple(Args_&&... args)
		noexcept(std::is_nothrow_constructible_v<my_base, std::index_sequence_for<Args_...>, extra_idx_seq<sizeof...(Args_)>, Args_...>)
		: my_base{ std::index_sequence_for<Args_...>{}, extra_idx_seq<sizeof...(Args_)>{}, args... } {}

	template <typename... Args_>
	requires (std::constructible_from<my_base, std::piecewise_construct_t, std::index_sequence_for<Args...>, extra_idx_seq<sizeof...(Args_)>, Args_...>)
	constexpr tuple(std::piecewise_construct_t, Args_&&... args)
		noexcept(std::is_nothrow_constructible_v<my_base, std::piecewise_construct_t, std::index_sequence_for<Args...>, extra_idx_seq<sizeof...(Args_)>, Args_...>)
		: my_base{ std::piecewise_construct, std::index_sequence_for<Args_...>{}, extra_idx_seq<sizeof...(Args_)>{}, args... } {}

	constexpr ~tuple() = default;
	
	constexpr tuple& operator=(const tuple&) = default;
	constexpr tuple& operator=(tuple&&) = default;
	
	template <typename... Args_>
	requires(sizeof...(Args) == sizeof...(Args_))
	constexpr tuple& operator=(const tuple<Args_...>& other) noexcept((std::is_nothrow_assignable_v<Args, Args_> && ...)) {
		[]<size_t... Ns>(std::index_sequence<Ns...>, tuple& me, const tuple<Args_...>& othr) noexcept((std::is_nothrow_assignable_v<Args, Args_> && ...)) {
			(((detail::get<Ns>(me) = detail::get<Ns>(othr))), ...);
		}(std::make_index_sequence<sizeof...(Args)>{}, *this, other);
		return *this;
	}

	template <typename... Args_>
	requires(sizeof...(Args) == sizeof...(Args_))
	constexpr tuple& operator=(tuple<Args_...>&& other) noexcept((std::is_nothrow_assignable_v<Args, Args_&&> && ...)) {
		[]<size_t... Ns>(std::index_sequence<Ns...>, tuple& me, tuple<Args_...>&& othr) noexcept((std::is_nothrow_assignable_v<Args, Args_&&> && ...)) {
			(((detail::get<Ns>(me) = detail::get<Ns>(static_cast<decltype(othr)&&>(othr)))), ...);
		}(std::make_index_sequence<sizeof...(Args)>{}, *this, std::move(other));
		return *this;
	}

	template <typename... Args_>
	requires(sizeof...(Args) == sizeof...(Args_))
	constexpr tuple& operator=(const std::tuple<Args_...>& other) noexcept((std::is_nothrow_assignable_v<Args, Args_> && ...)) {
		[]<size_t... Ns>(std::index_sequence<Ns...>, tuple& me, const tuple<Args_...>& othr) noexcept((std::is_nothrow_assignable_v<Args, Args_> && ...)) {
			(((detail::get<Ns>(me) = detail::get<Ns>(othr))), ...);
		}(std::make_index_sequence<sizeof...(Args)>{}, *this, other);
		return *this;
	}

	template <typename... Args_>
	requires(sizeof...(Args) == sizeof...(Args_))
	constexpr tuple& operator=(std::tuple<Args_...>&& other) noexcept((std::is_nothrow_assignable_v<Args, Args_&&> && ...)) {
		[]<size_t... Ns>(std::index_sequence<Ns...>, tuple& me, std::tuple<Args_...>&& othr) noexcept((std::is_nothrow_assignable_v<Args, Args_&&> && ...)) {
			(((detail::get<Ns>(me) = detail::get<Ns>(static_cast<decltype(othr)&&>(othr)))), ...);
		}(std::make_index_sequence<sizeof...(Args)>{}, *this, std::move(other));
		return *this;
	}
	
	constexpr friend auto operator<=>(const tuple& left, const tuple& right) = default;
	constexpr friend bool operator==(const tuple& left, const tuple& right) = default;
};

template <typename T, typename U>
struct tuple<T, U> : detail::_tuple_impl<std::index_sequence<0, 1>, T, U> {
	using my_base = detail::_tuple_impl<std::index_sequence<0, 1>, T, U>;
	using my_base::element;

	constexpr tuple() = default;
	constexpr tuple(const tuple&) = default;
	constexpr tuple(tuple&&) = default;

	template <typename T_, typename U_>
	requires (std::constructible_from<my_base, std::index_sequence<0, 1>, std::index_sequence<>, T_, U_>)
	explicit(!std::is_convertible_v<T_, T> || !std::is_convertible_v<U, U_>)
	constexpr tuple(T_&& first, U_&& second)
		noexcept (std::is_nothrow_constructible_v<T, T_> && std::is_nothrow_constructible_v<U, U_>)
		: my_base{ std::index_sequence<0, 1>{}, std::index_sequence<>{}, std::forward<T_>(first), std::forward<U_>(second) } {}

	template <typename T_>
	requires (std::constructible_from<my_base, std::index_sequence<0>, std::index_sequence<0>, T_>)
	explicit constexpr tuple(T_&& first)
		noexcept (std::is_nothrow_constructible_v<T, T_> && std::is_nothrow_constructible_v<U>)
		: my_base{ std::index_sequence<0>{}, std::index_sequence<0>{}, std::forward<T>(first) } {}
	
	template <typename T_, typename U_>
	requires (piecewise_constructible<T, T_> && piecewise_constructible<U, U_>)
	constexpr tuple(std::piecewise_construct_t, T_&& first, U_&& second)
		noexcept (is_nothrow_piecewise_constructible_v<T, T_> && is_nothrow_piecewise_constructible_v<U, U_>)
		: my_base{ std::piecewise_construct, std::index_sequence<0, 1>{}, std::index_sequence<>{}, std::forward<T_>(first), std::forward<U_>(second) } {}
	
	template <typename T_>
	requires (std::constructible_from<my_base, std::piecewise_construct_t, std::index_sequence<0>, T_>)
	explicit constexpr tuple(std::piecewise_construct_t, T_&& first)
		noexcept (is_nothrow_piecewise_constructible_v<T, T_> && std::is_nothrow_constructible_v<U>)
		: my_base{ std::piecewise_construct, std::index_sequence<0>{}, std::index_sequence<0>{}, std::forward<T_>(first) } {}

	constexpr ~tuple() = default;
	
	constexpr tuple& operator=(const tuple&) = default;
	constexpr tuple& operator=(tuple&&) = default;
	
	template <typename T_, typename U_>
	constexpr tuple& operator=(const tuple<T_, U_>& other) noexcept(std::is_nothrow_assignable_v<T, T_> && std::is_nothrow_assignable_v<U, U_>) {
		detail::get<0>(*this) = detail::get<0>(other);
		detail::get<1>(*this) = detail::get<1>(other);
		return *this;
	}

	template <typename T_, typename U_>
	constexpr tuple& operator=(tuple<T_, U_>&& other) noexcept(std::is_nothrow_assignable_v<T, T_&&> && std::is_nothrow_assignable_v<U, U_&&>) {
		detail::get<0>(*this) = std::forward<T_>(detail::get<0>(other));
		detail::get<1>(*this) = std::forward<U_>(detail::get<1>(other));
		return *this;
	}

	template <typename T_, typename U_>
	constexpr tuple& operator=(const std::tuple<T_, U_>& other) noexcept(std::is_nothrow_assignable_v<T, T_> && std::is_nothrow_assignable_v<U, U_>) {
		detail::get<0>(*this) = std::get<0>(other);
		detail::get<1>(*this) = std::get<1>(other);
		return *this;
	}

	template <typename T_, typename U_>
	constexpr tuple& operator=(std::tuple<T_, U_>&& other) noexcept(std::is_nothrow_assignable_v<T, T_&&> && std::is_nothrow_assignable_v<U, U_&&>) {
		detail::get<0>(*this) = std::forward<T_>(std::get<0>(other));
		detail::get<1>(*this) = std::forward<U_>(std::get<1>(other));
		return *this;
	}

	template <typename T_, typename U_>
	constexpr tuple& operator=(const std::pair<T_, U_>& other) noexcept(std::is_nothrow_assignable_v<T, T_> && std::is_nothrow_assignable_v<U, U_>) {
		detail::get<0>(*this) = std::forward<T_>(other.first);
		detail::get<1>(*this) = std::forward<U_>(other.second);
		return *this;
	}

	template <typename T_, typename U_>
	constexpr tuple& operator=(std::pair<T_, U_>&& other) noexcept(std::is_nothrow_assignable_v<T, T_&&> && std::is_nothrow_assignable_v<U, U_&&>) {
		detail::get<0>(*this) = std::forward<T_>(other.first);
		detail::get<1>(*this) = std::forward<U_>(other.second);
		return *this;
	}

	constexpr friend auto operator<=>(const tuple& left, const tuple& right) = default;
	constexpr friend bool operator==(const tuple& left, const tuple& right) = default;
};

SHION_EXPORT template <size_t N, typename... Args>
requires (N < sizeof...(Args))
constexpr auto get(tuple<Args...>& t) noexcept -> decltype(auto) {
	return detail::get<N>(t);
}

SHION_EXPORT template <size_t N, typename... Args>
requires (N < sizeof...(Args))
constexpr auto get(tuple<Args...> const& t) noexcept -> decltype(auto) {
	return detail::get<N>(t);
}

SHION_EXPORT template <size_t N, typename... Args>
requires (N < sizeof...(Args))
constexpr auto get(tuple<Args...>&& t) noexcept -> decltype(auto) {
	return detail::get<N>(static_cast<tuple<Args...>&&>(t));
}

SHION_EXPORT template <size_t N, typename... Args>
requires (N < sizeof...(Args))
constexpr auto get(tuple<Args...> const&& t) noexcept -> decltype(auto) {
	return detail::get<N>(static_cast<tuple<Args...> const&&>(t));
}

SHION_EXPORT template <typename T, typename... Args>
constexpr decltype(auto) get(tuple<Args...>& t) noexcept {
	return detail::get<T>(t);
}

SHION_EXPORT template <typename T, typename... Args>
constexpr decltype(auto) get(const tuple<Args...>& t) noexcept {
	return detail::get<T>(t);
}

SHION_EXPORT template <typename T, typename... Args>
constexpr decltype(auto) get(tuple<Args...>&& t) noexcept {
	return detail::get<T>(static_cast<tuple<Args...>&&>(t));
}

SHION_EXPORT template <typename T, typename... Args>
constexpr decltype(auto) get(tuple<Args...> const&& t) noexcept {
	return detail::get<T>(static_cast<tuple<Args...> const&&>(t));
}

SHION_EXPORT template <typename... Args>
constexpr auto tie(Args &... args) noexcept -> tuple<Args&...> {
	return tuple<Args&...>{ args... };
}

template <size_t I, typename... Args>
requires (I <= sizeof...(Args))
struct tuple_element<I, tuple<Args...>>
{
	using type = typename tuple<Args...>::template element<I>;
};

SHION_EXPORT template <size_t I, typename T> using tuple_element_t = typename tuple_element<I, T>::type;

SHION_EXPORT template <typename T> using tuple_last_t = tuple_element_t<tuple_size<T>::value - 1, T>;

SHION_EXPORT template <typename T> using tuple_first_t = tuple_element_t<0, T>;

SHION_EXPORT template <typename... Ts>
tuple(Ts...) -> tuple<Ts...>;

SHION_EXPORT template <typename... Ts, typename Fun>
constexpr auto match(const Fun& fun [[maybe_unused]], tuple<Ts...>& t [[maybe_unused]]) -> std::array<bool, sizeof...(Ts)> {
	auto impl = [&]<size_t N, typename T>(T& val) {
		if constexpr (std::predicate<Fun, T>) {
			return std::invoke(fun, val);
		} else {
			return false;
		}
	};
	auto unfold = [&]<size_t... Ns>(std::index_sequence<Ns...>) -> std::array<bool, sizeof...(Ts)> {
		return { impl.template operator()<Ns>(get<Ns>(t))... };
	};
	return unfold(std::index_sequence_for<Ts...>{});
}

SHION_EXPORT template <typename... Ts, typename Fun>
constexpr auto match(const Fun& fun [[maybe_unused]], tuple<Ts...> const& t [[maybe_unused]]) -> std::array<bool, sizeof...(Ts)> {
	auto impl = [&]<size_t N, typename T>(T const& val) {
		if constexpr (std::predicate<Fun, T>) {
			return std::invoke(fun, val);
		} else {
			return false;
		}
	};
	auto unfold = [&]<size_t... Ns>(std::index_sequence<Ns...>) -> std::array<bool, sizeof...(Ts)> {
		return { static_cast<bool>(impl.template operator()<Ns>(get<Ns>(t)))... };
	};
	return unfold(std::index_sequence_for<Ts...>{});
}

SHION_EXPORT template <typename... Ts>
constexpr auto forward_as_tuple(Ts&&... args) noexcept {
	return tuple<Ts...>{ static_cast<Ts&&>(args)... };
}

}

SHION_EXPORT template <size_t I, typename... Args>
requires (I <= sizeof...(Args))
struct std::tuple_element<I, SHION_NAMESPACE :: tuple<Args...>> : SHION_NAMESPACE :: tuple_element<I, SHION_NAMESPACE :: tuple<Args...>>
{
};

SHION_EXPORT template <typename... Args>
struct std::tuple_size<SHION_NAMESPACE :: tuple<Args...>> : SHION_NAMESPACE :: tuple_size<SHION_NAMESPACE :: tuple<Args...>>
{
};

namespace SHION_NAMESPACE
{

namespace detail
{

struct unfold_impl {
	template <size_t N, typename T, typename Fun>
	constexpr auto call(Fun&& fun, T&& arg) const -> decltype(auto)
	{
		using type = decltype(fun.template operator()<N>(std::declval<T>()));
		if constexpr (std::is_void_v<type>) {
			fun.template operator()<N>(std::forward<T>(arg));
			return empty{};
		} else {
			return fun.template operator()<N>(std::forward<T>(arg));
		}
	}
	
	template <typename Fun, typename T, size_t... Ns>
	constexpr auto operator()(Fun&& fun, T&& tuple, std::index_sequence<Ns...>) const {
		return forward_as_tuple(this->template call<Ns>(fun, get<Ns>(std::forward<T>(tuple)))...);
	}
};

struct fold_impl {
	template <typename Fun, typename T>
	constexpr auto call(Fun&& fun, T&& arg1) const -> decltype(auto) {
		if constexpr (std::invocable<Fun, T>) {
			return (fun(std::forward<T>(arg1)));
		} else {
			return std::forward<T>(arg1);
		}
	}

	template <typename Fun, typename T, typename U>
	constexpr auto call(Fun&& fun, T&& arg1, U&& arg2) const -> decltype(auto) {
		return (fun(std::forward<T>(arg1), std::forward<U>(arg2)));
	}

	template <typename Fun, typename T, typename U, typename V>
	constexpr auto call(Fun&& fun, T&& arg1, U&& arg2, V&& arg3) const -> decltype(auto) {
		return (fun(fun(std::forward<T>(arg1), std::forward<U>(arg2)), std::forward<V>(arg3)));
	}

	template <typename Fun, typename T, typename U, typename V, typename W, typename... Args>
	constexpr auto call(Fun&& fun, T&& arg1, U&& arg2, V&& arg3, W&& arg4, Args&&... args) const -> decltype(auto) {
		if constexpr (sizeof...(Args)) {
			return call(fun(fun(fun(std::forward<T>(arg1), std::forward<U>(arg2)), std::forward<V>(arg3)), std::forward<W>(arg4)), std::forward<Args>(args)...);
		} else {
			return (fun(fun(fun(std::forward<T>(arg1), std::forward<U>(arg2)), std::forward<V>(arg3)), std::forward<W>(arg4)));
		}
	}
	
	template <typename Fun, typename T, size_t... Ns>
	constexpr auto operator()(Fun&& fun, T&& tuple, std::index_sequence<Ns...>) const {
		return call(fun, get<Ns>(std::forward<T>(tuple))...);
	}
};

}

struct unfold_t {
	template <typename T, typename Fun>
	constexpr auto operator()(Fun&& fun, T&& tuple) const {
		return detail::unfold_impl{}(fun, std::forward<T>(tuple), std::make_index_sequence<tuple_size_selector<std::remove_cvref_t<T>>::type::value>{});
	}
};

struct fold_t {
	template <typename T, typename Fun>
	constexpr auto operator()(Fun&& fun, T&& tuple) const {
		return detail::fold_impl{}(fun, std::forward<T>(tuple), std::make_index_sequence<tuple_size_selector<std::remove_cvref_t<T>>::type::value>{});
	}
};

SHION_EXPORT inline constexpr auto unfold = unfold_t{};

SHION_EXPORT inline constexpr auto fold = fold_t{};

}

#endif 
