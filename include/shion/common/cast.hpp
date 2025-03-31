#ifndef SHION_UTILITY_CAST_H_
#define SHION_UTILITY_CAST_H_

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#  if !SHION_IMPORT_STD
#    include <concepts>
#    include <type_traits>
#  endif

#  include <shion/common/types.hpp>
#endif /* SHION_BUILDING_MODULES */

namespace SHION_NAMESPACE
{

/**
 * @brief Casts an integer to another type, raising an assertion failure in debug if the target type cannot hold the value.
 */
SHION_EXPORT template <std::integral To, std::integral From>
constexpr To lossless_cast(From v) noexcept {
	if constexpr (std::is_same_v<To, From>) {
		return v;
	} else {
		if constexpr (std::is_unsigned_v<From>) {
			SHION_ASSERT(v <= static_cast<std::make_unsigned_t<To>>(((std::numeric_limits<To>::max))()));
		} else {
			if constexpr (std::is_unsigned_v<To>) {
				SHION_ASSERT(v >= 0);
			} else {
				SHION_ASSERT(v >= ((std::numeric_limits<To>::min))());
			}
		}
		return static_cast<To>(v);
	}
}

}

#endif /* SHION_UTILITY_CAST_H_ */
