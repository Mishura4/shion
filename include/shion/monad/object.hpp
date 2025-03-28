#ifndef SHION_MONAD_OBJECT_H_
#define SHION_MONAD_OBJECT_H_

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
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


}

}

#endif /* SHION_MONAD_OBJECT_H_ */
