#ifndef SHION_META_TYPE_TRAITS_H_
#define SHION_META_TYPE_TRAITS_H_

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#  include <concepts>
#  include <type_traits>
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

SHION_EXPORT_END

}

#endif
