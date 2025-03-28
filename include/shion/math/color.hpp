#ifndef SHION_COLOR_H_
#define SHION_COLOR_H_

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#include <type_traits>
#include <shion/common.hpp>
#endif

namespace shion {

struct frgb {
	float r;
	float g;
	float b;
	
	static const frgb red;
	static const frgb blue;
	static const frgb green;
};

inline constexpr frgb frgb::red = {1.0f, 0.0f, 0.0f};
inline constexpr frgb frgb::blue = {0.0f, 1.0f, 0.0f};
inline constexpr frgb frgb::green = {0.0f, 0.0f, 1.0f};

struct frgba {
	float r;
	float g;
	float b;
	float a;
};

}

#endif
