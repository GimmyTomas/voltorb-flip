# Solver Algorithm

This document explains the Bayesian inference and minimax algorithm used by the solver.

## Overview

The solver finds the optimal panel to flip by:
1. Generating all board configurations compatible with revealed information
2. Using Bayesian inference to calculate type probabilities
3. Recursively computing win probability for each panel choice
4. Selecting the panel that maximizes win probability

## Bayesian Framework

### Prior Probabilities

Each board type has equal prior probability: P(type) = 1/10

### Likelihood

Given observed hints and revealed panels (collectively "evidence" E):

```
P(E | type) = N_compatible(type) / N_accepted(type)
```

Where:
- `N_compatible(type)` = number of boards of this type matching the evidence
- `N_accepted(type)` = total valid boards of this type (pre-computed)

### Posterior

Using Bayes' theorem:

```
P(type | E) = P(E | type) × P(type) / P(E)
            = [N_compatible(type) / N_accepted(type)] / Σ_t [N_compatible(t) / N_accepted(t)]
```

### Board-to-Type Assignment

**Important**: Multiple board types can have the same (n0, n2, n3) but different legality
constraints (maxTotalFree, maxRowFree). For correct Bayesian inference, each compatible
board must be counted in **all** types it is legal for.

Example: Level 2 has Type 0 and Type 5 both with (n0=7, n2=1, n3=3):
- Type 0: maxTotalFree=3, maxRowFree=2, N_accepted=3,285,100,800
- Type 5: maxTotalFree=2, maxRowFree=2, N_accepted=2,684,683,200

A board satisfying the stricter Type 5 constraint is legal for *both* types and
contributes to both types' probability mass. This is mathematically correct because
when the game generates a board, the player doesn't know which type was selected.

## Panel Probability Calculation

For any unknown panel position (i, j), the probability of each value is computed in
three steps:

### Step 1: Type Posterior

```
P(type | E) = [N_compatible(type) / N_accepted(type)] / Σ_t [N_compatible(t) / N_accepted(t)]
```

### Step 2: Value Probability Given Type

For each board type, count compatible boards with each panel value:

```
P(panel[i,j] = v | type, E) = count(boards of type where panel[i,j] = v) / N_compatible(type)
```

### Step 3: Marginalize Over Types

Apply the law of total probability:

```
P(panel[i,j] = v | E) = Σ_type P(panel[i,j] = v | type, E) × P(type | E)
```

This formula weights each type's value distribution by the probability that the
board came from that type.

### Implementation Note

The C++ and JavaScript solvers produce identical results to machine precision (< 1e-10).
Both implementations:
1. Check each compatible board for legality against ALL types with matching (n0, n2, n3)
2. Count the board in every type it satisfies (no early break)
3. Use the same Bayesian formulas above

## Minimax Algorithm

### Win Function

Define f(board) recursively:

1. **Base case (won)**: f(board) = 1 if all multipliers revealed
2. **Base case (lost)**: f(board) = 0 if a Voltorb is revealed
3. **Recursive case**:

```
f(board) = max_panel { Σ_v P(panel = v | board) × f(board_with_panel_revealed) }
```

### Optimal Panel

The optimal panel to flip is the one achieving the maximum in the recursive definition.

### Free Panel Optimization

If any panel is **guaranteed safe** (non-Voltorb in all compatible boards), flip it immediately. This:
- Provides information without risk
- Avoids expensive minimax computation
- Is always optimal (or tied for optimal)

## Iterative Deepening

The full minimax tree can be intractable for complex boards. The solver uses **iterative deepening** to provide anytime results:

1. Start with depth limit = 1
2. Run depth-limited minimax with a fresh memoization table
3. Yield the result (depth N, approximate if depth limit was hit)
4. Increment depth limit and repeat
5. Stop when: exact solution found, timeout reached, or max depth reached

### Depth-Limited Search

At each depth, the solver performs bounded expectimax:
- **Win check**: Return 1.0 if all multipliers revealed
- **Depth limit reached**: Return heuristic evaluation (see below)
- **Memo lookup**: Return cached result if available
- **Free panels**: Reveal without spending depth (always optimal)
- **Risky panels**: Loop over panels with upper-bound pruning, recurse at depthLimit-1

### Heuristic Evaluation

At the depth limit, win probability is estimated by:
1. Count remaining multipliers to reveal
2. Collect Voltorb probability of each risky panel
3. Sort by risk (lowest Voltorb probability first)
4. Return product of survival probabilities for the N safest panels

### Timeout Handling

The solver monitors elapsed time and returns the best depth-limited result when:
- Time exceeds configured timeout (5s for JS, 10s for C++)

Results are marked as "approximate" when the full tree was not explored.

### Monte Carlo Fallback (Deprecated)

The previous Monte Carlo sampling fallback has been superseded by iterative deepening with heuristic evaluation. The iterative deepening approach provides principled anytime results that improve monotonically with depth, unlike Monte Carlo which gave inconsistent estimates.

## Complexity Analysis

### Exhaustive Search

- **Compatible boards**: Can be millions for early-game positions
- **Recursion depth**: Up to 25 (one per panel)
- **Memoization**: Essential for tractable computation

### Iterative Deepening

- **Per-depth cost**: Bounded by depth limit and memoization
- **Heuristic evaluation**: O(unknown panels) at leaf nodes
- **Anytime property**: Each depth iteration improves on the previous

## Implementation Details

### Memoization

Board states are hashed for cache lookup. Two strategies are used:

**C++ (Zobrist hashing + transposition table):**
- Incremental XOR hash updates when revealing panels (O(1) per update)
- Fixed-size power-of-2 transposition table with full hash verification
- Depth tracking: only use cached result if stored depth >= current depth

**JavaScript (`compactKey()` + Map):**
- String key = level + 25 panel digits (each panel value shifted to single digit)
- Collision-free by construction
- Fresh `Map` created for each depth iteration to prevent stale results

### Early Termination

1. **Free panels**: Skip full minimax when safe panels exist
2. **Alpha-beta**: Prune branches that can't improve best found
3. **Timeout**: Return best result from current depth

### Parallel Enumeration

Board generation can be parallelized across:
- Voltorb position configurations (per row)
- Non-Voltorb value assignments (per type)

## References

- Bayesian decision theory
- Minimax game tree search
- Iterative deepening depth-first search (IDDFS)
