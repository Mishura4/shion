#ifndef SHION_COMMON_ASSERT_H_
#define SHION_COMMON_ASSERT_H_

#include <type_traits>
#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#  include <cassert>
#endif

namespace SHION_NAMESPACE
{

[[noreturn]] inline void unreachable() {
#ifdef _MSC_VER
	__assume(false);
#else
	__builtin_unreachable();
#endif
}

#ifndef NDEBUG
#  define SHION_ASSERT(a) assert(a)
#else
#  define SHION_ASSERT(a) if (!(a)) shion::unreachable();
#endif

}

#endif /* SHION_COMMON_ASSERT_H_ */
