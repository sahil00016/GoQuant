cmake_minimum_required(VERSION 3.10)
project(matching_engine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)

# Add include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/cpp-httplib
)

# Add source files
set(SOURCES
    src/main.cpp
    src/matching_engine.cpp
    src/order_book.cpp
    src/api/http_server.cpp
)

# Create executable
add_executable(matching_engine ${SOURCES})

# Link libraries
target_link_libraries(matching_engine
    PRIVATE
    Threads::Threads
) 