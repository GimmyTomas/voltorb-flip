# Building Instructions

## Requirements

- CMake 3.16+
- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- Git (for fetching test dependencies)

## Quick Start

```bash
# Clone the repository
git clone https://github.com/yourusername/voltorb-flip-solver.git
cd voltorb-flip-solver

# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
cmake --build .

# Run tests
ctest --output-on-failure

# Run applications
./voltorb_assistant    # Interactive solver
./voltorb_selfplay     # Automated testing
```

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `VOLTORB_ENABLE_THREADS` | ON | Enable multi-threading support |
| `VOLTORB_BUILD_TESTS` | ON | Build test suite |

Example with options:
```bash
cmake -DVOLTORB_BUILD_TESTS=OFF ..
```

## Build Types

```bash
# Debug build (default)
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build (optimized)
cmake -DCMAKE_BUILD_TYPE=Release ..

# Release with debug info
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
```

## Platform-Specific Notes

### macOS

```bash
# Using Homebrew CMake
brew install cmake
cmake -DCMAKE_CXX_COMPILER=clang++ ..
```

### Linux

```bash
# Ubuntu/Debian
sudo apt install cmake g++

# Build
cmake -DCMAKE_CXX_COMPILER=g++ ..
```

### Windows (Visual Studio)

```bash
# Generate VS solution
cmake -G "Visual Studio 17 2022" ..

# Build from command line
cmake --build . --config Release
```

## Running Tests

```bash
# Run all tests
ctest

# Run with verbose output
ctest --output-on-failure

# Run specific test
ctest -R test_board

# Run tests in parallel
ctest -j4
```

## Applications

### Voltorb Assistant

Interactive solver for real gameplay:

```bash
./voltorb_assistant
```

Commands:
- `new <level>` - Start new game
- `hint <row> <sum> <voltorbs>` - Set row hint
- `hintc <col> <sum> <voltorbs>` - Set column hint
- `flip <row> <col> <value>` - Reveal panel
- `solve` - Get recommendation
- `probs` - Show all probabilities
- `safe` - Show guaranteed safe panels

### Voltorb Self-Play

Automated testing and statistics:

```bash
./voltorb_selfplay [options]
```

Options:
- `-n <num>` - Number of games (default: 100)
- `-l <level>` - Play at specific level (default: progression)
- `-v` - Verbose output
- `-s <seed>` - Random seed

Example:
```bash
./voltorb_selfplay -n 1000 -l 8 -v
```

## Integration

### As a Library

```cmake
# In your CMakeLists.txt
add_subdirectory(voltorb-flip-solver)
target_link_libraries(your_target PRIVATE voltorb_lib)
```

### Header-Only Usage

The headers are designed for easy inclusion:

```cpp
#include "voltorb/solver.hpp"

voltorb::Solver solver;
auto result = solver.solve(board);
```

## Troubleshooting

### CMake Version Too Old

```bash
# Install newer CMake
pip install cmake --upgrade
```

### C++20 Features Not Supported

Ensure your compiler supports C++20:
```bash
g++ --version    # Need 10+
clang++ --version # Need 12+
```

### Test Dependencies Fail to Fetch

```bash
# Manual Catch2 installation
git clone https://github.com/catchorg/Catch2.git
cd Catch2 && cmake -B build && cmake --build build --target install
```
