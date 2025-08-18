#ifndef SHION_COMMON_ASSERT_H_
#define SHION_COMMON_ASSERT_H_

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#	include <type_traits>
#	include <cassert>
#	include <source_location>
#	include <atomic>
#	include <string_view>
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

using assert_handler = void(*)(std::string_view, std::source_location);

[[noreturn]] SHION_API void default_assert_handler(
	std::string_view msg,
	std::source_location where = std::source_location::current()
);

SHION_API extern std::atomic<assert_handler> g_assert_handler;

}

#endif /* SHION_COMMON_ASSERT_H_ */
