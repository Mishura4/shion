#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#include <shion/common/assert.hpp>

#include <cstdlib>
#include <filesystem>

#if _WIN32
#include <string>
#include <Windows.h>
#else
#include <iostream>
#include <syncstream>
#endif

#endif

namespace SHION_NAMESPACE
{

namespace
{

#ifdef _WIN32
auto towstring(std::string_view in) -> std::wstring
{
	std::wstring str;

	size_t sz{};
	mbstowcs_s(&sz, nullptr, 0, in.data(), in.size());
	str.resize_and_overwrite(sz, [&](wchar_t* buf, std::size_t len) noexcept {
		mbstowcs_s(nullptr, buf, len, in.data(), in.size());
		return sz;
	});
	return str;
}
#endif

}

[[noreturn]] SHION_API void default_assert_handler(
	std::string_view msg,
	std::source_location where
)
{
	auto report = [&](auto path) {
#ifdef _WIN32
		if constexpr (std::convertible_to<decltype(path), const wchar_t*>)
		{
			_wassert(towstring(msg).c_str(), path, where.line());
		}
		else
		{
			_wassert(towstring(msg).c_str(), towstring(path).c_str(), where.line());
		}
#else
	//std::osyncstream oss(std::cerr);
	std::cerr << "Assertion failed: " << msg << "\n"
		<< "\tFile " << path << ":" << where.line() << std::endl;
#endif
	};

	std::string_view filename = where.file_name();
#if defined(SHION_BUILD_ROOT)
	auto root = std::filesystem::path(SHION_BUILD_ROOT);
	auto failure = std::filesystem::path(filename);
	if (auto [failIt, rootIt] = std::ranges::mismatch(failure, root); rootIt == root.end())
	{
		std::filesystem::path relative;
		while (failIt != failure.end())
		{
			relative /= *failIt;
			++failIt;
		}
		report(relative.native().c_str());
	}
#endif
	report(where.file_name());
	std::abort();
};

std::atomic<assert_handler> g_assert_handler = &default_assert_handler;

void assert_failure_handler::operator()(
	std::string_view condition,
	std::string_view msg
)
{
	(*g_assert_handler)(std::format("Assertion '{}' failed: {}", condition, msg), where);
}

void assert_failure_handler::operator()(
	std::string_view condition
)
{
	(*g_assert_handler)(std::format("Assertion '{}' failed", condition), where);
}

void assert_failure_handler::impl(
	std::string_view condition,
	std::string_view fmt,
	std::format_args args
)
{
	std::string val = std::string("Assertion ");
	val += '\'';
	val += condition;
	val += "\' failed: ";
	val += std::vformat(fmt, args);
	(*g_assert_handler)(val, where);
}

}
