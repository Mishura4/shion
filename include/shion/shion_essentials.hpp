#ifndef SHION_ESSENTIALS_H_
#define SHION_ESSENTIALS_H_

#include <string_view>
#include <chrono>
#include <cstddef>
#include <cassert>
#include <type_traits>

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

#ifndef NDEBUG
#  define SHION_ASSERT(a) assert(a)
#else
#  define SHION_ASSERT(a) if (!(a)) shion::unreachable();
#endif


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

using ssize_t = std::make_signed_t<size_t>;

}

/**
 * @brief Casts an integer to another type, raising an assertion failure in debug if the target type cannot hold the value.
 */
template <std::integral To, std::integral From>
constexpr To lossless_cast(From v) noexcept {
	if constexpr (std::is_same_v<To, From>) {
		return v;
	}
	else {
		if constexpr (std::is_unsigned_v<From>) {
			SHION_ASSERT(v <= static_cast<std::make_unsigned_t<To>>(std::numeric_limits<To>::max()));
		} else {
			if constexpr (std::is_unsigned_v<To>) {
				SHION_ASSERT(v >= 0);
			} else {
				SHION_ASSERT(v >= std::numeric_limits<To>::min());
			}
		}
		return static_cast<To>(v);
	}
}

using namespace aliases;

namespace literals {

using namespace std::string_view_literals;
using namespace std::chrono_literals;

consteval int8 operator""_i8(unsigned long long int lit) {
	if (lit > lossless_cast<unsigned long long int>(std::numeric_limits<int8>::max())) {
		unreachable();
	}
	return static_cast<int8>(lit);
}

consteval int16 operator""_i16(unsigned long long int lit) {
	if (lit > lossless_cast<unsigned long long int>(std::numeric_limits<int16>::max())) {
		unreachable();
	}
	return static_cast<int16>(lit);
}

consteval int32 operator""_i32(unsigned long long int lit) {
	if (lit > lossless_cast<unsigned long long int>(std::numeric_limits<int32>::max())) {
		unreachable();
	}
	return static_cast<int32>(lit);
}

consteval int64 operator""_i64(unsigned long long int lit) {
	if (lit > lossless_cast<unsigned long long int>(std::numeric_limits<int64>::max())) {
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

consteval ssize_t operator""_sst(unsigned long long int lit) {
	if (lit > lossless_cast<unsigned long long int>(std::numeric_limits<ssize_t>::max())) {
		unreachable();
	}
	return static_cast<ssize_t>(lit);
}

consteval ssize_t operator""_st(unsigned long long int lit) {
	if (lit > std::numeric_limits<size_t>::max()) {
		unreachable();
	}
	return static_cast<size_t>(lit);
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

struct wildcard {
	template <typename T>
	constexpr operator T() noexcept(std::is_nothrow_constructible_v<T>) {
		return {};
	}
};

inline constexpr auto noop = []<typename... Args>(Args&&...) constexpr noexcept {
	return wildcard{};
};

/**
 * @brief In release, this returns a "default value". In debug, this calls std::unreachable.
 */
inline wildcard safe_unreachable(wildcard) {
	return wildcard{};
}

enum class value_type {
	value,
	const_value,
	lvalue_reference,
	const_lvalue_reference,
	rvalue_reference,
	const_rvalue_reference
};

template <value_type>
inline constexpr bool is_reference_type = false;

template <>
inline constexpr bool is_reference_type<value_type::lvalue_reference> = true;

template <>
inline constexpr bool is_reference_type<value_type::const_lvalue_reference> = true;

template <>
inline constexpr bool is_reference_type<value_type::rvalue_reference> = true;

template <>
inline constexpr bool is_reference_type<value_type::const_rvalue_reference> = true;

template <value_type>
inline constexpr bool is_value_type = false;

template <>
inline constexpr bool is_value_type<value_type::value> = true;

template <>
inline constexpr bool is_value_type<value_type::const_value> = true;

template <value_type>
inline constexpr bool is_lvalue_reference_type = false;

template <>
inline constexpr bool is_lvalue_reference_type<value_type::lvalue_reference> = true;

template <>
inline constexpr bool is_lvalue_reference_type<value_type::const_lvalue_reference> = true;

template <value_type>
inline constexpr bool is_rvalue_reference_type = false;

template <>
inline constexpr bool is_rvalue_reference_type<value_type::rvalue_reference> = true;

template <>
inline constexpr bool is_lvalue_reference_type<value_type::const_rvalue_reference> = true;

template <value_type>
inline constexpr bool is_const_type = false;

template <>
inline constexpr bool is_const_type<value_type::const_value> = true;

template <value_type Type>
decltype(auto) forward_like_type(auto& value) noexcept {
	using provided_t = decltype(value);
	using value_t = std::remove_reference_t<provided_t>;
	using stripped_t = std::remove_const_t<value_t>;

	if constexpr (is_value_type<Type> || is_rvalue_reference_type<Type>) {
		if constexpr (is_const_type<Type>) {
			return static_cast<std::add_rvalue_reference_t<std::add_const_t<stripped_t>>>(value);
		} else {
			return static_cast<std::add_rvalue_reference_t<stripped_t>>(value);
		}
	} else {
		static_assert(is_lvalue_reference_type<Type>);
		if constexpr (is_const_type<Type>) {
			return static_cast<std::add_lvalue_reference_t<std::add_const_t<stripped_t>>>(value);
		} else {
			return static_cast<std::add_lvalue_reference_t<stripped_t>>(value);
		}
	}
}

template <typename T>
inline constexpr bool is_copyable = std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>;

template <typename T>
inline constexpr bool is_moveable = std::is_move_constructible_v<T> && std::is_move_assignable_v<T>;

template <typename T>
inline constexpr bool is_nothrow_copyable = std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_copy_assignable_v<T>;

template <typename T>
inline constexpr bool is_nothrow_moveable = std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>;

struct dummy_t {};

inline constexpr auto dummy = dummy_t{};

template <typename T>
using complete = std::conditional_t<std::is_void_v<T>, dummy_t, T>;

template <auto V>
struct constant { inline constexpr static auto value = V; };

template <auto V>
inline constexpr auto constant_v = constant<V>::value;

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
