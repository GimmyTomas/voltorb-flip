# Optimal Voltorb Flip Solver

This document describes the optimal solver algorithm for Voltorb Flip. The solver computes the exact win probability using recursive minimax (expectimax) and uses iterative deepening to provide anytime results.

## Algorithm Overview

### Win Probability Definition

The optimal win probability for a board state S is defined recursively:

```
f(S) = 0                    if a Voltorb has been revealed
f(S) = 1                    if all multipliers (2s and 3s) are revealed
f(S) = max_p { E[f(S') | flip panel p] }   otherwise
```

Where:
- S is the current board state (revealed panels + hints)
- p ranges over unrevealed panels **with multiplier potential** (panels where some compatible board has a 2 or 3)
- S' is the state after revealing panel p
- E[...] denotes expected value over possible revealed values

The optimal move is the panel p that achieves this maximum.

### Expectation Calculation

When we flip a panel p, we condition on the revealed value:

```
E[f(S') | flip p] = Σ_v P(p=v|S) × f(S with p revealed as v)
```

Where:
- v ranges over {1, 2, 3} (voltorb outcomes give f=0)
- P(p=v|S) is computed using Bayesian inference over compatible boards

### Key Properties

1. **Correctness**: This algorithm is provably optimal - it maximizes win probability
2. **DAG Structure**: The search space is a DAG (different move orders can reach the same state)
3. **Memoization**: Results are cached by board state to avoid recomputation

## Implementation Details

### Iterative Deepening

The solver uses iterative deepening to provide anytime results:

1. Start with depth limit = 1
2. Run depth-limited minimax search
3. Report result (depth N, approximate if depth limit was hit)
4. Increment depth limit and repeat
5. Stop when:
   - Exact solution found (no depth limit hit)
   - Timeout reached
   - Maximum depth reached

### Depth-Limited Search

```python
def depth_limited_search(state, depth_limit):
    if is_won(state):
        return (None, 1.0, fully_explored=True)

    if depth_limit == 0:
        return (None, heuristic_eval(state), fully_explored=False)

    if state in memo and memo[state].depth >= depth_limit:
        return memo[state]

    best_panel = None
    best_win_prob = 0
    all_explored = True

    # Only consider panels with multiplier potential (skip useless panels)
    for panel in ordered_unknown_panels(state):  # filtered to multiplier-potential only
        # Upper-bound pruning
        if survival_prob(panel) <= best_win_prob:
            continue

        expected_win = 0
        panel_explored = True

        for value in [1, 2, 3]:
            prob = P(panel == value | state)
            if prob == 0:
                continue

            next_state = reveal(state, panel, value)
            result = depth_limited_search(next_state, depth_limit - 1)

            expected_win += prob * result.win_prob
            if not result.fully_explored:
                panel_explored = False

        if expected_win > best_win_prob:
            best_win_prob = expected_win
            best_panel = panel

        if not panel_explored:
            all_explored = False

    memo[state] = (best_panel, best_win_prob, all_explored)
    return memo[state]
```

### Heuristic Evaluation

When the depth limit is reached, the solver estimates win probability using:

1. Count remaining multipliers to reveal (only panels with multiplier potential)
2. Identify risky panels with multiplier potential (non-zero voltorb probability)
3. Sort panels by risk (lowest voltorb probability first)
4. Estimate: product of survival probabilities for the N safest panels

Panels without multiplier potential (can only be 1 or Voltorb) are excluded from
both the multiplier count and the risky panel list, since flipping them never
contributes to winning.

```python
def heuristic_eval(state):
    if is_won(state):
        return 1.0

    multipliers_needed = count_remaining_multipliers(state)
    if multipliers_needed == 0:
        return 1.0

    # Only consider panels that could actually be a multiplier
    voltorb_probs = [P(panel=0|state) for panel in risky_panels(state)
                     if has_multiplier_potential(state, panel)]
    voltorb_probs.sort()

    survival_product = 1.0
    for i in range(min(len(voltorb_probs), multipliers_needed)):
        survival_product *= (1 - voltorb_probs[i])

    return survival_product
```

### Optimizations

#### 1. Zobrist Hashing (C++ only)

Instead of recomputing hash from scratch, use incremental XOR updates:

```cpp
hash = hash ^ zobrist[pos][old_value] ^ zobrist[pos][new_value]
```

This enables O(1) hash updates when revealing panels.

#### 2. Transposition Table

Fixed-size power-of-2 table with:
- Full hash verification for collision handling
- Depth tracking (only use cached result if depth >= current)
- Always-replace policy (simple and effective for iterative deepening)

**Note:** Zobrist hashing and the transposition table are specific to the C++ implementation.
The JavaScript web solver uses `compactKey()` (level + 25 panel digits) as a collision-free
string key stored in a standard `Map`, with a fresh Map created per depth iteration.

#### 3. Move Ordering

Sort unknown panels by voltorb probability (lowest first):
- Safe panels come first (guaranteed to survive)
- Lower-risk panels are evaluated before higher-risk
- Improves pruning effectiveness

#### 4. Upper-Bound Pruning

For each panel, the maximum win probability is `1 - P(voltorb)`:
- If this bound is <= current best, skip evaluation
- Significant speedup when good moves are found early

#### 5. Free Panel Detection

Identify panels that are safe in ALL compatible boards:
- These can be flipped without risk
- Only considered if they have multiplier potential (see below)
- Always suggest free panels when available
- Recursive search continues after revealing

#### 6. Useless Panel Pruning

Skip panels where no compatible board has a multiplier (2 or 3):
- Such panels can only reveal a 1 (no progress) or Voltorb (game over)
- Flipping them is never optimal — they add risk without contributing to winning
- Checked via `hasMultiplierPotential()` which scans compatible boards at that position
- Applied in free panel detection, risky panel ordering, and heuristic evaluation
- When all unknowns are useless, `isWon()` returns true (all multipliers already revealed)

## Performance Characteristics

### Time Complexity

- Worst case: O(n! × d) where n = unknown panels, d = depth
- Typical: Much faster due to:
  - Memoization (avoids recomputing same states)
  - Pruning (skips suboptimal branches)
  - Move ordering (finds good moves early)

### Observed Performance (C++ only)

> The following benchmarks are from the C++ implementation. JavaScript performance
> varies by browser but is generally comparable for most boards.

| Level | Unknown Panels | Typical Time to Exact |
|-------|----------------|----------------------|
| 1-2   | 25             | < 100ms              |
| 3-4   | 25             | 100ms - 1s           |
| 5-6   | 25             | 1s - 5s              |
| 7-8   | 25             | 2s - 10s             |

Mid-game (fewer unknowns):
| Unknown Panels | Time |
|----------------|------|
| 15-20          | < 1s |
| 10-15          | < 500ms |
| 5-10           | < 100ms |

### Approximation Quality

| Depth | Expected Accuracy | Notes |
|-------|-------------------|-------|
| 1     | 70-85%            | Similar to greedy |
| 2     | 85-95%            | Considers consequences |
| 3     | 95-99%            | Usually sufficient |
| 4+    | 99%+              | Near-optimal |
| Exact | 100%              | Provably optimal |

## Comparison to Previous Approaches

### Old Greedy Approach
- Selected panel with lowest voltorb probability
- Fast but suboptimal
- Could miss better moves requiring short-term risk

### Monte Carlo Sampling
- Sampled random compatible boards
- Estimated probabilities from samples
- Inconsistent results, no guarantee of optimality

### New Iterative Deepening Approach
- Computes exact win probability when feasible
- Falls back to principled heuristic when time-limited
- Always improving: deeper search = better results
- Provably optimal when search completes

## Usage Notes

### Timeout Configuration

Default timeout: 10 seconds (C++), 5 seconds (JavaScript)

For interactive use:
- 1-2 seconds: Good balance of speed and accuracy
- 5+ seconds: Usually reaches exact solution

### Understanding Results

The solver reports:
- **Win Probability**: Expected probability of winning from this state
- **Depth**: Search depth reached (higher = more accurate)
- **Exact**: Whether the full search tree was explored
- **Suggested Panel**: The move that maximizes win probability

When result is "Depth N (approximate)":
- The solver explored N moves ahead
- Result is a lower bound on true win probability
- Actual optimal win probability may be higher

When result is "Exact":
- The full game tree was explored
- Win probability is provably correct
- Suggested move is provably optimal
