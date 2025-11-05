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

inline constexpr auto close_file = [](std::FILE* f) {
	return std::fclose(f);
};

using managed_file = shion::unique_ptr<std::FILE, close_file>;

inline std::vector<byte> read_file(const char *path) {
#if defined(__GNUC__) && __GNUC__ < 15 && __GNUC_MINOR__ < 2
	auto f = managed_file{ fopen(path, "rb") };
#else
	auto f = std::unique_ptr<std::FILE, decltype(close_file)> { std::fopen(path, "rb") };
#endif

	if (!f) {
		return {};
	}

	if (std::fseek(f.get(), 0, std::ios_base::end) != 0)
		return {};
	int sz = std::ftell(f.get());
	if (std::fseek(f.get(), 0, std::ios_base::beg) != 0)
		return {};

	std::vector<byte> ret{static_cast<size_t>(sz)};
	auto read_sz = std::fread(ret.data(), 1, ret.size(), f.get());

	if (read_sz < static_cast<unsigned int>(sz)) {
		ret.resize(read_sz);
		ret.shrink_to_fit();
	}

	return ret;
}

}

}

#endif

