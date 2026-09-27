cmake_minimum_required(VERSION 3.16)
project(ThreadSafeKeyValueStore CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

find_package(Threads REQUIRED)

# Main demo executable
add_executable(kvstore_demo
    src/main.cpp
    src/kvstore.cpp
)

target_include_directories(kvstore_demo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(kvstore_demo PRIVATE Threads::Threads)

# Unit Tests Executable
enable_testing()

add_executable(kvstore_tests
    tests/test_kvstore.cpp
    src/kvstore.cpp
)

target_include_directories(kvstore_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(kvstore_tests PRIVATE Threads::Threads)

add_test(NAME KeyValueStoreTests COMMAND kvstore_tests)
