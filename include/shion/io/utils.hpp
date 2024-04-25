#ifndef SHION_IO_UTILS_H_
#define SHION_IO_UTILS_H_

#include <cstdio>

#include "../shion_essentials.hpp"
#include "../tools.hpp"

namespace shion {

inline namespace io {

inline constexpr auto close_file = [](FILE* f) {
	return fclose(f);
};

using managed_file = shion::managed_ptr<FILE, close_file>;

std::vector<byte> read_file(const char *path) {
	auto f = managed_file{fopen(path, "rb")};

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

