#ifndef SHION_DEFINES_H_
#define SHION_DEFINES_H_

#ifdef SHION_SHARED_LIBRARY
#  ifdef WIN32
#    ifdef SHION_BUILD
#      define SHION_SYMBOL __declspec(dllexport)
#    else
#      define SHION_SYMBOL __declspec(dllimport)
#    endif
#  else
#    define SHION_SYMBOL
#  endif
#else
#  define SHION_SYMBOL
#endif

#define SHION_EXPORT

#if !defined(SHION_BUILDING_MODULES)
#  define SHION_BUILDING_MODULES 0
#endif

#define SHION_API SHION_SYMBOL

#endif /* SHION_DEFINES_H_ */
