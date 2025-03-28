#pragma once

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#  include <source_location>
#  include <format>
#  include <exception>
#  include <string>

#  include <shion/common/types.hpp>
#endif

namespace shion {

/**
 * @brief Base class for all exceptions thrown by the library.
 */
SHION_EXPORT class exception : public std::exception {
public:
	exception() = default;
	exception(const exception &) = default;
	exception(exception&&) = default;
	SHION_API exception(const std::string &msg);
	SHION_API exception(std::string &&msg) noexcept;

	exception& operator=(const exception&) = default;
	exception& operator=(exception&&) = default;

	~exception() override = default;

	template <typename T, typename... Args>
	exception(std::format_string<T, Args...> fmt, T&& arg1, Args&&... args) :
		exception(std::format(fmt, std::forward<T>(arg1), std::forward<Args>(args)...))
	{
	}

	SHION_API auto what() const noexcept -> const char* override;

private:
	std::string _message;
};

SHION_EXPORT class internal_exception : public exception {
public:
	using exception::operator=;

	SHION_API internal_exception(std::source_location loc = std::source_location::current());
	SHION_API internal_exception(const std::string &msg, std::source_location loc = std::source_location::current());
	SHION_API internal_exception(std::string &&msg, std::source_location loc = std::source_location::current());

	SHION_API auto where() const noexcept -> const std::source_location&;
	SHION_API auto format() const -> std::string;

private:
	std::source_location _where;
};

/**
 * @brief Base class for internal exceptions, that is, exceptions that are programmer errors,
 * for example a bad object state for a library operation.
 */
SHION_EXPORT class logic_exception : public exception {
public:
	using exception::exception;
	using exception::operator=;
};

/**
 * @brief A task was cancelled
 */
SHION_EXPORT class task_cancelled_exception : public exception {
public:
	using exception::exception;
	using exception::operator=;
	
	SHION_API task_cancelled_exception();
};

/**
 * @brief In release, this returns a "default value". In debug, this calls std::unreachable.
 */
inline wildcard safe_unreachable(wildcard) {
	return wildcard{};
}

/**
 * @brief In release, this returns a "default value". In debug, this calls std::unreachable.
 */
template <typename T>
decltype(auto) safe_unreachable(T&& t [[maybe_unused]]) {
#ifndef NDEBUG
	unreachable();
#else
	if constexpr (!std::is_void_v<T>) {
		return static_cast<decltype(t)>(t);
	} else {
		return;
	}
#endif
}

}
