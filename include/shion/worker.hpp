#ifndef SHION_TOOLS_WORKER_H_
#define SHION_TOOLS_WORKER_H_

#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <memory>
#include <utility>

#include "coro/awaitable.hpp"

namespace shion {

class worker {
public:
	worker() = default;
	worker(const worker &) = delete;
	worker(worker&&) = delete;
	~worker();

	worker& operator=(const worker&) = delete;
	worker& operator=(worker&&) = delete;

	template <typename Fun>
	requires (std::invocable<Fun>)
	[[nodiscard]] awaitable<std::invoke_result_t<Fun>> schedule(Fun&& work) {
		using ret = std::invoke_result_t<Fun>;
		promise<ret> promise{};
		awaitable<ret> awaitable = promise.get_awaitable();
		std::unique_lock lock{mutex};

		work_queue.emplace_back([fun = std::forward<Fun>(work), p = std::move(promise)]() mutable noexcept {
			try {
				p.set_value(std::invoke(std::forward<Fun>(fun)));
			} catch (...) {
				p.set_exception(std::current_exception());
			}
		});
		cv.notify_all();
		return awaitable;
	}

	template <typename Fun>
	requires (std::invocable<Fun>)
	void queue(Fun&& work) {
		static_assert(std::is_nothrow_invocable_v<Fun>);

		std::unique_lock lock{mutex};

		work_queue.emplace_back([fun = std::forward<Fun>(work)]() mutable noexcept {
			std::invoke(std::forward<Fun>(fun));
		});
		cv.notify_all();
	}

	awaitable<void> stop();

private:
	void _run();

  using work = std::function<void()>;
  std::mutex mutex;
	promise<void> end_promise;
  std::condition_variable cv;
  std::vector<work> work_queue;
	std::atomic<bool> running = false;
	std::thread thread{&worker::_run, this};
};

}

#endif /* SHION_TOOLS_WORKER_H_ */
