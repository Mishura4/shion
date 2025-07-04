#include <shion/common/defines.hpp>

#if !SHION_BUILDING_MODULES
#	include <format>
#	include <functional>
#	include <source_location>

#	include <shion/common/exception.hpp>
#endif

namespace SHION_NAMESPACE
{

exception::exception(const std::string &msg) : _message{msg}
{}

exception::exception(std::string &&msg) noexcept : _message{std::move(msg)}
{}

const char *exception::what() const noexcept {
	return (_message.c_str());
}

internal_exception::internal_exception(std::source_location loc) : _where{std::move(loc)}
{}


internal_exception::internal_exception(const std::string &msg, std::source_location loc) :
	exception(msg),
	_where{std::move(loc)}
{}

internal_exception::internal_exception(std::string &&msg, std::source_location loc) :
	exception(std::move(msg)),
	_where{std::move(loc)}
{}

const std::source_location& internal_exception::where() const noexcept {
	return (_where);
}

std::string internal_exception::format() const {
	return (std::format(
		"in {}\n"
		"\tat {} line {}:\n"
		"\t{}",
		_where.file_name(),
		_where.function_name(),
		_where.line(),
		what()
	));
}

task_cancelled_exception::task_cancelled_exception() : exception("A task was cancelled.") {
}


}
