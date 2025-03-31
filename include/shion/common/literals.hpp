#ifndef SHION_COMMON_LITERALS_H_
#define SHION_COMMON_LITERALS_H_

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#  if !SHION_IMPORT_STD
#    include <string_view>
#    include <chrono>
#  endif

#  include <shion/common/types.hpp>
#  include <shion/common/cast.hpp>
#  include <shion/common/assert.hpp>
#endif

SHION_EXPORT namespace SHION_NAMESPACE
{

namespace literals {

using namespace std::string_view_literals;
using namespace std::chrono_literals;

consteval int8 operator""_i8(unsigned long long int lit) {
	if (lit > lossless_cast<unsigned long long int>((std::numeric_limits<int8>::max)())) {
		unreachable();
	}
	return static_cast<int8>(lit);
}

consteval int16 operator""_i16(unsigned long long int lit) {
	if (lit > lossless_cast<unsigned long long int>((std::numeric_limits<int16>::max)())) {
		unreachable();
	}
	return static_cast<int16>(lit);
}

consteval int32 operator""_i32(unsigned long long int lit) {
	if (lit > lossless_cast<unsigned long long int>((std::numeric_limits<int32>::max)())) {
		unreachable();
	}
	return static_cast<int32>(lit);
}

consteval int64 operator""_i64(unsigned long long int lit) {
	if (lit > lossless_cast<unsigned long long int>((std::numeric_limits<int64>::max)())) {
		unreachable();
	}
	return static_cast<int64>(lit);
}

consteval uint8 operator""_u8(unsigned long long int lit) {
	if (lit > (std::numeric_limits<uint8>::max)()) {
		unreachable();
	}
	return static_cast<uint8>(lit);
}

consteval uint16 operator""_u16(unsigned long long int lit) {
	if (lit > (std::numeric_limits<uint16>::max)()) {
		unreachable();
	}
	return static_cast<uint16>(lit);
}

consteval uint32 operator""_u32(unsigned long long int lit) {
	if (lit > (std::numeric_limits<uint32>::max)()) {
		unreachable();
	}
	return static_cast<uint32>(lit);
}

consteval uint64 operator""_u64(unsigned long long int lit) {
	if (lit > (std::numeric_limits<uint64>::max)()) {
		unreachable();
	}
	return static_cast<uint64>(lit);
}

consteval ssize_t operator""_sst(unsigned long long int lit) {
	if (lit > lossless_cast<unsigned long long int>((std::numeric_limits<ssize_t>::max)())) {
		unreachable();
	}
	return static_cast<ssize_t>(lit);
}

consteval ssize_t operator""_st(unsigned long long int lit) {
	if (lit > (std::numeric_limits<size_t>::max)()) {
		unreachable();
	}
	return static_cast<size_t>(lit);
}

}

using namespace literals;

}

#endif /* SHION_COMMON_LITERALS_H_ */
