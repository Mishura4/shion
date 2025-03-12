#ifndef SHION_MODULES_H_
#define SHION_MODULES_H_

#include "shion/defines.hpp"

#undef SHION_BUILDING_MODULES
#undef SHION_DO_INCLUDES

#define SHION_DO_INCLUDES 0
#define SHION_BUILDING_MODULES 1
#define SHION_USE_MODULES 1

#undef SHION_MOD_INLINE
#define SHION_MOD_INLINE

#undef SHION_EXPORT
#undef SHION_EXPORT_START
#undef SHION_EXPORT_END

#define SHION_EXPORT export 
#define SHION_EXPORT_START export{
#define SHION_EXPORT_END }

#undef SHION_API

#define SHION_API SHION_EXPORT SHION_SYMBOL

namespace shion {}

// Include all macros and other things we need here, we are in the global module fragment
#include <cassert>
#include <climits>
#include <cstdlib>
#include <cstdint>
#include <cstddef>

#ifdef _WIN32
#  include <Windows.h>
#endif

#endif /* SHION_MODULES_H_ */
