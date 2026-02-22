cmake_minimum_required(VERSION 3.16)
project(voltorb-flip VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

# Option for thread support
option(VOLTORB_ENABLE_THREADS "Enable multi-threading support" ON)

# Find threads if enabled
if(VOLTORB_ENABLE_THREADS)
    find_package(Threads REQUIRED)
endif()

# Library
add_library(voltorb_lib
    src/board.cpp
    src/board_type.cpp
    src/constraints.cpp
    src/probability.cpp
    src/solver.cpp
    src/sampler.cpp
    src/generator.cpp
    src/game.cpp
)

target_include_directories(voltorb_lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(VOLTORB_ENABLE_THREADS)
    target_link_libraries(voltorb_lib PUBLIC Threads::Threads)
    target_compile_definitions(voltorb_lib PUBLIC VOLTORB_ENABLE_THREADS)
endif()

# Assistant app
add_executable(voltorb_assistant apps/assistant/main.cpp)
target_link_libraries(voltorb_assistant PRIVATE voltorb_lib)

# Self-play app
add_executable(voltorb_selfplay apps/selfplay/main.cpp)
target_link_libraries(voltorb_selfplay PRIVATE voltorb_lib)

# Testing
option(VOLTORB_BUILD_TESTS "Build test suite" ON)
if(VOLTORB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
install(TARGETS voltorb_lib voltorb_assistant voltorb_selfplay
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/voltorb DESTINATION include)
