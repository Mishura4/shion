#pragma once

#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#  include <source_location>
#  include <format>
#  include <exception>
#  include <string>
#  include <utility>

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
	exception(std::format_string<std::type_identity_t<T>, std::type_identity_t<Args>...> fmt, T&& arg1, Args&&... args) :
		exception(std::vformat(fmt.get(), std::make_format_args(std::forward<T>(arg1), std::forward<Args>(args)...)))
	{
	}

	SHION_API auto what() const noexcept -> const char* override;

private:
	std::string _message;
};

SHION_EXPORT class internal_exception : public exception {
public:
	using exception::exception;

	SHION_API internal_exception(std::source_location loc = std::source_location::current());
	SHION_API internal_exception(const std::string &msg, std::source_location loc = std::source_location::current());
	SHION_API internal_exception(std::string &&msg, std::source_location loc = std::source_location::current());

	SHION_API auto where() const noexcept -> const std::source_location&;
	SHION_API auto format() const -> std::string;

private:
	std::source_location _where{};
};

/**
 * @brief Base class for internal exceptions, that is, exceptions that are programmer errors,
 * for example a bad object state for a library operation.
 */
SHION_EXPORT class logic_exception : public exception {
public:
	using exception::exception;
};

/**
 * @brief A task was cancelled
 */
SHION_EXPORT class task_cancelled_exception : public exception {
public:
	using exception::exception;
	
	SHION_API task_cancelled_exception();
};

}
