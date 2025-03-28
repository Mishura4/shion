module;

#include <shion/export.hpp>
#include <shion/common/detail.hpp>

#if !SHION_IMPORT_STD
#include <limits>
#include <type_traits>
#include <stdexcept>
#include <new>
#include <utility>
#include <cmath>
#endif

export module shion:math;

#if SHION_IMPORT_STD
import std;
#endif
import :common;
import :utility;
import :meta;

#if SHION_EXTERN_MODULES
extern "C++" {
#endif
	
#include "shion/math/util.hpp"
#include "shion/math/color.hpp"
#include "shion/math/vector.hpp"

#if SHION_EXTERN_MODULES
}
#endif
