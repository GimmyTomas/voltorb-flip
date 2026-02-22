# Voltorb Flip Solver

An optimal solver for the Voltorb Flip minigame from Pokémon HeartGold & SoulSilver, using Bayesian inference and minimax search.

## Features

- **Optimal play**: Finds the panel that maximizes win probability
- **Bayesian inference**: Accurately models uncertainty across board types
- **Fast for most boards**: Uses memoization and early termination
- **Monte Carlo fallback**: Approximates solutions for complex boards
- **Interactive assistant**: Use alongside real gameplay
- **Self-play testing**: Validate solver performance

## Quick Start

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

## How It Works

1. **Generate compatible boards**: Enumerate all board configurations matching revealed information
2. **Bayesian inference**: Calculate P(type | evidence) for each board type
3. **Recursive minimax**: Compute expected win probability for each panel choice
4. **Select optimal panel**: Flip the panel maximizing win probability

For boards with too many compatible configurations, the solver falls back to Monte Carlo sampling for approximate results.

See [docs/algorithm.md](docs/algorithm.md) for details.

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
├── src/               # Implementation
├── apps/              # Applications
│   ├── assistant/     # Interactive solver
│   └── selfplay/      # Automated testing
├── tests/             # Unit tests
├── docs/              # Documentation
└── legacy/            # Original implementation
```

## Documentation

- [Game Rules](docs/game-rules.md) - Complete Voltorb Flip rules
- [Algorithm](docs/algorithm.md) - Bayesian inference and minimax
- [Architecture](docs/architecture.md) - Code organization
- [Building](docs/building.md) - Build instructions

## Performance

| Level | Exhaustive | Monte Carlo |
|-------|------------|-------------|
| 1-3   | < 100ms    | N/A         |
| 4-6   | 100ms-5s   | Fallback    |
| 7-8   | 1s-timeout | ~500ms      |

The solver uses a 5-second timeout by default and falls back to Monte Carlo sampling when exhaustive search is too slow.

## Requirements

- C++20 compatible compiler
- CMake 3.16+

## License

MIT License - see LICENSE file.

## Acknowledgments

- Original Voltorb Flip game by Nintendo/Game Freak
- Algorithm inspired by Bayesian decision theory
