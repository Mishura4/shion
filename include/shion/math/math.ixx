module;

#include <shion/export.hpp>
#include <shion/common/defines.hpp>

#if !SHION_IMPORT_STD
#include <cmath>
#include <cstddef>
#include <cstdint>
#endif

export module shion:math;

#if SHION_IMPORT_STD
import std;
#endif

import :common;
import :utility;
import :meta;

using namespace SHION_NAMESPACE ::literals;

SHION_EXTERN_CPP_START
	
#include "shion/math/util.hpp"
#include "shion/math/color.hpp"
#include "shion/math/vector.hpp"

SHION_EXTERN_CPP_END
