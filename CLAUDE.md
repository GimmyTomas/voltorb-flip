# Project Notes for Claude

## Overview

Voltorb Flip Solver — an optimal solver for the Voltorb Flip minigame from Pokémon HeartGold & SoulSilver. The project has three main components:

1. **C++ solver library** (`include/voltorb/`, `src/`) — Bayesian inference + iterative deepening minimax
2. **Web GUI** (`docs/`) — pixel-art browser interface with JS solver, hosted on GitHub Pages
3. **WASM module** (`docs/wasm/`, `docs/js/solver-wasm.js`) — C++ solver compiled to WebAssembly for use in the web GUI

## Architecture

- **C++ core**: Board representation, constraint enumeration, probability calculation, minimax solver with Zobrist hashing and transposition table
- **JS web frontend**: `docs/js/app.js` (controller), `docs/js/ui.js` (DOM), `docs/js/solver.js` (JS solver), `docs/js/board.js` (board model)
- **Web Worker**: `docs/js/solver-worker.js` — runs solver off main thread, posts `'progress'` and `'complete'` messages
- **WASM bindings**: `docs/wasm/solver_bindings.cpp` — Emscripten bindings exposing `solveBoard()` and `solveBoardWithProgress()` to JS

## Key Files

| Path | Description |
|------|-------------|
| `include/voltorb/solver.hpp` | Solver interface, SolverResult, SolverProgress, ProgressCallback |
| `src/solver.cpp` | Solver implementation (iterative deepening, minimax, heuristic) |
| `docs/wasm/solver_bindings.cpp` | Emscripten WASM bindings |
| `docs/js/solver-worker.js` | Web Worker dispatching JS and WASM solvers |
| `docs/js/app.js` | Main web app controller |
| `docs/js/solver.js` | JS solver (iterativeDeepening, probabilities) |
| `docs/js/ui.js` | DOM rendering, probability overlays |
| `build_wasm.sh` | Script to build WASM module via Emscripten |
| `CMakeLists.txt` | CMake build config (native + WASM targets) |

## Build Instructions

### Native C++
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

### WASM (requires Emscripten SDK)
```bash
source ~/emsdk/emsdk_env.sh
./build_wasm.sh
# Output: docs/js/solver-wasm.js, docs/js/voltorb_wasm.wasm
```

## Workflow Reminders

- **Always commit and push** after making changes to the web GUI. The user tests changes online via GitHub Pages, so changes must be pushed to be visible.
- The live site is at: https://gimmytomas.github.io/voltorb-flip/
- WASM rebuild requires Emscripten SDK (`source ~/emsdk/emsdk_env.sh` before `./build_wasm.sh`)
