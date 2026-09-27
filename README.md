# Thread-Safe Key-Value Store

A lightweight, thread-safe, in-memory key-value store implementation in C++20.

## Features

- **Thread-Safe**: Concurrent reads and exclusive writes using `std::shared_mutex`.
- **TTL Support**: Optional Time-To-Live duration per key.
- **Automatic Eviction**: Background reaper thread cleans up expired keys periodically.
- **Lazy Expiration**: Automatically returns `std::nullopt` for expired keys on access.

## Requirements

- C++20 compatible compiler (`clang++`, `g++`)
- CMake 3.16+

## Building & Running

### 1. Build the project
```bash
mkdir -p build && cd build
cmake ..
cmake --build .
```

### 2. Run the Demo
```bash
./kvstore_demo
```

### 3. Run Unit Tests
```bash
./kvstore_tests
# or using ctest
ctest --output-on-failure
```

## API Quick Reference

```cpp
#include "kvstore/kvstore.hpp"
#include <chrono>

using namespace std::chrono_literals;

// Initialize store with 1-second background cleanup interval
KeyValueStore store(1000ms);

// Set key-value pair (permanent)
store.set("key", "value");

// Set key with 500ms TTL
store.set("temp_key", "temp_value", 500ms);

// Get value
auto val = store.get("key"); // returns std::optional<std::string>

// Check existence
bool exists = store.exists("key"); // returns bool

// Delete key
bool deleted = store.del("key"); // returns bool
```
