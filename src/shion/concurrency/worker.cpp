#include <shion/concurrency/worker.hpp>

namespace shion::concurrency {
auto concurrent_work_queue::pop() -> work {
	static detail::work_data dummy;
	detail::work_data*       expected;
	#ifndef NDEBUG
	int loops = 0;
	#endif

	while (true) {
		#ifndef NDEBUG
		assert((loops++) < (2 << 24) && "infinite spin");
		#endif
		// first: check that we even have a front
		expected = nullptr;
		if (_front.compare_exchange_strong(expected, &dummy, std::memory_order_acq_rel)) {
			// queue empty: restore null, return
			_front.store(nullptr, std::memory_order_release);
			return {};
		} else if (expected == &dummy) {
			// another thread is updating, spin
			continue;
		} else if (_front.compare_exchange_strong(expected, &dummy, std::memory_order_acq_rel)) {
			// expected is not null, and now we have it in expected, and we "locked" with dummy, unlock and return :)
			if (expected->next == nullptr) // last node, queue now empty
				_back.store(nullptr, std::memory_order_release);
			_front.store(expected->next, std::memory_order_release);
			return {expected};
		}
	}
}

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
