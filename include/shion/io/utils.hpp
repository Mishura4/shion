#ifndef SHION_IO_UTILS_H_
#define SHION_IO_UTILS_H_

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#include <cstdio>
#include <vector>
#include <shion/common.hpp>
#include <shion/utility/unique_handle.hpp>
#endif

SHION_EXPORT namespace SHION_NAMESPACE {

inline namespace io {

inline constexpr auto close_file = [](FILE* f) {
	return fclose(f);
};

using managed_file = shion::unique_ptr<FILE, close_file>;

inline std::vector<byte> read_file(const char *path) {
#if defined(__GNUC__) && __GNUC__ < 15 && __GNUC_MINOR__ < 2
	auto f = managed_file{ fopen(path, "rb") };
#else
	auto f = std::unique_ptr<FILE, decltype(close_file)> { fopen(path, "rb") };
#endif

	if (!f) {
		return {};
	}

	if (fseek(f.get(), 0, SEEK_END) != 0)
		return {};
	int sz = ftell(f.get());
	if (fseek(f.get(), 0 , SEEK_SET) != 0)
		return {};

	std::vector<byte> ret{static_cast<size_t>(sz)};
	auto read_sz = fread(ret.data(), 1, ret.size(), f.get());

	if (read_sz < static_cast<unsigned int>(sz)) {
		ret.resize(read_sz);
		ret.shrink_to_fit();
	}

	return ret;
}

}

}

#endif

