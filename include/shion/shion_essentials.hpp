#ifndef SHION_ESSENTIALS_H_
#define SHION_ESSENTIALS_H_

namespace shion {

namespace detail {

template <typename T, typename U>
inline constexpr bool is_abi_compatible = alignof(T) == alignof(U) && sizeof(T) == sizeof(U);

}

}
#endif
