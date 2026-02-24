# Legacy vs New Solver: Compatible Board Count Discrepancy

> **STATUS: FIXED** - The bug described in this document has been fixed. The legacy solver's
> `find_and_fill_row_with_possibilities` function was replaced with an exhaustive recursive
> backtracking algorithm. Both solvers now produce identical compatible board counts.

## Executive Summary

The legacy solver **used to miss valid boards** that the new solver correctly finds. This document analyzes a specific case: **Level 2, Board 4, Type 0** where the legacy solver previously found 31 boards and the new solver found 34.

**After the fix:** Both solvers now find exactly 34 boards for this case, and all compatible board counts match exactly across all levels and board types.

---

## The Board

```
         Col:  0  1  2  3  4
              +---------------+
       Row 0: |  ?  ?  ?  ?  ? | sum=6, voltorbs=1
       Row 1: |  ?  ?  ?  ?  ? | sum=5, voltorbs=0  ← FREE ROW
       Row 2: |  ?  ?  ?  ?  ? | sum=5, voltorbs=1
       Row 3: |  ?  ?  ?  ?  ? | sum=4, voltorbs=3
       Row 4: |  ?  ?  ?  ?  ? | sum=5, voltorbs=2
              +---------------+
         sum:    6  2  7  7  3
    voltorbs:    1  3  0  1  2
                       ↑
                  FREE COLUMN
```

**Total voltorbs:** 1+0+1+3+2 = 7
**Total sum:** 6+5+5+4+5 = 25

## Type 0 Parameters (Level 2)

| Parameter | Value |
|-----------|-------|
| n0 (voltorbs) | 7 |
| n1 (ones) | 14 |
| n2 (twos) | 1 |
| n3 (threes) | 3 |
| maxTotalFree | 3 |
| maxRowFree | 2 |
| totalSum | 25 |

The board is **compatible with Type 0**: voltorbs match (7=7), sum matches (25=25).

---

## The Missing Boards

The new solver finds **3 boards** that the legacy solver misses:

### Missing Board 1: `1113011111101210031030101`
```
  1 1 1 3 0   Row 0: sum=6 ✓, voltorbs=1 ✓
  1 1 1 1 1   Row 1: sum=5 ✓, voltorbs=0 ✓
  1 0 1 2 1   Row 2: sum=5 ✓, voltorbs=1 ✓
  0 0 3 1 0   Row 3: sum=4 ✓, voltorbs=3 ✓
  3 0 1 0 1   Row 4: sum=5 ✓, voltorbs=2 ✓
  ─────────
  6 2 7 7 3   Column sums ✓
  1 3 0 1 2   Column voltorbs ✓
```
**Counts:** n0=7 ✓, n1=14 ✓, n2=1 ✓, n3=3 ✓
**Both isLegal() and compatible_type() return PASS**

### Missing Board 2: `1131011111101210013030101`
```
  1 1 3 1 0   Row 0: sum=6 ✓, voltorbs=1 ✓
  1 1 1 1 1   Row 1: sum=5 ✓, voltorbs=0 ✓
  1 0 1 2 1   Row 2: sum=5 ✓, voltorbs=1 ✓
  0 0 1 3 0   Row 3: sum=4 ✓, voltorbs=3 ✓
  3 0 1 0 1   Row 4: sum=5 ✓, voltorbs=2 ✓
  ─────────
  6 2 7 7 3   Column sums ✓
  1 3 0 1 2   Column voltorbs ✓
```
**Counts:** n0=7 ✓, n1=14 ✓, n2=1 ✓, n3=3 ✓
**Both isLegal() and compatible_type() return PASS**

### Missing Board 3: `3111011111101210013010301`
```
  3 1 1 1 0   Row 0: sum=6 ✓, voltorbs=1 ✓
  1 1 1 1 1   Row 1: sum=5 ✓, voltorbs=0 ✓
  1 0 1 2 1   Row 2: sum=5 ✓, voltorbs=1 ✓
  0 0 1 3 0   Row 3: sum=4 ✓, voltorbs=3 ✓
  1 0 3 0 1   Row 4: sum=5 ✓, voltorbs=2 ✓
  ─────────
  6 2 7 7 3   Column sums ✓
  1 3 0 1 2   Column voltorbs ✓
```
**Counts:** n0=7 ✓, n1=14 ✓, n2=1 ✓, n3=3 ✓
**Both isLegal() and compatible_type() return PASS**

---

## All 34 Compatible Boards (New Solver)

```
 #  Board                      Notes
─────────────────────────────────────────────────────
 1. 0113111111101211030030110
 2. 0113111111101213010010310
 3. 0131111111101213010010130
 4. 1013111111011211030030110
 5. 1013111111011213010010310
 6. 1013111111101210031031100
 7. 1013111111101210130030110
 8. 1013111111101213010001310
 9. 1013111111111200030130110
10. 1013111111111200031030101
11. 1013111111111203010000311
12. 1031111111011213010010130
13. 1031111111101210013031100
14. 1031111111101213010001130
15. 1031111111111200013030101
16. 1031111111111203010000131
17. 1113011111101210030130110
18. 1113011111101210031030101  ← MISSING FROM LEGACY
19. 1113011111101213010000311
20. 1130111111101210013030110
21. 1131011111101210013030101  ← MISSING FROM LEGACY
22. 1131011111101213010000131
23. 3011111111011211030010130
24. 3011111111101210013011300
25. 3011111111101210130010130
26. 3011111111101211030001130
27. 3011111111111200013010301
28. 3011111111111200030110130
29. 3011111111111201030000131
30. 3110111111101210013010310
31. 3110111111101210031010130
32. 3111011111101210013010301  ← MISSING FROM LEGACY
33. 3111011111101210030110130
34. 3111011111101211030000131
```

---

## Why Does the Legacy Solver Fail?

### The Legacy Algorithm

The function `find_and_fill_row_with_possibilities` works as follows:

```
1. Find the row or column with the fewest "combinations" to fill
2. For that row/column, enumerate all valid (2,3) placements:
   - Generate all ways to place the needed 2s
   - For each 2-placement, generate all ways to place 3s
   - Fill remaining cells with 1s
3. Check if result passes compatible_type()
4. If yes, add to the list
5. Erase the original partial board
6. Repeat until all boards are fully filled
```

### The Bug: Local Greedy Choice

The algorithm makes a **greedy local choice** when selecting which row/column to fill first. It picks the one with fewest combinations. But this choice affects what combinations are explored later.

#### Example Trace

Consider the voltorb configuration that could lead to missing boards:
```
  ? ? ? ? 0   (row 0: 1 voltorb at position 4)
  ? ? ? ? ?   (row 1: 0 voltorbs - FREE)
  ? 0 ? ? ?   (row 2: 1 voltorb at position 1)
  0 0 ? ? 0   (row 3: 3 voltorbs at positions 0,1,4)
  ? 0 ? 0 ?   (row 4: 2 voltorbs at positions 1,3)
```

When filling this, the algorithm might:

1. **Choose Row 3 first** (only 2 unknown cells, sum=4 needed)
   - Must place values summing to 4 in cells (3,2) and (3,3)
   - Options: 1+3, 2+2, 3+1
   - For Type 0 (needs 1 two, 3 threes): valid combos are 1+3 or 3+1
   - Generates: `row3 = 0 0 1 3 0` and `row3 = 0 0 3 1 0`

2. **For each Row 3 variant, choose next row/column**

The problem is that when the algorithm commits to a particular row 3 configuration early, it may not explore all valid combinations for the other rows that would have been compatible.

### Specific Failure Case

Compare these two boards:

**Board 17 (FOUND):** `1113011111101210030130110`
```
  1 1 1 3 0
  1 1 1 1 1
  1 0 1 2 1
  0 0 3 0 1  ← Row 3 has 3 at position 2
  3 0 1 1 0  ← Row 4 has 3 at position 0
```

**Board 18 (MISSING):** `1113011111101210031030101`
```
  1 1 1 3 0
  1 1 1 1 1
  1 0 1 2 1
  0 0 3 1 0  ← Row 3 has 3 at position 2 (same), but 1 moved from pos 4 to pos 3
  3 0 1 0 1  ← Row 4 has 3 at position 0 (same), voltorb moved from pos 3 to pos 4
```

The difference is subtle: in rows 3 and 4, the positions of the 1s and voltorbs are swapped between positions 3 and 4.

### Root Cause: Order-Dependent Enumeration

The legacy algorithm's enumeration is **order-dependent**:

1. When it fills row 3 with `0 0 3 0 1`, it has placed the 1 at position (3,4)
2. The column constraints then require specific values in column 4
3. When it later fills row 4, the constraint from column 4 may preclude certain arrangements

But if it had chosen `0 0 3 1 0` for row 3 instead:
1. The 1 is at position (3,3)
2. Column 4 now needs a different value at position (4,4)
3. Row 4 can have different valid arrangements

The key insight: **the order of filling rows/columns affects which complete boards are discovered**, and the greedy selection doesn't guarantee all valid boards are found.

### Why the New Solver Works

The new solver uses **exhaustive recursive backtracking**:

```
For each cell from (0,0) to (4,4):
    For each possible value (1, 2, 3):
        Place value
        If constraints still satisfiable:
            Recurse to next cell
        Backtrack
```

This explores **all** valid combinations systematically, regardless of order. It doesn't make greedy choices that could prune valid solutions.

---

## Detailed Investigation

### Voltorb Configuration Analysis

Running the legacy algorithm with each of the 13 voltorb configurations **individually**:

```
Config  0:  3 boards
Config  1:  4 boards
Config  2:  0 boards
Config  3:  2 boards
Config  4:  0 boards
Config  5:  0 boards
Config  6:  2 boards
Config  7:  0 boards
Config  8:  0 boards
Config  9:  3 boards
Config 10:  2 boards
Config 11:  0 boards [TARGET CONFIG - produces the 3 missing boards with new solver]
Config 12:  0 boards

Total from individual runs: 16 boards
```

### Critical Finding: Joint Run Produces More Boards!

When running **all 13 configs together**: 31 boards (15 more than individual runs!)

This reveals that the legacy algorithm has **non-deterministic behavior** based on processing order. When configs are processed together, some partial boards from one config can "merge" with partial boards from another config, producing boards with **different voltorb patterns** than any input config.

### Why Config 11 Fails

Config 11's voltorb pattern:
```
  . . . . V   Row 0: voltorb at col 4
  . . . . .   Row 1: no voltorbs (FREE ROW)
  . V . . .   Row 2: voltorb at col 1
  V V . . V   Row 3: voltorbs at cols 0, 1, 4
  . V . V .   Row 4: voltorbs at cols 1, 3
```

This creates:
- **Free row**: Row 1 (must be all 1s since sum=5 in 5 cells)
- **Free column**: Col 2 (must have exactly one 3 + four 1s due to sum=7 and maxRowFree constraint)

I manually verified that valid type 0 boards exist with this pattern. For example:
- (3,2)=3 (the required 3 in free col 2)
- Row 0: one 3 + three 1s (sum=6)
- Row 4: one 3 + two 1s (sum=5 with one 1 in col 2)

Yet the legacy algorithm fails to find these configurations because its filling order prevents exploring certain valid combinations.

---

## Conclusion

The legacy solver's `find_and_fill_row_with_possibilities` algorithm uses a greedy row/column selection strategy that causes it to miss some valid board configurations. The bug is not in the validation logic (both `isLegal()` and `compatible_type()` correctly accept the missing boards), but in the **enumeration strategy** that fails to explore all valid combinations.

The new solver's exhaustive backtracking approach correctly finds all 34 valid boards.

---

## Fix Applied

The bug was fixed by replacing `find_and_fill_row_with_possibilities` with a new function `fill_non_voltorbs_recursive` that uses exhaustive recursive backtracking:

```cpp
static void fill_non_voltorbs_recursive(Board& current, int pos, int remaining2s, int remaining3s,
                                        std::vector<Board>& result, uint8_t type) {
    // Find next unknown cell
    // If no more: validate and add to result
    // Otherwise: try placing 1, 2 (if remaining), or 3 (if remaining)
    // Prune early if constraints violated
    // Backtrack by resetting cell to -1
}
```

### Verification Results

After the fix:
- **Compatible board counts**: Match exactly between legacy and new solvers
- **Panel probabilities**: Match within 0.1% (minor differences due to different board type weighting formulas)
- **All tests pass**: 63,110 assertions across 17 test cases
