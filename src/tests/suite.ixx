module;

#include <filesystem>
#include <source_location>
#include <string>
#include <utility>
#include <type_traits>
#include <functional>
#include <optional>
#include <vector>
#include <chrono>

export module shion.tests:suite;

import shion;

export namespace shion::tests {

inline const auto failure_str = "[\u001b[31mFAILURE\u001b[0m]";
inline const auto timeout_str = "[\u001b[31mTIMEOUT\u001b[0m]";
inline const auto not_executed_str = "[\u001b[31mNOT EXECUTED\u001b[0m]";
inline const auto starting_str = "[\u001b[94mSTARTING\u001b[0m]";
inline const auto skipped_str = "[\u001b[33mSKIPPED\u001b[0m]";
inline const auto success_str = "[\u001b[32mSUCCESS\u001b[0m]";

class test {
public:
	enum class status {
		not_executed,
		skipped,
		started,
		success,
		failure,
		timeout
	};

	test() = default;

	template <typename Fun>
	test(std::string n, Fun&& function) :
		name{std::move(n)}, fun{std::forward<Fun>(function)}
	{}

	test(const test &) = delete;
	test(test&&) = default;

	test& operator=(const test&) = delete;
	test& operator=(test&&) = default;
	~test() = default;

	void fail(std::string reason = {}, const std::source_location &where = std::source_location::current());
	void success();
	void skip();
	void run();

	status get_status() const noexcept;
	auto get_duration() const noexcept -> std::chrono::duration<double, std::milli>;
	const std::string& get_name() const noexcept;

private:
	app_time start_time;
	app_time end_time;
	std::string name = {};
	std::function<void(test&)> fun = {};
	status state = status::not_executed;
};

struct test_suite {
	std::string name;
	std::vector<test> tests;

	template <typename Fun>
	requires (std::is_invocable_r_v<void, Fun, test&>)
	test &make_test(std::string n, Fun fun) {
		return (tests.emplace_back(std::move(n), std::forward<Fun>(fun)));
	}
};

std::vector<test_suite> init();

inline std::optional<logger> g_logger;

test_suite &make_test_suite(std::string name);

struct test_failure_exception : shion::internal_exception {
	using internal_exception::internal_exception;
};

} // namespace shion::tests
