module;

#include <shion/export.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>


#if !SHION_IMPORT_STD
#include <limits>
#include <type_traits>
#include <stdexcept>
#include <new>
#include <utility>
#include <algorithm>

#include <chrono> // Workaround g++ bug
#endif

export module shion:math;

#if SHION_IMPORT_STD
import std;
#endif
import :common;
import :utility;
import :meta;

using namespace SHION_NAMESPACE ::literals;

#if SHION_EXTERN_MODULES
extern "C++" {
#endif
	
#include "shion/math/util.hpp"
#include "shion/math/color.hpp"
#include "shion/math/vector.hpp"

#if SHION_EXTERN_MODULES
}
#endif
