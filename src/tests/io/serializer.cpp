module;

#include <shion/common/defines.hpp>

#if !SHION_IMPORT_STD
#include <cstddef>
#include <bit>
#include <string>
#include <array>
#include <tuple>
#endif

#include "../tests.hpp"

module shion.tests;

#if SHION_IMPORT_STD
import std;
#endif

import shion;

namespace shion::tests
{

constexpr bool serializer_helper_fundamental_impl(test* t)
{
	std::byte int_storage[sizeof(int)];

	shion::serializer_helper<int> s;
	
	TEST_ASSERT(*t, s.write({}, 42) == sizeof(int));
	TEST_ASSERT(*t, s.write(int_storage, 42) == sizeof(int));
	int value;
	TEST_ASSERT(*t, s.read({}, &value) == sizeof(int));
	TEST_ASSERT(*t, s.read({}, nullptr) == sizeof(int));
	TEST_ASSERT(*t, s.read(int_storage, nullptr) == sizeof(int));
	TEST_ASSERT(*t, s.read(int_storage, &value) == sizeof(int));
	TEST_ASSERT(*t, value == 42);

	auto opposite_endian = std::endian::native == std::endian::big ? std::endian::little : std::endian::big;
	
	TEST_ASSERT(*t, s.write(int_storage, 42, opposite_endian) == sizeof(int));
	TEST_ASSERT(*t, std::byteswap(std::bit_cast<int>(int_storage)) == 42);
	TEST_ASSERT(*t, s.read({}, &value, opposite_endian) == sizeof(int));
	TEST_ASSERT(*t, s.read({}, nullptr, opposite_endian) == sizeof(int));
	TEST_ASSERT(*t, s.read(int_storage, nullptr, opposite_endian) == sizeof(int));
	TEST_ASSERT(*t, s.read(int_storage, &value, opposite_endian) == sizeof(int));
	TEST_ASSERT(*t, value == 42);
	return true;
}

constexpr bool serializer_helper_tuples_impl(test* t)
{
	using t1 = tuple<int, char, float>;
	constexpr auto size1 = sizeof(t1::element<0>) + sizeof(t1::element<1>) + sizeof(t1::element<2>);
	auto t1_in = t1{ 42, 'a', 0.5f };
	auto t1_out = t1{};
	std::byte t1_storage[size1];

	shion::serializer_helper<t1> s1;

	TEST_ASSERT(*t, s1.write({}, t1_in) == size1);
	TEST_ASSERT(*t, s1.write(t1_storage, t1_in) == size1);
	TEST_ASSERT(*t, s1.read({}, &t1_out) == size1);
	
	TEST_ASSERT(*t, s1.read({}, nullptr) == size1);
	TEST_ASSERT(*t, s1.read(t1_storage, nullptr) == size1);
	TEST_ASSERT(*t, s1.read(t1_storage, &t1_out) == size1);
	TEST_ASSERT(*t, t1_out == t1_in);

	using t2 = std::array<int, 3>;
	constexpr auto size2 = sizeof(t2);
	t2 t2_in = { 0, 1, 2 };
	t2 t2_out{};
	std::byte t2_storage[size2];
	
	shion::serializer_helper<t2> s2;
	
	TEST_ASSERT(*t, s2.write({}, t2_in) == size2);
	TEST_ASSERT(*t, s2.write(t2_storage, t2_in) == size2);
	TEST_ASSERT(*t, s2.read({}, &t2_out) == size2);
	
	TEST_ASSERT(*t, s2.read({}, nullptr) == size2);
	TEST_ASSERT(*t, s2.read(t2_storage, nullptr) == size2);
	TEST_ASSERT(*t, s2.read(t2_storage, &t2_out) == size2);
	TEST_ASSERT(*t, t2_out == t2_in);

	using t3 = std::array<int, size_t{1024} * size_t{1024}>;
	constexpr auto size3 = sizeof(t3);
	t3 t3_in{};
	t3 t3_out{};
	std::byte t3_storage[size3];
	
	shion::serializer_helper<t3> s3;
	
	TEST_ASSERT(*t, s3.write({}, t3_in) == size3);
	TEST_ASSERT(*t, s3.write(t3_storage, t3_in) == size3);
	TEST_ASSERT(*t, s3.read({}, &t3_out) == size3);
	
	TEST_ASSERT(*t, s3.read({}, nullptr) == size3);
	TEST_ASSERT(*t, s3.read(t3_storage, nullptr) == size3);
	TEST_ASSERT(*t, s3.read(t3_storage, &t3_out) == size3);
	TEST_ASSERT(*t, t3_out == t3_in);
	return true;
}

constexpr bool serializer_helper_ranges_impl(test* t) {
	/*{
		std::byte                      big[64];
		std::string                    str_in("hey");
		std::string                    str_out{};
		shion::serializer_helper<std::string> test;
		auto expected_size = (sizeof(size_t) + str_in.size() * sizeof(std::string::value_type));
	
		TEST_ASSERT(*t, test.write({}, str_in) == expected_size);
		TEST_ASSERT(*t, test.write(big, str_in) == expected_size);
		TEST_ASSERT(*t, test.write({}, {}) == (sizeof(size_t)));

		TEST_ASSERT(*t, test.read({}, &str_out) == (sizeof(size_t)));
		TEST_ASSERT(*t, test.read(big, &str_out) == expected_size);
		TEST_ASSERT(*t, str_in == str_out);
	}*/

	/*
	{
		std::byte          big[sizeof(int) * 8];
		std::array<int, 8> data_in{ 0, 1, 2, 3, 4, 5, 6, 7 };
		std::array<int, 8> data_out;
		serializer_helper<std::array<int, 8>> test;
		auto expected_size = sizeof(int) * 8;
	
		TEST_ASSERT(*t, test.write({}, data_in) == expected_size);
		TEST_ASSERT(*t, test.write(big, data_in) == expected_size);
		TEST_ASSERT(*t, test.write({}, {}) == (sizeof(data_in)));

		TEST_ASSERT(*t, test.read({}, &data_out) == (sizeof(data_in)));
		TEST_ASSERT(*t, test.read(big, &data_out) == expected_size);
		TEST_ASSERT(*t, data_in == data_out);
	}*/

	shion::serializer_helper<int> inttest{};

	constexpr bool foo = shion::string_literal{"foo"} == "foo";

	constexpr auto eee = shion::serializer_helper<std::tuple<int, char>>{}.read({}, {});
	
	return true;
}

bool serializer_helper_fundamental(test& t) {
	if (!serializer_helper_fundamental_impl(&t))
		return false;

	constexpr bool constexpr_test = serializer_helper_fundamental_impl(nullptr);
	TEST_ASSERT(t, constexpr_test);

	return true;
}

bool serializer_helper_tuples(test& t) {
	if (!serializer_helper_tuples_impl(&t))
		return false;

	constexpr bool constexpr_test = serializer_helper_tuples_impl(nullptr);
	TEST_ASSERT(t, constexpr_test);

	return true;
}

bool serializer_helper_ranges(test& t) {
	if (!serializer_helper_ranges_impl(&t))
		return false;

	constexpr bool constexpr_test = serializer_helper_ranges_impl(nullptr);
	TEST_ASSERT(t, constexpr_test);

	return true;
}


}
