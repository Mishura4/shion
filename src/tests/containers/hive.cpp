#include "../containers.hpp"

#include <shion/containers/hive.hpp>

namespace shion::tests {

void hive_test(test& self) {
	using hive = shion::hive<std::unique_ptr<int>>;
	hive h;

	auto first = h.emplace(std::make_unique<int>(5));
	if (first == h.end() || !*first || **first != 5) {
		self.fail("failed to emplace first value");
		return;
	}

	auto second = h.emplace(std::make_unique<int>(42));
	if (second == h.end() || !*second || **second != 42) {
		self.fail("failed to emplace second value");
		return;
	}

	TEST_ASSERT(self, h.at_raw_index(0) == first);
	TEST_ASSERT(self, h.at_raw_index(1) == second);
	TEST_ASSERT(self, h.erase(second) == h.end());
	TEST_ASSERT(self, h.at_raw_index(1) == h.end());

	second = h.emplace(std::make_unique<int>(69));
	TEST_ASSERT(self, **second == 69);
	TEST_ASSERT(self, h.at_raw_index(1) == second);

	TEST_ASSERT(self, h.erase(first) == second);
	TEST_ASSERT(self, h.at_raw_index(0) == h.end());
	TEST_ASSERT(self, h.at_raw_index(1) == second);
	first = h.emplace(std::make_unique<int>(42069));
	TEST_ASSERT(self, first == h.begin());
	TEST_ASSERT(self, h.at_raw_index(0) == first);
	TEST_ASSERT(self, h.at_raw_index(1) == second);
	TEST_ASSERT(self, h.erase(h.erase(first)) == h.end());
	TEST_ASSERT(self, h.begin() == h.end());
	TEST_ASSERT(self, h.at_raw_index(0) == h.end());
	TEST_ASSERT(self, h.at_raw_index(1) == h.end());

	int i;
	for (i = 0; i < 69; ++i) {
		auto it = h.emplace(std::make_unique<int>(i));
	}
	for (auto it = h.begin(); it != h.end();) {
		it = it.erase();
	}
	TEST_ASSERT(self, h.begin() == h.end() && h.size() == 0);

	for (i = 0; i < 420; ++i) {
		auto it = h.emplace(std::make_unique<int>(i));
	}
	for (auto it = h.begin(); it != h.end();) {
		it = it.erase();
	}
	TEST_ASSERT(self, h.begin() == h.end() && h.size() == 0);

	for (i = 0; i < 420; ++i) {
		auto it = h.emplace(std::make_unique<int>(i));
	}
	hive::iterator at_63;
	hive::iterator at_128;
	i = 0;
	for (auto it = h.begin(); it != h.end();) {
		if (i == 63) {
			at_63 = it;
			++it;
		} else if (i > 63 && i < 128) {
			it = it.erase();
		} else if (i == 128) {
			at_128 = it;
			++it;
		}
		else
			++it;
		++i;
	}
	auto bad = std::unique_ptr<int>{};
	TEST_ASSERT(self, at_63++ == at_128);
	TEST_ASSERT(self, h.get_iterator(&(*at_63)) == at_63);
	TEST_ASSERT(self, h.get_iterator(&(*at_128)) == at_128);
	TEST_ASSERT(self, h.get_iterator(&bad) == h.end());

	auto size = h.size();
	for (i = 0; i < size; ++i) {
		h.erase(h.begin());
	}
	TEST_ASSERT(self, h.begin() == h.end() && h.size() == 0);
}


}
