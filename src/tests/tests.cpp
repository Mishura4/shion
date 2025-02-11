#include "tests.hpp"
#include "containers.hpp"

#include <shion/utility/named_parameter.hpp>
#include <shion/parexp/expr.hpp>

namespace shion::tests {
	
constexpr shion::string_literal foo = "foo";
constexpr shion::string_literal bar = "bar";

constexpr auto foobar = foo + "bar";
constexpr auto foo_bar = foo + ' ' + "bar";

std::vector<test_suite> init() {
	std::vector<test_suite> ret;
	
	auto& containers = ret.emplace_back("Containers");
	containers.make_test("hive with trivial type", &hive_test_trivial);
	containers.make_test("hive with non-trivial type", &hive_test_nontrivial);

	return ret;
}

void test::fail(std::string reason, const std::source_location &where) {
	std::filesystem::path filepath{where.file_name()};

	if (reason.empty()) {
		g_logger->info("{} {} ({}/{}:{})", failure_str, name, filepath.parent_path().stem().string(), filepath.filename().string(), where.line());
	}
	else {
		g_logger->info("{} {} ({}/{}:{}): {}", failure_str, name, filepath.parent_path().stem().string(), filepath.filename().string(), where.line(), reason);
	}
	state = status::failure;
}

void test::skip(){
	if (state == status::not_executed || state == status::started) {
		state = status::skipped;
		g_logger->info("{} {}", skipped_str, name);
	}
}

void test::success() {
	if (state != status::failure) {
		state = status::success;
		end_time = app_clock::now();
		g_logger->info("{} {} ({:%Q%q})", success_str, name, get_duration());
	}
}

auto test::get_duration() const noexcept -> std::chrono::duration<double, std::milli> {
	return std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(end_time - start_time);
}

void test::run() {
	if (state == status::skipped)
		return;
		g_logger->info("{} {}", starting_str, name);
	state = status::started;
	start_time = app_clock::now();
	try {
		fun(*this);
		success();
	} catch (const test_failure_exception &e) {
		end_time = app_clock::now();
		g_logger->info("{} {}: {}", failure_str, name, e.format());
	} catch (const std::exception &e) {
		end_time = app_clock::now();
		g_logger->info("{} {}: exception `{}`", failure_str, name, e.what());
	} catch (...) {
		end_time = app_clock::now();
		g_logger->info("{} {}: exception (unknown)", failure_str, name);
	}
}

auto test::get_name() const noexcept -> const std::string& {
	return (name);
}

auto test::get_status() const noexcept -> status {
	return (state);
}

}