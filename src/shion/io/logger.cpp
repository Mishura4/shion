#include <shion/io/logger.hpp>
#include <shion/tools.hpp>
#include <shion/exception.hpp>

namespace shion {

logger::logger(std::filesystem::path file_path, log_mask types_enabled) :
	_path{std::move(file_path)},
	_out{std::in_place_type<file_stream>, _path, std::ios::out | std::ios::app},
	_log_mask(types_enabled) {
	if (!std::get<file_stream>(_out).good()) {
		throw shion::exception{std::format("could not open {} for writing", _path.string())};
	}
}

logger::logger(std::ostream& stream, log_mask types_enabled) :
	_path{},
	_out{std::in_place_type<std::ostream*>, &stream},
	_log_mask(types_enabled) {
}

logger::~logger() {
	if (std::holds_alternative<file_stream>(_out))
		std::get<file_stream>(_out) << '\n';
}


bool logger::enabled(log_mask type) const noexcept {
	return _log_mask & type;
}


void logger::log(log_mask type, std::string_view msg) {
	if (!enabled(type)) {
		return;
	}

	std::string type_str;
	std::string_view type_sv{};
	if (!_log_type_formatter.has_value()) {
		using enum log_type;

		if (type & trace) {
			type_sv = "[TRACE]: "sv;
		} else if (type & debug) {
			type_sv = "[DEBUG]: "sv;
		} else if (type & error) {
			type_sv = "[ERROR]: "sv;
		} else if (type & warning) {
			type_sv = "[WARNING]: "sv;
		} else if (type & info) {
			type_sv = "[INFO]: "sv;
		}
	} else if (*_log_type_formatter) {
		type_str = _log_type_formatter->operator()(type);
		type_sv = type_str;
	}

	std::string time_str;
	if (!_time_formatter.has_value()) {
		time_str = std::format("[{:%T}] ", wall_clock::now());
	} else if (*_time_formatter) {
		time_str = _time_formatter->operator()(wall_clock::now());
	}

	_log(msg, type_sv, time_str);
}

void logger::set_log_type_formatter(std::function<std::string(log_mask)> fun) {
	_log_type_formatter.emplace(std::move(fun));
}

void logger::set_log_type_formatter(std::nullptr_t) noexcept {
	_log_type_formatter.emplace(nullptr);
}

void logger::set_time_formatter(std::function<std::string(time)> fun) {
	_time_formatter.emplace(std::move(fun));
}

void logger::set_time_formatter(std::nullptr_t) noexcept {
	_time_formatter.emplace(nullptr);
}

void logger::_log(std::string_view msg, std::string_view type_str, std::string_view time) {
	if (std::holds_alternative<std::vector<logger*>>(_out)) {
		for (logger *l : std::get<std::vector<logger*>>(_out)) {
			l->_log(msg, type_str, time);
		}
		return;
	}

	std::ostream *out;
	if (std::holds_alternative<std::ostream*>(_out))
		out = std::get<std::ostream*>(_out);
	else if (std::holds_alternative<file_stream>(_out))
		out = &std::get<file_stream>(_out);
	else
		return;

	*out << time << type_str << msg << '\n';
}

}

