module;

#include <shion/common/defines.hpp>

#if !SHION_IMPORT_STD

#include <cstddef>
#include <bit>
#include <string>
#include <array>
#include <tuple>
#include <span>
#include <ranges>
#include <algorithm>
#include <numeric>
#include <list>
#include <queue>
#include <unordered_map>

#endif

#include "../tests.hpp"

module shion.tests;

#if SHION_IMPORT_STD
import std;
#endif

import shion;

namespace shion::tests
{

template <typename T>
constexpr bool serializer_helper_fundamental_impl(test* t, T val)
{
	std::byte storage[sizeof(T)];

	shion::serializer_helper<T> s;
	auto opposite_endian = std::endian::native == std::endian::big ? std::endian::little : std::endian::big;
	
	TEST_ASSERT(*t, s.write({}, val) == sizeof(T));
	TEST_ASSERT(*t, s.write(storage, val) == sizeof(T));
	T value;
	TEST_ASSERT(*t, s.size({}) == sizeof(T));
	TEST_ASSERT(*t, s.read(storage, value) == sizeof(T));
	TEST_ASSERT(*t, value == val);
	auto bytes = std::span<const std::byte>(storage);
	TEST_ASSERT(*t, s.construct(bytes) == val);
	bytes = std::span<const std::byte>(storage);
	if constexpr (!std::floating_point<T>)
	{
		TEST_ASSERT(*t, s.construct(bytes, opposite_endian) == std::byteswap(val));
	}
	else
	{
		TEST_ASSERT(*t, s.construct(bytes, opposite_endian) == val);
	}
	
	TEST_ASSERT(*t, s.write(storage, val, opposite_endian) == sizeof(T));
	if constexpr (!std::floating_point<T>)
	{
		TEST_ASSERT(*t, std::byteswap(std::bit_cast<T>(storage)) == val);
	}
	else
	{
		TEST_ASSERT(*t, std::bit_cast<T>(storage) == val);
	}
	TEST_ASSERT(*t, s.size({}, opposite_endian) == sizeof(T));
	TEST_ASSERT(*t, s.read(storage, value, opposite_endian) == sizeof(T));
	TEST_ASSERT(*t, value == val);
	bytes = std::span<const std::byte>(storage);
	TEST_ASSERT(*t, s.construct(bytes, opposite_endian) == val);
	return true;
}

constexpr bool serializer_helper_tuples_impl(test* t)
{
	using t1 = tuple<int, char, float>;
	constexpr auto size1 = sizeof(t1::element<0>) + sizeof(t1::element<1>) + sizeof(t1::element<2>);
	auto t1_in = t1{ 42, 'a', 0.5f };
	auto t1_out = t1{};
	shion::byte t1_storage[size1];

	shion::serializer_helper<t1> s1;

	TEST_ASSERT(*t, s1.write({}, t1_in) == size1);
	TEST_ASSERT(*t, s1.write(t1_storage, t1_in) == size1);

	TEST_ASSERT(*t, s1.size({}) == size1);
	TEST_ASSERT(*t, s1.read(t1_storage, t1_out) == size1);
	TEST_ASSERT(*t, t1_out == t1_in);

	auto t1_bytes = std::span<const std::byte>(t1_storage);
	TEST_ASSERT(*t, s1.construct(t1_bytes) == t1_in);
	return true;
}

constexpr bool serializer_helper_array(test* t)
{
	using array = std::array<char, 64>;
	std::byte                      big[64]{};
	array str_in{};
	array str_out{};
	shion::serializer_helper<array> test;
	ptrdiff_t expected_size = str_in.size();
	std::ranges::copy("hello world!", str_in.begin());
	
	TEST_ASSERT(*t, test.write({}, str_in) == expected_size);
	TEST_ASSERT(*t, test.write(big, str_in) == expected_size);
	TEST_ASSERT(*t, test.write({}, {}) == expected_size);

	TEST_ASSERT(*t, test.size({}) == expected_size);
	TEST_ASSERT(*t, test.read(big, str_out) == expected_size);
	TEST_ASSERT(*t, str_in == str_out);
	return true;
}

constexpr bool serializer_helper_ranges_string(test* t)
{
	std::byte                      big[64]{};
	std::string                    str_in("hey");
	std::string                    str_out{};
	shion::serializer_helper<std::string> test;
	ptrdiff_t expected_size = (1 + str_in.size() * sizeof(std::string::value_type));
	
	TEST_ASSERT(*t, test.write({}, str_in) == expected_size);
	TEST_ASSERT(*t, test.write(big, str_in) == expected_size);
	TEST_ASSERT(*t, test.write({}, {}) == 1);

	TEST_ASSERT(*t, test.size({}) == -1);
	TEST_ASSERT(*t, test.read(big, str_out) == expected_size);
	TEST_ASSERT(*t, str_in == str_out);
	return true;
}

constexpr bool serializer_helper_ranges_big_string(test* t)
{
	auto do_test = [&](ptrdiff_t bytes, ptrdiff_t expected_size_bytes)
	{
		std::vector<std::byte> big(bytes + expected_size_bytes);
		std::string            bigstr_in{};
		std::string            bigstr_out{};

		bigstr_in.resize(bytes);
		std::ranges::fill_n(std::begin(bigstr_in), bigstr_in.size(), 'a');
		shion::serializer_helper<std::string> test;
		ptrdiff_t expected_size = (expected_size_bytes + bigstr_in.size() * sizeof(std::string::value_type));
	
		TEST_ASSERT(*t, test.write({}, bigstr_in) == expected_size);
		TEST_ASSERT(*t, test.write(big, bigstr_in) == expected_size);
		TEST_ASSERT(*t, test.write({}, {}) == 1);
		
		TEST_ASSERT(*t, test.size({}) == -1);
		TEST_ASSERT(*t, test.size(std::span{big}.subspan(0, expected_size_bytes)) == expected_size_bytes + bytes);
		TEST_ASSERT(*t, test.read(big, bigstr_out) == expected_size);
		TEST_ASSERT(*t, bigstr_in == bigstr_out);
		return true;
	};
	if (!do_test(0b01111111, 1))
		return false;
	if (!do_test(0b10000000, 2))
		return false;
	if (!do_test(0b00111111'11111111, 2))
		return false;
	if (!do_test(0b01111111'11111111, 3))
		return false;
	return true;
}

constexpr bool serializer_helper_ranges_vector_string(test* t)
{
	using vec = std::vector<std::string>;
	std::byte              big[4096]{};
	vec                    vec_in({ "hello", " ", "world", "!"});
	vec                    vec_out;
	serializer_helper<vec> test;
	ptrdiff_t expected_size = (1 + (1 * 4) + vec_in[0].size() + vec_in[1].size() + vec_in[2].size() + vec_in[3].size());
	
	TEST_ASSERT(*t, test.write({}, vec_in) == expected_size);
	TEST_ASSERT(*t, test.write(big, vec_in) == expected_size);
	TEST_ASSERT(*t, test.write({}, {}) == (1));

	TEST_ASSERT(*t, test.size({}) == -1);
	TEST_ASSERT(*t, test.read(big, vec_out) == expected_size);
	TEST_ASSERT(*t, std::ranges::equal(vec_in, vec_out));
	return true;
}

bool serializer_helper_fundamental(test& t) {
	if (!serializer_helper_fundamental_impl(&t, 42))
		return false;

	if (!serializer_helper_fundamental_impl(&t, 0.42f))
		return false;

	return true;
}

bool serializer_helper_tuples(test& t) {
	if (!serializer_helper_tuples_impl(&t))
		return false;

	if (!serializer_helper_array(&t))
		return false;
	
	return true;
}

bool serializer_helper_contiguous_ranges(test& t) {
	if (!serializer_helper_ranges_string(&t))
		return false;

	if (!serializer_helper_ranges_big_string(&t))
		return false;

	if (!serializer_helper_ranges_vector_string(&t))
		return false;

	return true;
}

bool serializer_helper_ranges_list(test* t)
{
	std::byte                      big[64]{};
	std::list<int>                 list_in{ 1, 2, 4, 8 };
	std::list<int>                 list_out{};
	shion::serializer_helper<std::list<int>> test;
	ptrdiff_t expected_size = (1 + list_in.size() * sizeof(int));
	
	TEST_ASSERT(*t, test.write({}, list_in) == expected_size);
	TEST_ASSERT(*t, test.write(big, list_in) == expected_size);
	TEST_ASSERT(*t, test.write({}, {}) == 1);

	TEST_ASSERT(*t, test.size({}) == -1);
	TEST_ASSERT(*t, test.read(big, list_out) == expected_size);
	TEST_ASSERT(*t, list_in == list_out);
	return true;
}

bool serializer_helper_ranges_deque(test* t)
{
	std::byte                      big[64]{};
	std::deque<int>                 list_in{ 1, 2, 4, 8 };
	std::deque<int>                 list_out{};
	shion::serializer_helper<std::deque<int>> test;
	ptrdiff_t expected_size = (1 + list_in.size() * sizeof(int));
	
	TEST_ASSERT(*t, test.write({}, list_in) == expected_size);
	TEST_ASSERT(*t, test.write(big, list_in) == expected_size);
	TEST_ASSERT(*t, test.write({}, {}) == 1);

	TEST_ASSERT(*t, test.size({}) == -1);
	TEST_ASSERT(*t, test.read(big, list_out) == expected_size);
	TEST_ASSERT(*t, list_in == list_out);
	return true;
}

bool serializer_helper_ranges_umap(test* t)
{
	std::byte                      big[1024]{};
	using umap = std::unordered_map<std::string, int>;
	umap                 list_in{
		{ "one", 1 }, { "two", 2 }, { "four", 4 }, { "eight", 8 }
	};
	umap                 list_out{};
	shion::serializer_helper<umap> test;
	ptrdiff_t expected_size = (
		1 + list_in.size() * sizeof(int)
		+ std::ranges::fold_left(list_in | std::views::keys, size_t{0}, [](size_t sz, const std::string& value) {
			return sz + serializer_helper<std::string>().write({}, value);
		})
	);
	
	TEST_ASSERT(*t, test.write({}, list_in) == expected_size);
	TEST_ASSERT(*t, test.write(big, list_in) == expected_size);
	TEST_ASSERT(*t, test.write({}, {}) == 1);

	TEST_ASSERT(*t, test.size({}) == -1);
	TEST_ASSERT(*t, test.read(big, list_out) == expected_size);
	TEST_ASSERT(*t, list_in == list_out);
	return true;
}

bool serializer_helper_list_ranges(test& t) {
	
	if (!serializer_helper_ranges_list(&t))
		return false;

	if (!serializer_helper_ranges_deque(&t))
		return false;

	if (!serializer_helper_ranges_umap(&t))
		return false;

	return true;
}

}
