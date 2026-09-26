#include "include/kvstore/kvstore.hpp"

KeyValueStore::KeyValueStore(std::chrono::milliseconds cleanup_interval)
{
	reaper_thread_ = std::thread(&KeyValueStore::run_cleaner, this, cleanup_interval);
}

KeyValueStore::~KeyValueStore() {
	stop_requested_ = true;
	cv_stop_.notify_all();
	if (reaper_thread_.joinable()) {
		reaper_thread_.join();
	}
}

void KeyValueStore::run_cleaner(std::chrono::milliseconds interval) {
	while (!stop_requested_.load()) {
		std::unique_lock<std::mutex> lock(cv_mutex_);

		cv_stop_.wait_for(lock, interval, [this]
		{ return stop_requested_.load(); });
		if (stop_requested_.load()) {
			break;
		}

		lock.unlock();

		std::shared_lock<std::shared_mutex> store_lock(mutex_);
		std::vector<std::string> keys_to_delete;
		for (const auto& item : store_) {
			if (item.second.is_expired()) {
				keys_to_delete.push_back(item.first);
			}
		}
		store_lock.unlock();

		for (const auto& key : keys_to_delete) {
			if (store_[key].is_expired()) {
				del(key);
			}
		}
	}
}