#ifndef SHION_BITMASK_H_
#define SHION_BITMASK_H_

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#  if !SHION_IMPORT_STD
#    include <utility>
#    include <type_traits>
#  endif
#endif /* SHION_BUILDING_MODULES */

namespace shion {

SHION_EXPORT template <typename T>
struct bit_mask;

SHION_EXPORT template <typename T>
requires (std::is_enum_v<T>)
struct bit_mask<T> {
	constexpr bit_mask() = default;
	constexpr bit_mask(std::initializer_list<T> rhs) noexcept : value{} {
		for (T flag : rhs) {
			value |= std::to_underlying(flag);
		}
	}
	constexpr bit_mask(std::underlying_type_t<T> rhs) noexcept : value{rhs} {}
	constexpr bit_mask(T rhs) noexcept : value{static_cast<std::underlying_type_t<T>>(rhs)} {}

	std::underlying_type_t<T> value;

	constexpr operator T() const {
		return value;
	}

	friend constexpr bit_mask operator|(bit_mask lhs, bit_mask rhs) noexcept {
		return {static_cast<T>(lhs.value | rhs.value)};
	}

	friend constexpr bit_mask operator&(bit_mask lhs, bit_mask rhs) noexcept {
		return {static_cast<T>(lhs.value & rhs.value)};
	}

	friend constexpr bit_mask operator^(bit_mask lhs, bit_mask rhs) noexcept {
		return {static_cast<T>(lhs.value ^ rhs.value)};
	}

	friend constexpr bit_mask operator&(T lhs, bit_mask rhs) noexcept {
		return {static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) & rhs)};
	}

	friend constexpr bit_mask operator|(T lhs, bit_mask rhs) noexcept {
		return {static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) | rhs)};
	}

	friend constexpr bit_mask operator^(T lhs, bit_mask rhs) noexcept {
		return {static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) ^ rhs)};
	}

	friend constexpr bit_mask operator~(bit_mask mask) {
		return {~static_cast<T>(static_cast<std::underlying_type_t<T>>(mask))};
	}

	constexpr bit_mask& operator|=(bit_mask rhs) noexcept {
		value |= rhs.value;
		return *this;
	}

	constexpr bit_mask& operator&=(bit_mask rhs) noexcept {
		value &= rhs.value;
		return *this;
	}

	constexpr bit_mask& operator^=(bit_mask rhs) noexcept {
		value ^= rhs.value;
		return *this;
	}

	constexpr bit_mask& operator=(T rhs) noexcept {
		value = static_cast<std::underlying_type_t<T>>(rhs);
		return *this;
	}

	constexpr bool operator==(bit_mask const&) const noexcept = default;

	constexpr auto operator<=>(bit_mask const&) const noexcept = default;

	constexpr operator bool() const noexcept {
		return (value != 0);
	}
};

SHION_EXPORT template <typename T>
bit_mask(T) -> bit_mask<T>;

SHION_EXPORT template <typename T>
bit_mask<T> bit_if(T flag, bool condition) noexcept {
	return (condition ? bit_mask<T>{flag} : bit_mask<T>{});
}

}

#endif /* SHION_BITMASK_H_ */
