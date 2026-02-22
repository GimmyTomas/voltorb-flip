include(FetchContent)

# Fetch Catch2 for testing
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(Catch2)

# Test executables
add_executable(test_board test_board.cpp)
target_link_libraries(test_board PRIVATE voltorb_lib Catch2::Catch2WithMain)

add_executable(test_constraints test_constraints.cpp)
target_link_libraries(test_constraints PRIVATE voltorb_lib Catch2::Catch2WithMain)

add_executable(test_solver test_solver.cpp)
target_link_libraries(test_solver PRIVATE voltorb_lib Catch2::Catch2WithMain)

add_executable(test_legacy_comparison test_legacy_comparison.cpp)
target_link_libraries(test_legacy_comparison PRIVATE voltorb_lib Catch2::Catch2WithMain)

# Register tests with CTest
include(Catch)
catch_discover_tests(test_board)
catch_discover_tests(test_constraints)
catch_discover_tests(test_solver)
catch_discover_tests(test_legacy_comparison)
