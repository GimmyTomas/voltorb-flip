# Voltorb Flip Solver

An optimal solver for the Voltorb Flip minigame from Pokémon HeartGold & SoulSilver, using Bayesian inference and minimax search.

## Features

- **Optimal play**: Finds the panel that maximizes win probability
- **Bayesian inference**: Accurately models uncertainty across board types
- **Fast for most boards**: Uses memoization and early termination
- **Iterative deepening**: Anytime results with increasing search depth
- **Web GUI**: Browser-based interface with live probability display ([play online](https://gimmytomas.github.io/voltorb-flip/))
- **WASM solver**: C++ solver compiled to WebAssembly for near-native speed in the browser
- **Interactive assistant**: Use alongside real gameplay (CLI)
- **Self-play testing**: Validate solver performance

## Quick Start

### Native C++ Build

```bash
# Build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .

# Run interactive assistant
./voltorb_assistant

# Run self-play test
./voltorb_selfplay -n 100
```

### WASM Build (optional)

Requires [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html):

```bash
source ~/emsdk/emsdk_env.sh
./build_wasm.sh
# Output: docs/js/solver-wasm.js, docs/js/voltorb_wasm.wasm
```

## Web GUI

A pixel-art browser interface for the solver, hosted on GitHub Pages:

**[Play Online](https://gimmytomas.github.io/voltorb-flip/)**

Features:
- Assistant mode: enter hints from the real game and get optimal move suggestions
- Self-play demo: watch the solver play automatically
- Live probability overlays with color-coded safety bars
- Choice of WASM (default) or JS solver engine
- Progressive depth updates showing improving results in real time

No server required — runs entirely client-side.

## How It Works

1. **Generate compatible boards**: Enumerate all board configurations matching revealed information
2. **Bayesian inference**: Calculate P(type | evidence) for each board type
3. **Recursive minimax**: Compute expected win probability for each panel choice
4. **Select optimal panel**: Flip the panel maximizing win probability

For complex boards, the solver uses iterative deepening with a timeout, yielding approximate results that improve with deeper search.

See [documentation/algorithm.md](documentation/algorithm.md) for details.

## Usage

### Interactive Assistant

```
> new 3                           # Start level 3 game
> hint 0 8 1                      # Row 0: sum=8, voltorbs=1
> hintc 2 7 2                     # Column 2: sum=7, voltorbs=2
> flip 0 0 1                      # Reveal panel (0,0) = 1
> solve                           # Get recommendation
Suggested panel: (2,3)
Win probability: 0.7234
> safe                            # Show safe panels
Safe panels: (0,1) (0,3)
```

### Self-Play

```bash
# Play 1000 games with level progression
./voltorb_selfplay -n 1000

# Play at specific level
./voltorb_selfplay -n 500 -l 8 -v
```

## Project Structure

```
voltorb-flip/
├── include/voltorb/   # Public headers
├── src/               # C++ implementation
├── apps/              # CLI applications
│   ├── assistant/     # Interactive solver
│   └── selfplay/      # Automated testing
├── tests/             # Unit tests
├── docs/              # Web GUI (GitHub Pages root)
│   ├── js/            # JavaScript modules + WASM output
│   ├── css/           # Stylesheets
│   └── wasm/          # WASM bindings source
├── documentation/     # Algorithm & architecture docs
├── build_wasm.sh      # WASM build script
└── legacy/            # Original implementation
```

## Documentation

- [Game Rules](documentation/game-rules.md) - Complete Voltorb Flip rules
- [Algorithm](documentation/algorithm.md) - Bayesian inference and minimax
- [Optimal Solver](documentation/optimal_solver.md) - Detailed solver algorithm
- [Architecture](documentation/architecture.md) - Code organization
- [Building](documentation/building.md) - Build instructions

## Performance

| Level | Typical Time (C++) | WASM (browser) | Result |
|-------|-------------------|----------------|--------|
| 1-3   | < 100ms           | < 200ms        | Exact  |
| 4-6   | 100ms-5s          | 200ms-5s       | Exact or depth-limited |
| 7-8   | 1s-5s             | 1s-10s         | Depth-limited, improving |

The solver uses iterative deepening with a configurable timeout (default 60s in web GUI). Each depth iteration improves the result, and the WASM engine runs at near-native speed.

## Requirements

- C++20 compatible compiler
- CMake 3.16+
- Emscripten SDK (optional, for WASM build)

## License

MIT License - see LICENSE file.

## Acknowledgments

- Original Voltorb Flip game by Nintendo/Game Freak
- Algorithm inspired by Bayesian decision theory
