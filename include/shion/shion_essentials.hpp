#ifndef SHION_ESSENTIALS_H_
#define SHION_ESSENTIALS_H_

#include <string_view>
#include <chrono>
#include <cstddef>

namespace shion {

namespace literals {

using namespace std::string_view_literals;
using namespace std::chrono_literals;

}

namespace aliases {

using schar = signed char;
using uchar = unsigned char;
using byte = std::byte;
using uint = unsigned int;
using ulong = unsigned long;
using ushort = unsigned short;
using ull = unsigned long long;

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

using std::size_t;

using app_clock = std::chrono::steady_clock;
using app_time = app_clock::time_point;
using app_duration = app_clock::duration;

using wall_clock = std::chrono::system_clock;
using wall_time = wall_clock::time_point;
using wall_duration = wall_clock::duration;

}

using namespace aliases;
using namespace literals;

struct version {
	uint major;
	uint minor;
	uint patch;
};

namespace detail {

template <typename T, typename U>
inline constexpr bool is_abi_compatible = alignof(T) == alignof(U) && sizeof(T) == sizeof(U);

}

struct unreachable_result {
	template <typename T>
	operator T() {
		return {};
	}
};

[[noreturn]] inline void unreachable() {
#ifdef _MSC_VER
	__assume(false);
#else
	__builtin_unreachable();
#endif
}

/**
 * @brief In release, this returns a "default value". In debug, this calls std::unreachable.
 */
template <typename T = void>
T safe_unreachable() {
#ifndef NDEBUG
	unreachable();
#else
	if constexpr (!std::is_void_v<T>) {
		return T{};
	} else {
		return;
	}
#endif
}


/**
 * @brief In release, this returns a "default value". In debug, this calls std::unreachable.
 */
inline unreachable_result safe_unreachable(unreachable_result) {
	return unreachable_result{};
}


/**
 * @brief In release, this returns a "default value". In debug, this calls std::unreachable.
 */
template <typename T>
decltype(auto) safe_unreachable(T&& t [[maybe_unused]]) {
#ifndef NDEBUG
	unreachable();
#else
	if constexpr (!std::is_void_v<T>) {
		return static_cast<decltype(t)>(t);
	} else {
		return;
	}
#endif
}

#ifdef SHION_SHARED_LIBRARY
#  ifdef WIN32
#    ifdef SHION_BUILD
#      define SHION_API __declspec(dllexport)
#    else
#      define SHION_API __declspec(dllimport)
#    endif
#  else
#    define SHION_API
#  endif
#else
#  define SHION_API
#endif

}
#endif
