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
├── sampler.hpp      # Monte Carlo sampling fallback
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

## Solver (solver.hpp)

### SolverOptions
Configuration:
- `timeout`: Max computation time
- `maxCompatibleBoards`: Fallback threshold
- `sampleSize`: Monte Carlo sample count

### SolverResult
Output:
- `suggestedPosition`: Best panel to flip
- `winProbability`: Expected win rate
- `isExact`: Whether exhaustive or sampled
- `computeTime`: Actual computation time

### Solver
Main algorithm:
1. Generate compatible boards (or sample if too many)
2. Find free panels (guaranteed safe)
3. Recursive minimax with memoization
4. Timeout handling → Monte Carlo fallback

## Sampler (sampler.hpp)

### MonteCarloSampler
Fallback for intractable boards:
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

## Data Flow

```
User Input → GameSession
                 ↓
           BoardGenerator → Board (actual)
                 ↓
           Board (covered) → Solver
                                ↓
                        CompatibleBoardGenerator
                                ↓
                        ProbabilityCalculator
                                ↓
                        Minimax Recursion
                                ↓
                        SolverResult
                                ↓
           GameSession ← Suggested Panel
                 ↓
           User Output
```

## Performance Considerations

### Memory
- Compatible boards can be large (millions)
- Memoization cache grows with search depth
- Monte Carlo uses fixed-size sample buffer

### Time
- Exhaustive search: exponential in unknown panels
- Monte Carlo: linear in sample size
- Free panel detection: early termination

### Parallelism
- Board enumeration: parallelizable per type
- Minimax: can parallelize panel evaluation
- Sampling: embarrassingly parallel
