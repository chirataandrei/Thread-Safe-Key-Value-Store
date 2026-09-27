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

		std::unique_lock<std::shared_mutex> write_lock(mutex_);
		for (const auto& key : keys_to_delete) {
			auto it = store_.find(key);
			if (it != store_.end() && it->second.is_expired()) {
				store_.erase(it);
			}
		}
		write_lock.unlock();
	}
}

void KeyValueStore::set(const std::string& key, const std::string& value, std::optional<std::chrono::milliseconds> ttl)
{
	std::optional<std::chrono::steady_clock::time_point> expire_at = std::nullopt;
	
	if (ttl.has_value()) {
		expire_at = std::chrono::steady_clock::now() + *ttl;
	}

	std::unique_lock<std::shared_mutex> store_lock(mutex_);
	store_.insert_or_assign(key, StoreValue{value, expire_at});

	store_lock.unlock();
}

std::optional<std::string> KeyValueStore::get(const std::string& key) const {
	std::shared_lock<std::shared_mutex> get_lock(mutex_);

	auto it = store_.find(key);
	if (it == store_.end()) {
		get_lock.unlock();
		return std::nullopt;
	}

	if (it->second.expire_at != std::nullopt && it->second.is_expired() == true) {
		get_lock.unlock();
		return std::nullopt;
	}
	
	std::string value = it->second.value;
	get_lock.unlock();
	return value;
}