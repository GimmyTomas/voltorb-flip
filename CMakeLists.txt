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
    src/zobrist.cpp
    src/transposition.cpp
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

# Legacy solver library
add_library(legacy_solver_lib legacy/legacy_solver.cpp)
target_include_directories(legacy_solver_lib PUBLIC legacy)

# Legacy interactive app
add_executable(legacy_solver legacy/main.cpp)
target_link_libraries(legacy_solver PRIVATE legacy_solver_lib)

# Debug investigation tools
add_executable(investigate_diff debug/investigate_diff.cpp)
target_link_libraries(investigate_diff PRIVATE voltorb_lib legacy_solver_lib)

add_executable(investigate_level2 debug/investigate_level2.cpp)
target_link_libraries(investigate_level2 PRIVATE voltorb_lib legacy_solver_lib)

add_executable(trace_legacy debug/trace_legacy.cpp)
target_link_libraries(trace_legacy PRIVATE voltorb_lib legacy_solver_lib)

add_executable(trace_fill debug/trace_fill.cpp)
target_link_libraries(trace_fill PRIVATE voltorb_lib legacy_solver_lib)

add_executable(benchmark_solver debug/benchmark_solver.cpp)
target_link_libraries(benchmark_solver PRIVATE voltorb_lib)

# WebAssembly target (built with Emscripten)
option(VOLTORB_BUILD_WASM "Build WebAssembly module" OFF)

if(VOLTORB_BUILD_WASM AND EMSCRIPTEN)
    add_executable(voltorb_wasm docs/wasm/solver_bindings.cpp)
    target_link_libraries(voltorb_wasm PRIVATE voltorb_lib)
    target_link_options(voltorb_wasm PRIVATE
        --bind
        -sNO_EXIT_RUNTIME=1
        -sMODULARIZE=1
        -sEXPORT_ES6=1
        -sEXPORT_NAME="VoltorbSolverModule"
        -sENVIRONMENT='worker'
        -sALLOW_MEMORY_GROWTH=1
        -sINITIAL_MEMORY=67108864
        -sMAXIMUM_MEMORY=268435456
    )
    set_target_properties(voltorb_wasm PROPERTIES
        OUTPUT_NAME "voltorb_wasm"
        SUFFIX ".js"
    )
endif()

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
