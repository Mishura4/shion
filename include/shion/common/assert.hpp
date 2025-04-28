#ifndef SHION_COMMON_ASSERT_H_
#define SHION_COMMON_ASSERT_H_

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#	include <type_traits>
#	include <cassert>
#endif

SHION_EXPORT namespace SHION_NAMESPACE
{

[[noreturn]] inline void unreachable()
{
#ifdef _MSC_VER
	__assume(false);
#elifdef __clang__
	__builtin_unreachable();
#elifdef __GNUC__
	__builtin_unreachable();
#else
	std::abort();
#endif
}

}

#endif /* SHION_COMMON_ASSERT_H_ */
