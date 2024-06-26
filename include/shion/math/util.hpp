#ifndef SHION_MATH_UTIL_H_
#define SHION_MATH_UTIL_H_

#include <limits>
#include <type_traits>

#include "../shion_essentials.hpp"
#include "../tools.hpp"

namespace shion {

template <class T, class U>
struct ratio {
	T count;
	U max;

	constexpr bool valid() const noexcept {
		return (max != 0);
	}

	constexpr double divide() {
		SHION_ASSERT(valid());
		return (static_cast<double>(count) / static_cast<double>(max));
	}

	constexpr double get_percent() {
		return (divide() * 100);
	}
};

}

#endif
