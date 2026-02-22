# Legacy Voltorb Flip Solver

This directory contains the original implementation of the Voltorb Flip solver.

## Files

- `main.cpp` - Original C++ implementation
- `algorithm/` - LaTeX documentation of the algorithm
- `old/` - Even older versions

## Notes

The legacy solver works correctly but has performance issues on certain board configurations where the number of compatible boards explodes. The new implementation addresses this with:

1. Better memoization
2. Early termination for safe panels
3. Monte Carlo fallback for complex boards
4. Timeout handling

## Original Author

Giovanni Maria Tomaselli (April 2022)
