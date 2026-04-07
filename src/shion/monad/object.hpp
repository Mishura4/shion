#ifndef SHION_MONAD_OBJECT_H_
#define SHION_MONAD_OBJECT_H_

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
	#include <shion/meta/macros.hpp>
	#include <type_traits>
	#include <utility>
#endif

namespace SHION_NAMESPACE
{

SHION_EXPORT namespace monad
{

/**
 * @brief Simple type constructor, forwarding its argument.
 *
 * Meant to be passed to standard library functions like std::ranges::transform.
 */
template <typename T>
inline constexpr auto construct = []<typename... Ts>(Ts&&... args) constexpr noexcept(std::is_nothrow_constructible_v<T, Ts...>) -> T requires (std::is_constructible_v<T, Ts...>) {
	return T(std::forward<Ts>(args)...);
};

template <typename T>
struct construct_from_tuple_t
{
private:
	template <typename Tuple, size_t... Ns>
	static constexpr T impl(Tuple&& tuple, std::index_sequence<Ns...>) noexcept(
		std::is_nothrow_constructible_v<T, std::tuple_element_t<Ns, Tuple>...>
	)
	{
		return T(std::get<Ns>(std::forward<Tuple>(tuple))...);
	}

public:
	template <typename Tuple>
		requires shion::tuple_like<std::remove_reference_t<Tuple>>
	SHION_STATIC_OPERATOR_CALL constexpr T operator()(Tuple&& tuple) SHION_CONST_OPERATOR_CALL noexcept(
		noexcept(impl(std::forward<Tuple>(tuple), std::make_index_sequence<std::tuple_size<Tuple>::value>{}))
	)
	{
		return impl(
			std::forward<Tuple>(tuple),
			std::make_index_sequence<std::tuple_size<Tuple>::value>{}
		);
	}
};

/**
 * @brief Constructor from tuple, unwraps the tuple and passes each element as argument to the type T's constructors.
 *
 * Meant to be passed to standard library functions like std::ranges::transform.
 * Similar to std::make_from_tuple, but noexcept accordingly, and can be used as an argument.
 */
template <typename T>
inline constexpr auto construct_from_tuple = construct_from_tuple_t<T>{};

/**
 * @brief Simple address getter.
 *
 * Meant to be passed to standard library functions like std::ranges::transform.
 */
inline constexpr auto address_of = [](auto&& value) constexpr noexcept {
	return &value;
};


}

}

#endif /* SHION_MONAD_OBJECT_H_ */
