# Architecture Overview

## Module Structure

```
voltorb/
├── types.hpp        # Core type definitions
├── board.hpp        # Board representation
├── board_type.hpp   # Board type parameters & lookup tables
├── constraints.hpp  # Legality checking & board enumeration
├── probability.hpp  # Probability calculations
├── solver.hpp       # Minimax solver with memoization
├── zobrist.hpp      # Zobrist hash key generation
├── transposition.hpp # Transposition table for memoization
├── sampler.hpp      # Monte Carlo sampling fallback (deprecated)
├── generator.hpp    # Random board generation
└── game.hpp         # Game session management
```

## Core Types (types.hpp)

### PanelValue
Enum representing panel states: `Unknown`, `Voltorb`, `One`, `Two`, `Three`

### Position
Row/column coordinate on the 5×5 board with conversion utilities.

### LineHint
Sum and Voltorb count for a row or column.

### GameResult
Enum: `InProgress`, `Won`, `Lost`

## Board (board.hpp)

Represents game state:
- 5×5 panel grid
- Row and column hints
- Level

Key methods:
- `get()/set()`: Panel access
- `checkGameResult()`: Determine win/loss
- `hash()`: For memoization
- `toCovered()`: Create covered version

## Board Type Data (board_type.hpp)

Static lookup tables for all 80 board types (8 levels × 10 types):
- `BoardTypeParams`: n0, n2, n3, constraints
- Pre-computed permutation counts
- Pre-computed acceptance counts (valid boards)

## Constraints (constraints.hpp)

### LegalityChecker
Validates boards against free-row/column multiplier limits.

### VoltorbPositionGenerator
Enumerates all valid Voltorb placements given hints:
- Row-by-row permutation
- Early pruning via column count tracking
- Conflict detection with revealed panels

### CompatibleBoardGenerator
Builds complete boards from Voltorb positions:
- Type-specific 2/3 placement
- Constraint propagation
- Backtracking search

## Probability (probability.hpp)

### ProbabilityCalculator
Bayesian inference implementation:
- Type posterior: P(type | evidence)
- Panel probabilities: P(panel = v | evidence)
- Safe panel detection

### BoardProbabilities
Complete probability state for solver decisions.

## Zobrist Hashing (zobrist.hpp)

Random key generation for incremental board hashing:
- Pre-computed random keys per (position, value) pair
- O(1) hash update when revealing a panel via XOR

## Transposition Table (transposition.hpp)

Fixed-size power-of-2 hash table for memoization:
- Full hash verification to handle collisions
- Depth tracking: only use cached result if stored depth >= current depth
- Always-replace policy (effective for iterative deepening)
- Configurable size (default 1M entries for C++, 256K for WASM)

## Solver (solver.hpp)

### SolverOptions
Configuration:
- `timeout`: Max computation time
- `maxDepth`: Maximum iterative deepening depth
- `maxCompatibleBoards`: Board enumeration cap

### SolverResult
Output:
- `suggestedPosition`: Best panel to flip
- `winProbability`: Expected win rate
- `isExact`: Whether exhaustive or sampled
- `computeTime`: Actual computation time

### SolverProgress
Per-depth progress update:
- `bestPanel`: Best panel found at this depth
- `winProbability`: Win probability estimate
- `depth`: Search depth completed
- `isExact`: Whether full tree was explored
- `nodesSearched`: Nodes evaluated so far

### Solver
Main algorithm:
1. Generate compatible boards
2. Prune useless panels (no multiplier potential — can only be 1 or Voltorb)
3. Find free panels (guaranteed safe with multiplier potential)
4. Iterative deepening minimax with memoization
5. Timeout handling → return best depth-limited result

## Sampler (sampler.hpp) — Deprecated

### MonteCarloSampler
> **Note:** The Monte Carlo sampler is no longer used by the solver. It has been
> superseded by iterative deepening with heuristic evaluation, which provides
> principled anytime results. The sampler code is retained for reference only.

Deprecated fallback for intractable boards (no longer used):
- Rejection sampling of random boards
- Type weighting by acceptance count
- Probability estimation from samples

## Generator (generator.hpp)

### BoardGenerator
Creates random valid boards:
- Shuffle-based placement
- Rejection sampling for legality
- Configurable seed for reproducibility

## Game (game.hpp)

### GameSession
Interactive gameplay management:
- Board generation
- Panel flipping with result tracking
- Solver integration for suggestions
- Level progression

### SelfPlayer
Automated testing:
- Play N games with solver
- Statistics collection
- Win rate calculation

## Web Architecture

### JavaScript Modules

The web GUI (`docs/`) is a pure client-side application:

```
docs/
├── js/
│   ├── app.js           # Main controller (mode switching, event handling)
│   ├── ui.js            # DOM rendering, probability overlays, animations
│   ├── board.js         # Board model (mirrors C++ Board)
│   ├── boardTypes.js    # Board type data (80 types, N_accepted)
│   ├── solver.js        # JS solver (probabilities, iterative deepening)
│   ├── generator.js     # Random board generation
│   ├── solver-worker.js # Web Worker (dispatches JS or WASM solver)
│   ├── solver-wasm.js   # Emscripten WASM loader (generated)
│   └── voltorb_wasm.wasm # Compiled C++ solver (generated)
├── wasm/
│   └── solver_bindings.cpp  # C++ Emscripten bindings (source)
├── css/
│   ├── style.css        # Main pixel-art styling
│   └── docs.css         # Algorithm documentation page styling
├── index.html           # Main page
└── algorithm.html       # Algorithm documentation page
```

### Web Worker Architecture

The solver runs in a **Web Worker** to keep the UI thread responsive:

```
Main Thread (app.js)          Worker Thread (solver-worker.js)
       |                               |
       |--- postMessage({solve}) ----->|
       |                               |--- JS solver or WASM solver
       |<-- postMessage({progress}) ---|    (posts 'progress' at each depth)
       |<-- postMessage({progress}) ---|
       |<-- postMessage({complete}) ---|
       |                               |
  UI updates on each message
```

### WASM Integration

The WASM module (`solver_bindings.cpp`) exposes two functions:

1. **`solveBoard()`** — original blocking call, returns final result only
2. **`solveBoardWithProgress()`** — accepts a JS callback that is called at each completed depth level with `{bestPanel, winProbability, depth, isExact, nodesSearched}`

The worker detects which function is available and uses `solveBoardWithProgress` when possible, falling back to `solveBoard` for backward compatibility.

Probabilities and safe panels are computed in JS (fast, using the same Bayesian inference) before the WASM search begins, so progress messages include full probability data from depth 1 onward.

## Data Flow

```
User Input → GameSession (or Web UI)
                 ↓
           BoardGenerator → Board (actual)
                 ↓
           Board (covered) → Solver / Web Worker
                                ↓
                        CompatibleBoardGenerator
                                ↓
                        ProbabilityCalculator
                                ↓
                        Minimax Recursion (iterative deepening)
                                ↓ (progress at each depth)
                        SolverResult
                                ↓
           GameSession / UI ← Suggested Panel + Probabilities
                 ↓
           User Output
```

In the web GUI, this flow runs inside a Web Worker. The WASM path replaces the minimax recursion step with the compiled C++ solver, while probability calculation happens in JS.

## Performance Considerations

### Memory
- Compatible boards can be large (millions)
- Memoization cache grows with search depth (fresh per depth iteration)
- WASM transposition table: ~8MB (256K entries)

### Time
- Exhaustive search: exponential in unknown panels
- Iterative deepening: bounded by depth limit and timeout
- Free panel detection: early termination
- WASM: ~2-5x faster than JS for the minimax search

### Parallelism
- Board enumeration: parallelizable per type
- Minimax: can parallelize panel evaluation
- Sampling: embarrassingly parallel
