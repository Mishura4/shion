#ifndef SHION_TYPES_H_
#define SHION_TYPES_H_

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#  include <cstddef>
#  include <cstdint>
#  include <chrono>

#  include <type_traits>
#endif /* !SHION_BUILDING_MODULES */

SHION_EXPORT namespace SHION_NAMESPACE
{

namespace aliases
{

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

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using std::size_t;

using app_clock = std::chrono::steady_clock;
using app_time = app_clock::time_point;
using app_duration = app_clock::duration;

using wall_clock = std::chrono::system_clock;
using wall_time = wall_clock::time_point;
using wall_duration = wall_clock::duration;

using ssize_t = std::ptrdiff_t;

}

using namespace aliases;

struct wildcard {
	template <typename T>
	constexpr operator T() noexcept(std::is_nothrow_constructible_v<T>) {
		return {};
	}
};

struct noop_t {
	template <typename... Args>
	constexpr wildcard operator()(Args&&...) noexcept {
		return {};
	}
};

inline constexpr auto noop = noop_t{};

struct empty {};

struct dummy_t {};

inline constexpr auto dummy = dummy_t{};

template <typename T>
using complete = std::conditional_t<std::is_void_v<T>, dummy_t, T>;

template <auto V>
struct constant { inline constexpr static auto value = V; };

template <auto V>
inline constexpr auto constant_v = constant<V>::value;

}

#endif /* SHION_TYPES_H_ */
