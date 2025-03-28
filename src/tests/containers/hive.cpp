module;

#include <vector>
#include <string_view>
#include <string>
#include <concepts>
#include <utility>
#include <functional>
#include <source_location>
#include <optional>
#include <chrono>

#include "../tests.hpp"

module shion.tests;

#if SHION_MODULES
import shion;
#endif
namespace shion::tests {

struct non_trivial {
	non_trivial() = delete;
	non_trivial(ssize_t a) noexcept : i(a) {}
	non_trivial(const non_trivial&) = delete;
	non_trivial(non_trivial&& rhs) noexcept : i(rhs.i) {}

	non_trivial& operator=(const non_trivial&) = delete;
	non_trivial& operator=(non_trivial&& rhs) noexcept {
		i = rhs.i;
		return *this;
	}

	bool operator==(ssize_t other) const noexcept {
		return other == i;
	}

	ssize_t i;
};

template <typename T>
void hive_test(test& self) {
	using hive = shion::hive<T>;
	hive h;

	auto first = h.emplace(5);
	if (first == h.end() || *first != 5) {
		self.fail("failed to emplace first value");
		return;
	}

	auto second = h.emplace(42);
	if (second == h.end() || *second != 42) {
		self.fail("failed to emplace second value");
		return;
	}

	TEST_ASSERT(self, h.at_raw_index(0) == first);
	TEST_ASSERT(self, h.at_raw_index(1) == second);
	TEST_ASSERT(self, h.erase(second) == h.end());
	TEST_ASSERT(self, h.at_raw_index(1) == h.end());

	second = h.emplace(69);
	TEST_ASSERT(self, *second == 69);
	TEST_ASSERT(self, h.at_raw_index(1) == second);

	TEST_ASSERT(self, h.erase(first) == second);
	TEST_ASSERT(self, h.at_raw_index(0) == h.end());
	TEST_ASSERT(self, h.at_raw_index(1) == second);
	first = h.emplace(42069);
	TEST_ASSERT(self, first == h.begin());
	TEST_ASSERT(self, h.at_raw_index(0) == first);
	TEST_ASSERT(self, h.at_raw_index(1) == second);
	TEST_ASSERT(self, h.erase(h.erase(first)) == h.end());
	TEST_ASSERT(self, h.begin() == h.end());
	TEST_ASSERT(self, h.at_raw_index(0) == h.end());
	TEST_ASSERT(self, h.at_raw_index(1) == h.end());
	
	typename hive::iterator it;
	int i;
	for (i = 0; i < 69; ++i) {
		it = h.emplace(i);
	}
	for (it = h.begin(); it != h.end();) {
		it = it.erase();
	}
	TEST_ASSERT(self, h.begin() == h.end() && h.size() == 0);

	for (i = 0; i < 420; ++i) {
		it = h.emplace(i);
	}
	for (it = h.begin(); it != h.end();) {
		it = it.erase();
	}
	TEST_ASSERT(self, h.begin() == h.end() && h.size() == 0);

	for (i = 0; i < 420; ++i) {
		it = h.emplace(i);
	}
	typename hive::iterator at_63;
	typename hive::iterator at_128;
	i = 0;
	for (it = h.begin(); it != h.end();) {
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
	auto bad = T{0};
	TEST_ASSERT(self, at_63++ == at_128);
	TEST_ASSERT(self, h.get_iterator(&(*at_63)) == at_63);
	TEST_ASSERT(self, h.get_iterator(&(*at_128)) == at_128);
	TEST_ASSERT(self, h.get_iterator(&bad) == h.end());

	auto size = h.size();
	for (i = 0; i < size; ++i) {
		h.erase(h.begin());
	}
	TEST_ASSERT(self, h.begin() == h.end() && h.size() == 0);

	for (i = 0; i < detail::hive_page_num_elements<T>; ++i) {
		h.emplace(i);
	}
	TEST_ASSERT(self, h.capacity() == detail::hive_page_num_elements<T>);
	auto first_second_page = h.emplace(i);
	TEST_ASSERT(self, *first_second_page == i);
	TEST_ASSERT(self, h.capacity() == 2 * detail::hive_page_num_elements<T>);
	it = h.begin();
	for (i = 0; i < detail::hive_page_num_elements<T>; ++i) {
		TEST_ASSERT(self, *it == i);
		++it;
	}
	TEST_ASSERT(self, it != h.end());
	TEST_ASSERT(self, it == first_second_page);
	TEST_ASSERT(self, it == h.at_raw_index(detail::hive_page_num_elements<T>));
	it = h.begin();
	for (i = 0; i < detail::hive_page_num_elements<T>; ++i) {
		TEST_ASSERT(self, *it == i);
		it = it.erase();
	}
	TEST_ASSERT(self, it == h.at_raw_index(detail::hive_page_num_elements<T>));
	TEST_ASSERT(self, h.end() == h.at_raw_index(0));
	TEST_ASSERT(self, it != h.end());
	TEST_ASSERT(self, it == first_second_page);
	TEST_ASSERT(self, it == h.begin() && std::addressof(*it) == std::addressof(*h.begin()));
	it = it.erase();
	TEST_ASSERT(self, h.begin() == h.end() && h.size() == 0);
	h.clear();
	TEST_ASSERT(self, h.capacity() == 0);
	auto result = h.try_emplace(42, 42);
	TEST_ASSERT(self, result.second);
	TEST_ASSERT(self, *result.first == 42);
	TEST_ASSERT(self, result.first == h.begin() && (++result.first) == h.end());
	result = h.try_emplace(42, 69);
	TEST_ASSERT(self, result.second == false);
	TEST_ASSERT(self, *result.first == 42);
	TEST_ASSERT(self, result.first == h.begin() && (++result.first) == h.end());
	result = h.try_emplace(21, 21);
	TEST_ASSERT(self, result.second);
	TEST_ASSERT(self, *result.first == 21);
	TEST_ASSERT(self, result.first == h.begin() && *(++result.first) == 42);
	result = h.try_emplace(69, 69);
	TEST_ASSERT(self, result.second);
	TEST_ASSERT(self, *result.first == 69);
	TEST_ASSERT(self, *h.begin() == 21 && (++result.first) == h.end());
	result = h.try_emplace(4 * detail::hive_page_num_elements<T>, 4 * detail::hive_page_num_elements<T>);
	TEST_ASSERT(self, result.second);
	TEST_ASSERT(self, *result.first == 4 * detail::hive_page_num_elements<T>);
	TEST_ASSERT(self, *h.begin() == 21 && (++result.first) == h.end());
	result = h.try_emplace(2 * detail::hive_page_num_elements<T>, 2 * detail::hive_page_num_elements<T>);
	TEST_ASSERT(self, result.second);
	TEST_ASSERT(self, *result.first == 2 * detail::hive_page_num_elements<T>);
	TEST_ASSERT(self, *h.begin() == 21 && *(++result.first) == 4 * detail::hive_page_num_elements<T>);
	TEST_ASSERT(self, h.pages() == 3);
	TEST_ASSERT(self, h.size() == 5);


	if constexpr (std::is_copy_constructible_v<T>) {
		TEST_ASSERT(self, std::is_copy_constructible_v<hive>);
	} else {
		TEST_ASSERT(self, !std::is_copy_constructible_v<hive>);
	}
	if constexpr (std::is_move_constructible_v<T>) {
		TEST_ASSERT(self, std::is_move_constructible_v<hive>);
	} else {
		TEST_ASSERT(self, !std::is_move_constructible_v<hive>);
	}
	if constexpr (std::is_copy_assignable_v<T>) {
		TEST_ASSERT(self, std::is_copy_assignable_v<hive>);
	} else {
		TEST_ASSERT(self, !std::is_copy_assignable_v<hive>);
	}
	if constexpr (std::is_move_assignable_v<T>) {
		TEST_ASSERT(self, std::is_move_assignable_v<hive>);
	} else {
		TEST_ASSERT(self, !std::is_move_assignable_v<hive>);
	}
}


void hive_test_trivial(test& self) {
	return hive_test<ssize_t>(self);
}

void hive_test_nontrivial(test& self) {
	return hive_test<non_trivial>(self);
}

}
