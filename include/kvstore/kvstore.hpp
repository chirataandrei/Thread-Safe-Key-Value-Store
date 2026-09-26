#pragma once

#include "entry.hpp"
#include <unordered_map>
#include <shared_mutex>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

class KeyValueStore {

	private:
		// background reaper loop method
		void run_cleaner(std::chrono::milliseconds interval);

		// store data
		std::unordered_map<std::string, StoreValue> store_;

		// enables concurrent reads and exclusive writes
		mutable std::shared_mutex mutex_;

		// TTL reaper
		std::thread reaper_thread_;
		std::atomic<bool> stop_requested_{false};
			
		// signaling primitives to suspend the reaper thread without busy-waiting
		std::condition_variable cv_stop_;
		std::mutex cv_mutex_;

	public:
		explicit KeyValueStore(std::chrono::milliseconds cleanup_interval = std::chrono::milliseconds(1000));
		~KeyValueStore();
		
		// prevent copying and moving
		KeyValueStore(const KeyValueStore&) = delete;
		KeyValueStore& operator=(const KeyValueStore&) = delete;
		KeyValueStore(KeyValueStore&&) = delete;
		KeyValueStore& operator=(KeyValueStore&&) = delete;

		// public API
		void set(const std::string& key, const std::string& value, std::optional<std::chrono::milliseconds> ttl = std::nullopt);
		std::optional<std::string> get(const std::string& key) const;
		bool del(const std::string& key);
		bool exists(const std::string& key) const;
};