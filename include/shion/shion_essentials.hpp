#ifndef SHION_ESSENTIALS_H_
#define SHION_ESSENTIALS_H_

namespace shion {

namespace detail {

template <typename T, typename U>
inline constexpr bool is_abi_compatible = alignof(T) == alignof(U) && sizeof(T) == sizeof(U);

}

#ifdef SHION_SHARED_LIBRARY
#  ifdef WIN32
#    ifdef SHION_BUILD
#      define SHION_API __declspec(dllexport)
#    else
#      define SHION_API __declspec(dllimport)
#    endif
#  else
#    define SHION_API
#  endif
#else
#  define SHION_API
#endif

}
#endif
