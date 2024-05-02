#ifndef shion_COMMON_LOGGER_H_
#define shion_COMMON_LOGGER_H_

#include <variant>
#include <filesystem>
#include <fstream>
#include <functional>
#include <mutex>
#include <memory>
#include <utility>
#include <format>

#include "../utility/bit_mask.h"
#include "../shion_essentials.hpp"

namespace shion {

enum class log_type : uint64 {
	none    = 0,
	error   = 1,
	warning = 1 << 1,
	info    = 1 << 2,
	debug   = 1 << 3,
	trace   = 1 << 4,
	all     = ~0_u64
};

using log_mask = shion::bit_mask<log_type>;

class SHION_API logger {
public:
	static constexpr auto default_enabled_logs = log_mask{log_type::info, log_type::debug, log_type::warning};

	using time = std::chrono::time_point<std::chrono::system_clock>;

	constexpr logger() = default;
	explicit logger(std::filesystem::path file_path, log_mask types_enabled = default_enabled_logs);
	explicit logger(std::ostream& stream, log_mask types_enabled = default_enabled_logs);

	template <typename T, typename... Ts>
	requires (std::convertible_to<logger&, T> && (std::convertible_to<logger&, Ts> && ...))
	explicit logger(T&& sub_logger, Ts&&... sub_loggers) :
		_out{std::in_place_type<std::vector<logger*>>, std::initializer_list<logger*>{std::addressof(sub_logger), std::addressof(sub_loggers)...}} {
		for (logger *l : std::get<std::vector<logger*>>(_out)) {
			_log_mask |= l->_log_mask;
		}
	}

	~logger();

	template <typename T, typename... Args>
	void log(log_mask type, std::format_string<T, Args...> fmt, T&& arg1, Args&&... args) {
		log(type, std::format(fmt, std::forward<T>(arg1), std::forward<Args>(args)...));
	}

	void log(log_mask type, std::string_view msg);

	template <typename Fun, typename... Args>
	requires (std::is_invocable_r_v<std::string_view, Fun, Args...>)
	void log(log_mask type, Fun&& fun, Args&&... args) {
		if (!enabled(type))
			return;

		this->log(type, std::invoke(std::forward<Fun>(fun), std::forward<Args>(args)...));
	}

	template <typename T, typename... Args>
	void info(std::format_string<T, Args...> fmt, T&& arg1, Args&&... args) {
		this->log(log_type::info, std::format(fmt, std::forward<T>(arg1), std::forward<Args>(args)...));
	}

	void info(std::string_view msg) {
		this->log(log_type::info, msg);
	}

	template <typename Fun, typename... Args>
	requires (std::is_invocable_r_v<std::string_view, Fun, Args...>)
	void info(Fun&& fun, Args&&... args) {
		this->log(log_type::info, std::forward<Fun>(fun), std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	void warn(std::format_string<T, Args...> fmt, T&& arg1, Args&&... args) {
		this->log(log_type::warning, std::format(fmt, std::forward<T>(arg1), std::forward<Args>(args)...));
	}

	void warn(std::string_view msg) {
		this->log(log_type::warning, msg);
	}

	template <typename Fun, typename... Args>
	requires (std::is_invocable_r_v<std::string_view, Fun, Args...>)
	void warn(Fun&& fun, Args&&... args) {
		this->log(log_type::warning, std::forward<Fun>(fun), std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	void error(std::format_string<T, Args...> fmt, T&& arg1, Args&&... args) {
		this->log(log_type::error, std::format(fmt, std::forward<T>(arg1), std::forward<Args>(args)...));
	}

	void error(std::string_view msg) {
		this->log(log_type::error, msg);
	}

	template <typename Fun, typename... Args>
	requires (std::is_invocable_r_v<std::string_view, Fun, Args...>)
	void error(Fun&& fun, Args&&... args) {
		this->log(log_type::error, std::forward<Fun>(fun), std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	void debug(std::format_string<T, Args...> fmt, T&& arg1, Args&&... args) {
		this->log(log_type::debug, std::format(fmt, std::forward<T>(arg1), std::forward<Args>(args)...));
	}

	void debug(std::string_view msg) {
		this->log(log_type::debug, msg);
	}

	template <typename Fun, typename... Args>
	requires (std::is_invocable_r_v<std::string_view, Fun, Args...>)
	void debug(Fun&& fun, Args&&... args) {
		this->log(log_type::debug, std::forward<Fun>(fun), std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	void trace(std::format_string<T, Args...> fmt, T&& arg1, Args&&... args) {
		this->log(log_type::trace, std::format(fmt, std::forward<T>(arg1), std::forward<Args>(args)...));
	}

	void trace(std::string_view msg) {
		this->log(log_type::trace, msg);
	}

	template <typename Fun, typename... Args>
	requires (std::is_invocable_r_v<std::string_view, Fun, Args...>)
	void trace(Fun&& fun, Args&&... args) {
		this->log(log_type::trace, std::forward<Fun>(fun), std::forward<Args>(args)...);
	}

	[[nodiscard]] bool enabled(log_mask type) const noexcept;

	void set_log_type_formatter(std::function<std::string(log_mask)> fun);

	void set_log_type_formatter(std::nullptr_t) noexcept;

	void set_time_formatter(std::function<std::string(time)> fun);

	void set_time_formatter(std::nullptr_t) noexcept;

private:
	void _log(std::string_view msg, std::string_view type_str, std::string_view time);

	template <typename T>
	using formatter = std::optional<std::function<T>>;

	using file_stream = std::ofstream;
	using output = std::variant<file_stream, std::ostream*, std::vector<logger*>>;

	std::filesystem::path            _path{};
	output                           _out{};
	log_mask                         _log_mask = default_enabled_logs;
	formatter<std::string(log_mask)> _log_type_formatter{std::nullopt};
	formatter<std::string(time)>     _time_formatter{std::nullopt};
};

}

#endif /* shion_COMMON_LOGGER_H_ */
