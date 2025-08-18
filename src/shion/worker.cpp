/*#include <shion/common/defines.hpp>

import shion;

import std;

namespace shion {

worker::~worker() {
	if (thread.joinable())
		thread.join();
}


void worker::_run() {
	std::unique_lock lock{mutex, std::defer_lock};

	running = true;
	while (true) {
		lock.lock();
		cv.wait(lock, [this]() { return !running.load(std::memory_order_acquire) || !work_queue.empty(); });
		if (!running.load(std::memory_order_relaxed)) {
			break;
		}
		std::vector<work> requests = std::move(work_queue);
		lock.unlock();
		for (work &fun : requests) {
			fun();
		}
	}
	end_promise.set_value();
}

shion::awaitable<void> worker::stop() {
	running.store(false, std::memory_order_release);
	cv.notify_all();
	return end_promise.get_awaitable();
}


}
*/