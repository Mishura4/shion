#ifndef SHION_EXPORT_H_
#define SHION_EXPORT_H_

#define SHION_BUILDING_MODULES 1
#define SHION_EXPORT export
#define SHION_EXPORT_START export {
#define SHION_EXPORT_END }

// Include all macros and other things we need here, we are in the global module fragment
#include <cassert>
#include <climits>
#include <cstdlib>
#include <cstdint>
#include <cstddef>

#ifdef _WIN32
#  include <Windows.h>
#endif

#include <shion/common/defines.hpp>

#endif /* SHION_MODULES_H_ */
