#pragma once

#include <shion/defines.hpp>

#include <source_location>
#include <format>

#include "shion_essentials.hpp"

namespace shion {

/**
 * @brief Base class for all exceptions thrown by the library.
 */
SHION_EXPORT class exception : public std::exception {
public:
	exception() = default;
	exception(const exception &) = default;
	exception(exception&&) = default;
	SHION_SYMBOL exception(const std::string &msg);
	SHION_SYMBOL exception(std::string &&msg) noexcept;

	exception& operator=(const exception&) = default;
	exception& operator=(exception&&) = default;

	~exception() override = default;

	template <typename T, typename... Args>
	exception(std::format_string<T, Args...> fmt, T&& arg1, Args&&... args) :
		exception(std::format(fmt, std::forward<T>(arg1), std::forward<Args>(args)...))
	{
	}

	SHION_SYMBOL auto what() const noexcept -> const char* override;

private:
	std::string _message;
};

SHION_EXPORT class internal_exception : public exception {
public:
	using exception::operator=;

	SHION_SYMBOL internal_exception(std::source_location loc = std::source_location::current());
	SHION_SYMBOL internal_exception(const std::string &msg, std::source_location loc = std::source_location::current());
	SHION_SYMBOL internal_exception(std::string &&msg, std::source_location loc = std::source_location::current());

	SHION_SYMBOL auto where() const noexcept -> const std::source_location&;
	SHION_SYMBOL auto format() const -> std::string;

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

}
