# Legacy Voltorb Flip Solver

This directory contains the original implementation of the Voltorb Flip solver, refactored into a library for use in testing.

## Files

- `legacy_solver.hpp` - Header file with public API
- `legacy_solver.cpp` - Refactored implementation as a library
- `main.cpp` - Original C++ implementation (standalone)
- `algorithm/` - LaTeX documentation of the algorithm
- `old/` - Even older versions

## Library Usage

The legacy solver is compiled as a static library (`liblegacy_solver_lib.a`) and used primarily for comparison testing against the new solver.

## Compatibility with New Solver

After a bug fix to the `find_and_fill_row_with_possibilities` function (replaced with exhaustive recursive backtracking), both solvers now produce:

- **Identical compatible board counts** for all levels and board types
- **Panel probabilities within 0.1%** (minor differences due to different board type weighting formulas in probability calculations)

## Performance Notes

The legacy solver has performance issues on certain board configurations where the number of compatible boards explodes. The new implementation addresses this with:

1. Better memoization
2. Early termination for safe panels
3. Monte Carlo fallback for complex boards
4. Timeout handling

## Original Author

Giovanni Maria Tomaselli (April 2022)
