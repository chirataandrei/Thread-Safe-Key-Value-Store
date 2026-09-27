#include "kvstore/kvstore.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <thread>
#include <chrono>
#include <string>
#include <sstream>

using namespace std::chrono_literals;

// Simple lightweight test runner macros
#define TEST_CASE(name) void name(); static bool registered_##name = (test_cases.push_back({#name, name}), true); void name()
struct TestCase {
    std::string name;
    void (*func)();
};
static std::vector<TestCase> test_cases;

// Individual Test Cases

TEST_CASE(test_set_and_get_basic) {
    KeyValueStore store;
    store.set("key1", "value1");
    auto val = store.get("key1");
    assert(val.has_value());
    assert(*val == "value1");

    // Non-existent key
    auto empty_val = store.get("key_missing");
    assert(!empty_val.has_value());
}

TEST_CASE(test_overwrite_existing_key) {
    KeyValueStore store;
    store.set("key1", "initial_val");
    store.set("key1", "updated_val");
    auto val = store.get("key1");
    assert(val.has_value());
    assert(*val == "updated_val");
}

TEST_CASE(test_exists_functionality) {
    KeyValueStore store;
    assert(!store.exists("key1"));
    store.set("key1", "val1");
    assert(store.exists("key1"));
    store.del("key1");
    assert(!store.exists("key1"));
}

TEST_CASE(test_delete_functionality) {
    KeyValueStore store;
    store.set("key1", "val1");
    bool deleted_first = store.del("key1");
    assert(deleted_first == true);
    assert(!store.get("key1").has_value());

    bool deleted_again = store.del("key1");
    assert(deleted_again == false);
}

TEST_CASE(test_ttl_lazy_expiration) {
    KeyValueStore store;
    store.set("temp_key", "temp_val", 150ms);
    assert(store.exists("temp_key"));
    assert(store.get("temp_key").value() == "temp_val");

    std::this_thread::sleep_for(200ms);

    // After TTL, get() and exists() should return false/nullopt
    assert(!store.get("temp_key").has_value());
    assert(!store.exists("temp_key"));
}

TEST_CASE(test_ttl_overwrite_resets_ttl) {
    KeyValueStore store;
    // Set with short TTL
    store.set("key1", "val1", 100ms);
    // Overwrite with no TTL before it expires
    store.set("key1", "val1_permanent");

    std::this_thread::sleep_for(150ms);

    // Should still exist because overwrite removed TTL
    assert(store.exists("key1"));
    assert(store.get("key1").value() == "val1_permanent");
}

TEST_CASE(test_background_reaper_thread) {
    // 50ms cleanup interval
    KeyValueStore store(50ms);

    for (int i = 0; i < 20; ++i) {
        store.set("reaper_key_" + std::to_string(i), "reaper_val", 100ms);
    }

    // Wait 250ms for reaper to clean all keys
    std::this_thread::sleep_for(250ms);

    for (int i = 0; i < 20; ++i) {
        assert(!store.exists("reaper_key_" + std::to_string(i)));
    }
}

TEST_CASE(test_concurrent_read_write_stress) {
    KeyValueStore store(50ms);
    constexpr int NUM_THREADS = 8;
    constexpr int OPERATIONS_PER_THREAD = 1000;

    std::vector<std::thread> workers;
    for (int t = 0; t < NUM_THREADS; ++t) {
        workers.emplace_back([&store, t]() {
            for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                std::string k = "concurrent_key_" + std::to_string((t * 50) + (i % 50));
                std::string v = "val_" + std::to_string(i);

                if (i % 4 == 0) {
                    store.set(k, v, 20ms);
                } else if (i % 7 == 0) {
                    store.del(k);
                } else if (i % 2 == 0) {
                    store.set(k, v);
                } else {
                    store.get(k);
                    store.exists(k);
                }
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }
}

TEST_CASE(test_destruction_safety) {
    // Ensure KeyValueStore can be constructed and destroyed rapidly without deadlocks
    for (int i = 0; i < 10; ++i) {
        KeyValueStore temp_store(10ms);
        temp_store.set("k", "v", 5ms);
    }
}

int main() {
    std::cout << "Running KeyValueStore Unit Tests...\n";
    int passed = 0;
    int failed = 0;

    for (const auto& tc : test_cases) {
        std::cout << "  [TEST] " << tc.name << " ... ";
        try {
            tc.func();
            std::cout << "PASSED\n";
            passed++;
        } catch (const std::exception& e) {
            std::cout << "FAILED: " << e.what() << "\n";
            failed++;
        } catch (...) {
            std::cout << "FAILED (Unknown exception)\n";
            failed++;
        }
    }

    std::cout << "\nTest Summary: " << passed << " passed, " << failed << " failed.\n";
    return (failed == 0) ? 0 : 1;
}
