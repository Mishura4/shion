#ifndef SHION_UTILITY_UUID_H_
#define SHION_UTILITY_UUID_H_

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES

#include <cstddef>
#include <cstdint>
#include <ranges>
#include <algorithm>
#include <cstring>
#include <string>
#include <charconv>
#include <random>
#include <chrono>
#include <thread>
#include <type_traits>

#endif

SHION_EXPORT namespace SHION_NAMESPACE
{

inline constexpr char* to_hex(char* buf, const unsigned char* val, size_t n) noexcept
{
	for (size_t i = 0; i < n; ++i)
	{
		uint8_t high = (val[i] >> 4);
		uint8_t low = val[i] & 0x0F;
	
		buf[i * 2] = high >= 10 ? 'a' + high : '0' + high;
		buf[i * 2 + 1] = low >= 10 ? 'a' + low : '0' + low;
	}
	return buf + n;
}

inline constexpr char* to_hex(char* buf, const std::byte* val, size_t n) noexcept
{
	for (size_t i = 0; i < n; ++i)
	{
		auto value = static_cast<unsigned char>(val[i]);
		uint8_t high = (value >> 4);
		uint8_t low = value & 0x0F;
	
		buf[i * 2] = high >= 10 ? 'a' - 10 + high : '0' + high;
		buf[i * 2 + 1] = low >= 10 ? 'a' - 10 + low : '0' + low;
	}
	return buf + n * 2;
}

class uuid
{
public:
	static_assert(sizeof(uint64_t) == 8 && alignof(uint64_t) == 8);

	constexpr uuid() = default;
	constexpr uuid(const uuid&) noexcept = default;
	constexpr uuid(uuid&&) noexcept = default;
	constexpr ~uuid() = default;
	
	constexpr uuid& operator=(const uuid&) noexcept = default;
	constexpr uuid& operator=(uuid&&) noexcept = default;
	
	static const uuid nil;
	static const uuid max;

	static constexpr uuid nil_uuid() noexcept
	{
		return uuid{};
	}

	static constexpr uuid max_uuid() noexcept
	{
		uuid ret;

		std::ranges::fill(ret._bytes, static_cast<std::byte>(std::numeric_limits<uint8_t>::max()));
		return ret;
	}

	template <typename Random = std::mt19937_64>
	requires (std::invocable<Random> && std::integral<std::invoke_result_t<Random>>)
	static constexpr uuid generate_v4(Random&& rand) noexcept (std::is_nothrow_invocable_v<Random>)
	{
		using result = std::invoke_result_t<Random>;
		uuid ret;

		if (std::is_constant_evaluated())
		{
			for (int i = 0; i * sizeof(result) < 16; ++i) {
				auto val = static_cast<result>(rand());
				for (size_t j = 0; j < sizeof(result); ++j) {
					ret._bytes[i * sizeof(result) + j] = static_cast<std::byte>(val >> ((sizeof(result) - 1 - j) * 8));
				}
			}
		}
		else
		{
			for (size_t i = 0; i * sizeof(result) < 16; ++i) {
				auto val = rand();
				std::memcpy(&ret._bytes[0] + i * sizeof(result), &val, sizeof(result));
			}
		}
		ret._bytes[6] = static_cast<std::byte>((static_cast<unsigned char>(ret._bytes[6]) & 0b00001111) | (4 << 4));
		ret._bytes[8] = static_cast<std::byte>((static_cast<unsigned char>(ret._bytes[8]) & 0b00111111) | 0b10000000);
		return ret;
	}

	template <typename Random = std::mt19937_64, typename Duration = std::chrono::milliseconds>
	requires (std::invocable<Random> && std::integral<std::invoke_result_t<Random>>)
	static constexpr uuid generate_v7(Random&& rand, std::chrono::time_point<std::chrono::system_clock, Duration> timestamp) noexcept (std::is_nothrow_invocable_v<Random>)
	{
		using result = std::invoke_result_t<Random>;
		uuid ret;
		
		auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch());
		static_assert(sizeof(dur.count()) >= 6);
		if (std::is_constant_evaluated())
		{
			for (int i = 0; i * sizeof(result) < 16; ++i) {
				auto val = static_cast<result>(rand());
				for (size_t j = 0; j < sizeof(result); ++j) {
					ret._bytes[i * sizeof(result) + j] = static_cast<std::byte>(val >> ((sizeof(result) - 1 - j) * 8));
				}
			}
			for (size_t i = 0; i < 6; ++i) {
				ret._bytes[i] = static_cast<std::byte>(dur.count() >> (6 - 1 - i) * 8);
			}
		}
		else
		{
			auto count = (std::endian::native == std::endian::big ? dur.count() : std::byteswap(dur.count()));
			std::memcpy(&ret._bytes[0], reinterpret_cast<std::byte*>(&count) + sizeof(count) - 6, 6);
			auto val = rand();
			std::memcpy(&ret._bytes[0] + 6, &val, 2);
			for (size_t i = 0; i * sizeof(result) < 8; ++i) {
				val = rand();
				std::memcpy(&ret._bytes[0] + 8 + i * sizeof(result), &val, sizeof(result));
			}
		}
		ret._bytes[6] = static_cast<std::byte>((static_cast<unsigned char>(ret._bytes[6]) & 0b00001111) | (7 << 4));
		ret._bytes[8] = static_cast<std::byte>((static_cast<unsigned char>(ret._bytes[8]) & 0b00111111) | 0b10000000);
		return ret;
	}

	static constexpr uuid generate_v4() noexcept
	{
		if (std::is_constant_evaluated()) {
			return generate_v4([]() constexpr -> uint64_t { return 0x7844784478447844; });
		} else {
			thread_local std::mt19937_64 engine{std::chrono::high_resolution_clock::now().time_since_epoch().count() ^ std::hash<std::thread::id>{}(std::this_thread::get_id())};

			return generate_v4(engine);
		}
	}

	static constexpr uuid generate_v7() noexcept
	{
		if (std::is_constant_evaluated()) {
			return generate_v7([]() constexpr -> uint64_t { return 0x7844784478447844; }, {});
		} else {
			thread_local std::mt19937_64 engine{std::chrono::high_resolution_clock::now().time_since_epoch().count() ^ std::hash<std::thread::id>{}(std::this_thread::get_id())};

			return generate_v7(engine, std::chrono::system_clock::now());
		}
	}

	constexpr int ver() const noexcept
	{
		return (static_cast<unsigned char>(_bytes[6]) & 0xF0) >> 4;
	}

	constexpr bool is_null() const noexcept
	{
		if (std::is_constant_evaluated()) {
			return std::ranges::all_of(_bytes, [](std::byte b) noexcept { return b == std::byte{}; });
		} else {
			constexpr std::byte zero[16] = {};
			return std::memcmp(_bytes, zero, sizeof(zero)) == 0;
		}
	}

	constexpr friend bool operator==(const uuid& lhs, const uuid& rhs) noexcept = default;

	constexpr explicit operator std::string() const
	{
		std::string ret;

		ret.resize_and_overwrite(32, [h = high(), l = low()](char* buf, size_t) noexcept {
			auto _ = std::to_chars(buf, buf + 16, h, 16);
			_ = std::to_chars(buf + 16, buf + 32, l, 16);
			return 32;
		});
		return ret;
	}

	constexpr std::string to_hex() const {
		std::string ret;

		ret.resize_and_overwrite(32, [this](char *buf, size_t) noexcept {
			shion::to_hex(buf, _bytes, 16);
			return 32;
		});
		return ret;
	}

	constexpr std::string to_string() const {
		std::string ret;

		ret.resize_and_overwrite(36, [this](char *buf, size_t) noexcept {
			to_chars(buf, buf + 36, *this, true);
			return 36;
		});
		return ret;
	}

	friend constexpr std::to_chars_result to_chars(char* first, char* end, const uuid& id, bool with_dashes) noexcept
	{
		if (with_dashes)
		{
			if (auto diff = end - first; static_cast<ptrdiff_t>(diff) < 36)
			{
				return {first + diff, std::errc::result_out_of_range};
			}
			first = shion::to_hex(first, id._bytes, 4);
			*first = '-';
			++first;
			first = shion::to_hex(first, id._bytes + 4, 2);
			*first = '-';
			++first;
			first = shion::to_hex(first, id._bytes + 6, 2);
			*first = '-';
			++first;
			first = shion::to_hex(first, id._bytes + 8, 2);
			*first = '-';
			++first;
			first = shion::to_hex(first, id._bytes + 10, 6);

			return {first, std::errc{}};
		}
		else
		{
			if (auto diff = end - first; static_cast<ptrdiff_t>(diff) < 32)
			{
				return {first + diff, std::errc::result_out_of_range};
			}

			return {shion::to_hex(first, id._bytes, 16), std::errc{}};
		}
	}

	std::uint64_t high() const noexcept
	{
		return std::launder(reinterpret_cast<std::uint64_t const(*)>(&_bytes[0]))[0];
	}

	std::uint64_t low() const noexcept
	{
		return std::launder(reinterpret_cast<std::uint64_t const(*)>(&_bytes[0]))[1];
	}

	constexpr auto begin() const noexcept
	{
		return &_bytes[0];
	}

	constexpr auto end() const noexcept
	{
		return &_bytes[0] + 16;
	}

	constexpr const std::byte* data() const noexcept
	{
		return _bytes;
	}

	constexpr size_t size() const noexcept
	{
		return 16;
	}

	constexpr std::byte operator[](size_t n) const noexcept
	{
		return _bytes[n];
	}

	constexpr operator std::span<const std::byte, 16>() const noexcept
	{
		return _bytes;
	}

	friend constexpr auto operator<=>(const uuid& a, const uuid& b) noexcept = default;

private:
	alignas(std::uint64_t) std::byte _bytes[16]{};
};

constexpr inline uuid uuid::nil = uuid::nil_uuid();
constexpr inline uuid uuid::max = uuid::max_uuid();

}

#endif /* SHION_UTILITY_UUID_H_ */