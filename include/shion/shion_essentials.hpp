#ifndef SHION_ESSENTIALS_H_
#define SHION_ESSENTIALS_H_

#include <string_view>
#include <chrono>
#include <cstddef>

namespace shion {

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

namespace literals {

using namespace std::string_view_literals;
using namespace std::chrono_literals;

consteval int8 operator""_i8(unsigned long long int lit) {
	if (lit > std::numeric_limits<int8>::max()) {
		unreachable();
	}
	return static_cast<int8>(lit);
}

consteval int16 operator""_i16(unsigned long long int lit) {
	if (lit > std::numeric_limits<int16>::max()) {
		unreachable();
	}
	return static_cast<int16>(lit);
}

consteval int32 operator""_i32(unsigned long long int lit) {
	if (lit > std::numeric_limits<int32>::max()) {
		unreachable();
	}
	return static_cast<int32>(lit);
}

consteval int64 operator""_i64(unsigned long long int lit) {
	if (lit > std::numeric_limits<int64>::max()) {
		unreachable();
	}
	return static_cast<int64>(lit);
}

consteval uint8 operator""_u8(unsigned long long int lit) {
	if (lit > std::numeric_limits<uint8>::max()) {
		unreachable();
	}
	return static_cast<uint8>(lit);
}

consteval uint16 operator""_u16(unsigned long long int lit) {
	if (lit > std::numeric_limits<uint16>::max()) {
		unreachable();
	}
	return static_cast<uint16>(lit);
}

consteval uint32 operator""_u32(unsigned long long int lit) {
	if (lit > std::numeric_limits<uint32>::max()) {
		unreachable();
	}
	return static_cast<uint32>(lit);
}

consteval uint64 operator""_u64(unsigned long long int lit) {
	if (lit > std::numeric_limits<uint64>::max()) {
		unreachable();
	}
	return static_cast<uint64>(lit);
}

}
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
