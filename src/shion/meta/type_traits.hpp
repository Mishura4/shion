#ifndef SHION_META_TYPE_TRAITS_H_
#define SHION_META_TYPE_TRAITS_H_

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#	include <concepts>
#	include <type_traits>
#	include <optional>

#	include <shion/common/types.hpp>
#endif

namespace SHION_NAMESPACE
{

SHION_EXPORT_START

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

enum class value_type {
	value,
	const_value,
	lvalue_reference,
	const_lvalue_reference,
	rvalue_reference,
	const_rvalue_reference
};

template <value_type>
inline constexpr bool is_reference_type = false;

template <>
inline constexpr bool is_reference_type<value_type::lvalue_reference> = true;

template <>
inline constexpr bool is_reference_type<value_type::const_lvalue_reference> = true;

template <>
inline constexpr bool is_reference_type<value_type::rvalue_reference> = true;

template <>
inline constexpr bool is_reference_type<value_type::const_rvalue_reference> = true;

template <value_type>
inline constexpr bool is_value_type = false;

template <>
inline constexpr bool is_value_type<value_type::value> = true;

template <>
inline constexpr bool is_value_type<value_type::const_value> = true;

template <value_type>
inline constexpr bool is_lvalue_reference_type = false;

template <>
inline constexpr bool is_lvalue_reference_type<value_type::lvalue_reference> = true;

template <>
inline constexpr bool is_lvalue_reference_type<value_type::const_lvalue_reference> = true;

template <value_type>
inline constexpr bool is_rvalue_reference_type = false;

template <>
inline constexpr bool is_rvalue_reference_type<value_type::rvalue_reference> = true;

template <>
inline constexpr bool is_lvalue_reference_type<value_type::const_rvalue_reference> = true;

template <value_type>
inline constexpr bool is_const_type = false;

template <>
inline constexpr bool is_const_type<value_type::const_value> = true;

template <value_type Type>
decltype(auto) forward_like_type(auto& value) noexcept {
	using provided_t = decltype(value);
	using value_t = std::remove_reference_t<provided_t>;
	using stripped_t = std::remove_const_t<value_t>;

	if constexpr (is_value_type<Type> || is_rvalue_reference_type<Type>) {
		if constexpr (is_const_type<Type>) {
			return static_cast<std::add_rvalue_reference_t<std::add_const_t<stripped_t>>>(value);
		} else {
			return static_cast<std::add_rvalue_reference_t<stripped_t>>(value);
		}
	} else {
		static_assert(is_lvalue_reference_type<Type>);
		if constexpr (is_const_type<Type>) {
			return static_cast<std::add_lvalue_reference_t<std::add_const_t<stripped_t>>>(value);
		} else {
			return static_cast<std::add_lvalue_reference_t<stripped_t>>(value);
		}
	}
}

template <typename T>
inline constexpr bool is_copyable = std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>;

template <typename T>
inline constexpr bool is_moveable = std::is_move_constructible_v<T> && std::is_move_assignable_v<T>;

template <typename T>
inline constexpr bool is_nothrow_copyable = std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_copy_assignable_v<T>;

template <typename T>
inline constexpr bool is_nothrow_moveable = std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>;

template <typename T, typename U>
inline constexpr bool is_placeholder_for = alignof(T) == alignof(U) && sizeof(T) == sizeof(U);

template <typename T>
inline constexpr bool is_allocator_v = requires { typename std::allocator_traits<T>::allocator_type; };

#if defined(__cpp_lib_is_implicit_lifetime) and (__cpp_lib_is_implicit_lifetime >= 202302L)
template <typename T>
inline constexpr bool is_implicit_lifetime_v = std::is_implicit_lifetime_v<T>;
#elif SHION_HAS_BUILTIN(__builtin_is_implicit_lifetime)
template <typename T>
inline constexpr bool is_implicit_lifetime_v = __builtin_is_implicit_lifetime(T);
#else
// We cannot do this without compiler builtins. But we can approximate, maybe.
// The important part is we don't want false positives.
// False negatives are okay, just a missed optimization.

template <typename T>
inline constexpr bool is_implicit_lifetime_v = false;

template <typename T>
requires (std::is_scalar_v<T>)
inline constexpr bool is_implicit_lifetime_v<T> = true;

template <typename T>
requires (std::is_array_v<T>)
inline constexpr bool is_implicit_lifetime_v<T> = true;

template <typename T>
requires (std::is_class_v<T>)
inline constexpr bool is_implicit_lifetime_v<T> = 
	(std::is_aggregate_v<T> && std::is_trivially_destructible_v<T>)
	|| (std::is_trivially_destructible_v<T> && std::is_trivially_constructible_v<T>);

SHION_EXPORT_END

namespace detail
{
	
struct user_declared_destructor {
	~user_declared_destructor() = default;
};
struct user_provided_destructor {
	constexpr ~user_provided_destructor() {}
};
struct not_trivially_constructible {
	constexpr not_trivially_constructible() {}
};

enum class enum_class {};
enum enum_test {};

struct incomplete;

}

// Taken from https://github.com/llvm/llvm-project/pull/101807/files
static_assert(is_implicit_lifetime_v<decltype(nullptr)>);
static_assert(!is_implicit_lifetime_v<void>);
static_assert(!is_implicit_lifetime_v<const void>);
static_assert(!is_implicit_lifetime_v<volatile void>);
static_assert(is_implicit_lifetime_v<int>);
static_assert(!is_implicit_lifetime_v<int&>);
static_assert(!is_implicit_lifetime_v<int&&>);
static_assert(is_implicit_lifetime_v<float>);
static_assert(is_implicit_lifetime_v<double>);
static_assert(is_implicit_lifetime_v<long double>);
static_assert(is_implicit_lifetime_v<int*>);
static_assert(is_implicit_lifetime_v<int[]>);
static_assert(is_implicit_lifetime_v<int[5]>);
static_assert(is_implicit_lifetime_v<detail::enum_test>);
static_assert(is_implicit_lifetime_v<detail::enum_class>);
static_assert(is_implicit_lifetime_v<detail::user_declared_destructor>);
static_assert(!is_implicit_lifetime_v<detail::user_provided_destructor>);
static_assert(!is_implicit_lifetime_v<detail::not_trivially_constructible>);
static_assert(!is_implicit_lifetime_v<int()>);
static_assert(is_implicit_lifetime_v<int(*)()>);
static_assert(is_implicit_lifetime_v<int detail::user_declared_destructor::*>);
static_assert(is_implicit_lifetime_v<int (detail::user_declared_destructor::*)()>);
  //static_assert(!is_implicit_lifetime_v<detail::incomplete>);
  // expected-error@-1 {{incomplete type 'IncompleteStruct' used in type trait expression}}
  static_assert(is_implicit_lifetime_v<detail::incomplete[]>);
  static_assert(is_implicit_lifetime_v<detail::incomplete[5]>);
// Good enough?

SHION_EXPORT_START

#endif

template <typename T>
concept implicit_lifetime_type = is_implicit_lifetime_v<T>;

template <typename T>
struct tuple_size {};

template <typename... Args>
struct tuple_size<tuple<Args...>> : std::integral_constant<size_t, sizeof...(Args)>
{
};

template <size_t I, typename T>
struct tuple_element {};

template <typename T>
struct tuple_size_selector // Workaround msvc struggling to export std::tuple_size partial specialization
{
	using type = std::tuple_size<T>;
};

template <typename... Args>
struct tuple_size_selector<tuple<Args...>>
{
	using type = tuple_size<tuple<Args...>>;
};

template <size_t I, typename T>
struct tuple_element_selector // Workaround msvc struggling to export std::tuple_size partial specialization
{
	using type = std::tuple_element<I, T>;
};

template <size_t I, typename... Args>
struct tuple_element_selector<I, tuple<Args...>>
{
	using type = tuple_element<I, tuple<Args...>>;
};

template <typename T>
concept tuple_like = requires { tuple_size_selector<T>::type::value; };

template <typename T>
concept tuple_range = std::ranges::range<T> && tuple_like<T>;

template <typename T, typename Value = typename std::iterator_traits<decltype(std::declval<T>().begin())>::value_type>
concept pushable_container =
	requires (T cont, Value value) { cont.push_back(std::forward<Value>(value)); }
	|| requires (T cont, Value value) { cont.push(std::forward<Value>(value)); };

template <typename T, typename Value = typename std::iterator_traits<decltype(std::declval<T>().begin())>::value_type>
concept emplaceable_container =
	requires (T cont, Value value) { cont.emplace_back(std::forward<Value>(value)); }
	|| requires (T cont, Value value) { cont.emplace(std::forward<Value>(value)); };

template <typename T>
concept resizable_container = requires (T range) { range.resize(1); };

template <typename T>
concept reservable_container = requires (T range) { range.reserve(1); };

template <typename T>
concept dynamic_size_container =
	resizable_container<T>
	|| pushable_container<T>
	|| emplaceable_container<T>;

template <typename T>
concept appendable_range = std::ranges::range<T> && dynamic_size_container<T>;

SHION_EXPORT_END

}

#endif
