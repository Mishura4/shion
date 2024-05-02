#include "tests.hpp"

#include <iostream>
#include <thread>
#include <atomic>
#include <condition_variable>

#include <shion/math/util.hpp>

namespace shion::tests {

using clock = std::chrono::steady_clock;

using timepoint = std::chrono::time_point<clock>;

constexpr inline auto timeout_duration = std::chrono::seconds{60};

struct dthread : std::thread {
	using thread::thread;
	using thread::operator=;

	~dthread(){
		if (joinable())
			detach();
	}
};

}

namespace tests = shion::tests;

int main() {
	using tests::g_logger;
	g_logger.emplace(std::cout);
	g_logger->set_time_formatter(nullptr);
	g_logger->set_log_type_formatter(nullptr);
	tests::timepoint start = tests::clock::now();
	std::atomic max_time = (start + tests::timeout_duration).time_since_epoch().count();
	std::atomic finished = false;
	std::condition_variable cv;
	auto tests = tests::init();

	auto th = tests::dthread([&] {
		for (tests::test_suite &suite : tests) {
			for (tests::test &t : suite.tests) {
				if (finished) { // aborted
					return;
				}
				max_time = (tests::clock::now() + tests::timeout_duration).time_since_epoch().count();
				t.run();
			}
		}
		finished = true;
		cv.notify_all();
	});

	std::mutex m;
	std::unique_lock l{m};
	tests::timepoint timeout;

	do {
		timeout = tests::timepoint{tests::clock::duration{max_time.load()}};
		cv.wait_until(l, timeout, [&]() -> bool {
			return finished;
		});
	} while (!finished && tests::clock::now() < timeout);
	if (!finished) {
		finished = true;
		g_logger->info("Timing out suite");
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}

	size_t total_failed = 0;
	size_t total_count = 0;
	for (const tests::test_suite &suite : tests) {
		if (suite.tests.size() == 0)
			continue;
		size_t count = 0;
		size_t failed = 0;
		g_logger->info("\nSuite {}:", suite.name);
		for (const tests::test &t : suite.tests) {
			static constexpr char fmt_str[] = "  {: <20}{: <24}{} ({})";
			using enum tests::test::status;

			++count;
			switch (t.get_status()) {
				case not_executed:
					g_logger->info(fmt_str, tests::not_executed_str, t.get_name(), t.get_description(), t.get_duration());
					++failed;
					break;

				case skipped:
					g_logger->info(fmt_str, tests::skipped_str, t.get_name(), t.get_description(), t.get_duration());
					break;

				case started:
				case timeout:
					g_logger->info(fmt_str, tests::timeout_str, t.get_name(), t.get_description(), t.get_duration());
					++failed;
					break;

				case success:
					g_logger->info(fmt_str, tests::success_str, t.get_name(), t.get_description(), t.get_duration());
					break;

				case failure:
					g_logger->info(fmt_str, tests::failure_str, t.get_name(), t.get_description(), t.get_duration());
					++failed;
					break;
			}
		}
		g_logger->info("Completed {}: {:.2f}% success [{}/{}]\n", suite.name, shion::ratio{count - failed, count}.get_percent(), count - failed, count);
		total_failed += failed;
		total_count += count;
	}
	if (total_count == 0) {
		g_logger->info("No tests executed.");
	} else {
		g_logger->info(
			"Tests completed: {:.2f}% success [{}/{}]",
			shion::ratio{total_count - total_failed, total_count}.get_percent(),
			total_count - total_failed,
			total_count
		);
	}
	return (total_failed);
}
