#ifndef SHION_EXPORT_H_
#define SHION_EXPORT_H_

#define SHION_BUILDING_MODULES 1

#define SHION_EXPORT export
#define SHION_EXPORT_START export {
#define SHION_EXPORT_END }

#if defined(SHION_EXTERN_MODULES) && SHION_EXTERN_MODULES
#	define SHION_EXTERN_CPP extern "C++"
#	define SHION_EXTERN_CPP_START extern "C++" {
#	define SHION_EXTERN_CPP_END }
#else
#	define SHION_EXTERN_CPP
#	define SHION_EXTERN_CPP_START
#	define SHION_EXTERN_CPP_END
#endif

#include <shion/common/defines.hpp>

#endif /* SHION_MODULES_H_ */
