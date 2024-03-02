cmake_minimum_required(VERSION 3.13)

set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(TinyMake CXX)

find_package(GTest REQUIRED)

add_compile_options(-O3 -g -Wall -Werror)

set(SRC_DIR "src")
set(INCLUDE_DIR "include")
set(TEST_DIR "tests")

include_directories(${INCLUDE_DIR})

# This is safer than GLOB (GLOB is not allowed in production)
set(SRCS
    src/auto-var-replacement.cpp
    src/lexer.cpp
    src/parser.cpp
    src/rule-dep.cpp
    src/rule-filter.cpp
    src/thread-pool.cpp
    src/var-replacement.cpp
    src/var-resolution.cpp
)

# Executable
add_executable(${CMAKE_PROJECT_NAME} ${SRCS} "src/main.cpp")

# Tests
file(GLOB_RECURSE TEST_FILES "${TEST_DIR}/*.cpp")
set(TEST_EXECUTABLE "${CMAKE_PROJECT_NAME}-tests")
add_executable(${TEST_EXECUTABLE} ${TEST_FILES} ${SRCS})
target_link_options(${TEST_EXECUTABLE} PRIVATE -no-pie) # By default, gtest is built as a static library, thus `-no-pie`.
target_link_libraries(${TEST_EXECUTABLE} GTest::GTest GTest::Main) # Thread library is added automatically.