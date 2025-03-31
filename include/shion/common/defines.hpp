#ifndef SHION_BASE_H_
#define SHION_BASE_H_




// ------------- Config -------------
#ifndef SHION_BUILDING_MODULES
#  define SHION_BUILDING_MODULES 0
#endif /* SHION_BUILDING_MODULES */

#ifndef SHION_BUILDING_LIBRARY
#  define SHION_BUILDING_LIBRARY 0
#endif /* SHION_BUILDING_LIBRARY */

#ifndef SHION_NAMESPACE
#  define SHION_NAMESPACE shion
#endif /* SHION_NAMESPACE */

#ifndef SHION_MODULES
#  define SHION_MODULES 0
#endif /* SHION_MODULES */

#ifndef SHION_SHARED
#  define SHION_SHARED 0
#endif

#ifndef SHION_MODULE_LIBRARY
#  define SHION_MODULE_LIBRARY 0
#endif

#ifndef SHION_IMPORT_STD
#  define SHION_IMPORT_STD 0
#endif

// ------------- Internals -------------
#if SHION_SHARED
#  if _WIN32
#    if SHION_BUILD
#      define SHION_API __declspec(dllexport)
#    else
#      define SHION_API __declspec(dllimport)
#      define SHION_EXPORT_API SHION_API
#    endif /* SHION_BUILDING_LIBRARY */
#  endif /* _WIN32 */
#endif /* SHION_SHARED */

#if SHION_BUILDING_LIBRARY
#  ifndef SHION_IMPORT_STD
#    define SHION_IMPORT_STD 0
#  endif
#endif /* SHION_BUILDING_LIBRARY */

#ifndef SHION_API
#  define SHION_API
#endif /* SHION_API */

#ifndef SHION_EXPORT_API
#define SHION_EXPORT_API inline
#endif

#ifndef SHION_EXPORT
#  define SHION_EXPORT
#  define SHION_EXPORT_START
#  define SHION_EXPORT_END
#endif /* SHION_EXPORT */

#ifndef SHION_DEBUG
#  ifdef NDEBUG
#    define SHION_DEBUG 0
#  else
#    define SHION_DEBUG 1
#  endif
#endif

#ifndef SHION_RELEASE
#  ifdef NDEBUG
#    define SHION_RELEASE 1
#  else
#    define SHION_RELEASE 0
#  endif
#endif

#if defined(__clang__)
#  define SHION_STD_ATTR clang
#elif defined(_MSC_VER)
#  define SHION_STD_ATTR msvc
#elif defined(__GNUC__)
#  define SHION_STD_ATTR gnu
#else
#  define SHION_STR_ATTR
#endif

#  define SHION_LIFETIMEBOUND [[ SHION_STD_ATTR :: lifetimebound ]]

#if _MSC_VER
#  define SHION_CPPVER _MSVC_LANG
#else
#  define SHION_CPPVER __cplusplus
#endif /* _MSC_VER */

#if defined(__has_builtin)
#  define SHION_HAS_BUILTIN(a) __has_builtin(a)
#else
  // MSVC, for some reason
#  define SHION_HAS_BUILTIN(a) 0
#endif

#ifndef SHION_HAS_PFR
#  define SHION_HAS_PFR 0
#endif

#include <shion/common/assert.hpp>

#endif /* SHION_BASE_H_ */
