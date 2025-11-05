#ifndef SHION_COMMON_ASSERT_H_
#define SHION_COMMON_ASSERT_H_

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#	include <type_traits>
#	include <cassert>
#	include <source_location>
#	include <atomic>
#	include <string_view>
#	include <format>
#	include <version>
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

struct SHION_API assert_failure_handler
{
private:
	[[noreturn]] void impl(std::string_view condition, std::string_view fmt, std::format_args args);

public:
	constexpr assert_failure_handler(std::source_location loc = std::source_location::current()) : where(std::move(loc)) {}
	
	[[noreturn]] void operator()(std::string_view condition);
	[[noreturn]] void operator()(std::string_view condition, std::string_view msg);
	template <typename... Ts>
	[[noreturn]] void operator()(std::string_view condition, std::format_string<std::type_identity_t<Ts>...> fmt, Ts&&... args)
	{
		this->impl(condition, fmt.get(), std::make_format_args(std::forward<Ts>(args)...));
	}


	std::source_location where;
};

}

#endif /* SHION_COMMON_ASSERT_H_ */
