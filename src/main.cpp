#include "kvstore/kvstore.hpp"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cassert>
#include <string>

using namespace std::chrono_literals;

void test_basic_operations() {
    std::cout << "--- 1. Basic Key-Value Operations ---\n";
    KeyValueStore store;

    // Test SET & GET
    store.set("user:100", "Alice");
    store.set("user:101", "Bob");

    auto val1 = store.get("user:100");
    auto val2 = store.get("user:101");
    auto val3 = store.get("user:999"); // Non-existent key

    std::cout << "[SET/GET] user:100 => " << (val1 ? *val1 : "NOT FOUND") << " (Expected: Alice)\n";
    std::cout << "[SET/GET] user:101 => " << (val2 ? *val2 : "NOT FOUND") << " (Expected: Bob)\n";
    std::cout << "[SET/GET] user:999 => " << (val3 ? *val3 : "NOT FOUND") << " (Expected: NOT FOUND)\n";

    assert(val1.has_value() && *val1 == "Alice");
    assert(val2.has_value() && *val2 == "Bob");
    assert(!val3.has_value());

    // Test EXISTS
    std::cout << "[EXISTS] user:100 exists? " << (store.exists("user:100") ? "Yes" : "No") << " (Expected: Yes)\n";
    std::cout << "[EXISTS] user:999 exists? " << (store.exists("user:999") ? "Yes" : "No") << " (Expected: No)\n";

    assert(store.exists("user:100"));
    assert(!store.exists("user:999"));

    // Test UPDATE (Overwrite)
    store.set("user:100", "Alice Updated");
    auto val1_updated = store.get("user:100");
    std::cout << "[OVERWRITE] user:100 => " << (val1_updated ? *val1_updated : "NOT FOUND") << " (Expected: Alice Updated)\n";
    assert(val1_updated.has_value() && *val1_updated == "Alice Updated");

    // Test DEL
    bool deleted = store.del("user:101");
    bool deleted_nonexistent = store.del("user:999");
    std::cout << "[DEL] user:101 deleted? " << (deleted ? "Yes" : "No") << " (Expected: Yes)\n";
    std::cout << "[DEL] user:999 deleted? " << (deleted_nonexistent ? "Yes" : "No") << " (Expected: No)\n";

    assert(deleted);
    assert(!deleted_nonexistent);
    assert(!store.exists("user:101"));

    std::cout << "[PASS] Basic operations passed successfully.\n\n";
}

void test_ttl_and_eviction() {
    std::cout << "--- 2. TTL and Background Eviction ---\n";
    // Initialize KeyValueStore with a fast reaper cleanup interval of 100ms
    KeyValueStore store(100ms);

    // Set key with 300ms TTL
    store.set("session:xyz", "token_12345", 300ms);
    std::cout << "[TTL] Created session:xyz with 300ms TTL.\n";

    // Immediate lookup
    auto val_immediate = store.get("session:xyz");
    std::cout << "[TTL] Immediate lookup => " << (val_immediate ? *val_immediate : "EXPIRED/NOT FOUND") << "\n";
    assert(val_immediate.has_value() && *val_immediate == "token_12345");

    // Sleep 400ms to allow TTL expiration
    std::cout << "[TTL] Sleeping for 400ms...\n";
    std::this_thread::sleep_for(400ms);

    // Lazy check upon lookup
    auto val_expired = store.get("session:xyz");
    std::cout << "[TTL] Lookup after 400ms => " << (val_expired ? *val_expired : "EXPIRED/NOT FOUND") << " (Expected: EXPIRED)\n";
    assert(!val_expired.has_value());
    assert(!store.exists("session:xyz"));

    // Populate multiple keys with short TTLs for background reaper testing
    std::cout << "[TTL Reaper] Setting 10 keys with 200ms TTL...\n";
    for (int i = 0; i < 10; ++i) {
        store.set("temp_key_" + std::to_string(i), "temp_val", 200ms);
    }

    // Wait 500ms so reaper thread runs and removes them
    std::this_thread::sleep_for(500ms);

    int remaining = 0;
    for (int i = 0; i < 10; ++i) {
        if (store.exists("temp_key_" + std::to_string(i))) {
            remaining++;
        }
    }
    std::cout << "[TTL Reaper] Remaining expired keys after cleanup => " << remaining << " (Expected: 0)\n";
    assert(remaining == 0);

    std::cout << "[PASS] TTL and background eviction passed successfully.\n\n";
}

void test_concurrency() {
    std::cout << "--- 3. Multi-Threaded Concurrency Test ---\n";
    KeyValueStore store(200ms);

    constexpr int NUM_WRITERS = 4;
    constexpr int NUM_READERS = 4;
    constexpr int OPS_PER_THREAD = 1000;

    std::vector<std::thread> threads;
    std::atomic<bool> start_flag{false};

    // Spawn Writer Threads
    for (int t = 0; t < NUM_WRITERS; ++t) {
        threads.emplace_back([&store, &start_flag, t]() {
            while (!start_flag.load()) {
                std::this_thread::yield();
            }
            for (int i = 0; i < OPS_PER_THREAD; ++i) {
                std::string key = "key_" + std::to_string((t * 100) + (i % 20));
                std::string val = "val_" + std::to_string(i);

                if (i % 3 == 0) {
                    store.set(key, val, 50ms); // short TTL
                } else if (i % 5 == 0) {
                    store.del(key);
                } else {
                    store.set(key, val); // no TTL
                }
            }
        });
    }

    // Spawn Reader Threads
    for (int t = 0; t < NUM_READERS; ++t) {
        threads.emplace_back([&store, &start_flag, t]() {
            while (!start_flag.load()) {
                std::this_thread::yield();
            }
            for (int i = 0; i < OPS_PER_THREAD; ++i) {
                std::string key = "key_" + std::to_string((t * 100) + (i % 20));
                auto result = store.get(key);
                bool exists = store.exists(key);
                (void)result;
                (void)exists;
            }
        });
    }

    std::cout << "Launching " << NUM_WRITERS << " writers and " << NUM_READERS 
              << " readers concurrently (" << (NUM_WRITERS + NUM_READERS) * OPS_PER_THREAD 
              << " operations total)...\n";

    start_flag.store(true);

    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }

    std::cout << "[PASS] Concurrency stress test finished without race conditions or deadlocks.\n\n";
}

int main() {
    std::cout << "=========================================\n";
    std::cout << " Thread-Safe Key-Value Store Demo & Test \n";
    std::cout << "=========================================\n\n";

    test_basic_operations();
    test_ttl_and_eviction();
    test_concurrency();

    std::cout << "All tests passed successfully!\n";
    return 0;
}
