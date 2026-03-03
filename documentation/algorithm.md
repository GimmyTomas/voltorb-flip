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

The C++, JavaScript, and WASM solvers produce identical results to machine precision (< 1e-10).
All implementations:
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

If any panel is **guaranteed safe** (non-Voltorb in all compatible boards) **and** has multiplier potential (some compatible board has a 2 or 3 there), flip it immediately. This:
- Provides information without risk
- Avoids expensive minimax computation
- Is always optimal (or tied for optimal)

Safe panels without multiplier potential are "useless" — see below.

### Useless Panel Pruning

A panel is **useless** if no compatible board has a multiplier (2 or 3) at that position. Flipping such a panel can only reveal a 1 (no progress toward winning) or a Voltorb (game over). The solver never explores or recommends useless panels.

This is checked via `hasMultiplierPotential(state, pos)`, which returns true if any compatible board has a 2 or 3 at the position. The filter is applied in three places:

1. **Free panel detection**: Only return a free panel if it has multiplier potential
2. **Risky panel ordering**: Exclude useless panels from the candidate list
3. **Heuristic evaluation**: Only count panels with multiplier potential when estimating survival probability

**Edge cases:**
- If all remaining unknowns are useless, `isWon()` returns true (all multipliers already revealed), so the empty panel list is never reached
- If all free panels are useless, `findFreePanel` returns null and the search proceeds to risky panels with multiplier potential
- If all risky panels are useless, the heuristic returns 1.0 (correct, since remaining multipliers must all be safe)

## Solver Implementation

The full minimax tree can be intractable for complex boards. The solver uses **iterative deepening** with heuristic evaluation and memoization to provide anytime results that improve with deeper search. Key aspects:

- **Iterative deepening**: Searches at increasing depth limits, yielding the best result so far at each depth
- **Heuristic evaluation**: At the depth limit, estimates win probability from the survival product of the safest panels
- **Memoization**: C++ uses Zobrist hashing with a fixed-size transposition table; JavaScript uses `compactKey()` with a `Map`
- **Pruning**: Upper-bound pruning, useless panel pruning, and free panel short-circuiting
- **Timeout**: Configurable (60s default in web GUI, 10s for C++ CLI)

For full details on iterative deepening, depth-limited search, heuristic evaluation, approximation bounds, and performance characteristics, see [optimal_solver.md](optimal_solver.md).

## References

- Bayesian decision theory
- Minimax game tree search
- Iterative deepening depth-first search (IDDFS)
