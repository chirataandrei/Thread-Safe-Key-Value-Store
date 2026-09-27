#pragma once

#include <string>
#include <optional>
#include <chrono>

struct StoreValue {
	std::string value;
	std::optional<std::chrono::steady_clock::time_point> expire_at;

	bool is_expired() const {
		return expire_at.has_value() && std::chrono::steady_clock::now() >= expire_at.value();
	}
};