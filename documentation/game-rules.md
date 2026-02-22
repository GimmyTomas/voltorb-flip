# Voltorb Flip Game Rules

Voltorb Flip is a minigame from Pokémon HeartGold & SoulSilver (non-Japanese versions).

## Overview

- **Board**: 5×5 grid of covered panels
- **Panel values**: 0 (Voltorb), 1, 2, or 3
- **Goal**: Flip all multiplier panels (2s and 3s) without hitting a Voltorb

## Game Flow

1. A new board is generated based on the current level
2. Player sees row/column hints but panels are covered
3. Player flips panels one at a time
4. Game ends when:
   - **Win**: All multipliers (2s and 3s) are revealed
   - **Loss**: A Voltorb (0) is revealed

## Hints

Each row and column displays two numbers:
- **Sum**: Total of all panel values in that line
- **Voltorb count**: Number of 0s in that line

Example: `8,2` means the line has sum=8 with 2 Voltorbs.

## Coin Rewards

The player earns coins equal to the **product** of all flipped panel values:
- Flipping 1s doesn't change the score (×1)
- Flipping 2s doubles the score (×2)
- Flipping 3s triples the score (×3)

Players can quit early to keep accumulated coins.

## Levels (1-8)

Higher levels have:
- More Voltorbs
- More multipliers (2s and 3s)
- Higher potential rewards
- More difficult puzzles

### Level Progression

- **Win**: Advance to next level (max level 8)
- **Loss**: Drop to `min(panels_flipped, current_level)` or stay if equal

Special rule: Level 8 requires winning 5 games in a row with ≥8 panels flipped each.

## Board Types

Each level has 10 possible board types (0-9), each defining:
- `n_0`: Number of Voltorbs
- `n_2`: Number of 2s
- `n_3`: Number of 3s
- `max_total_free`: Max multipliers in "free" rows/columns combined
- `max_row_free`: Max multipliers in any single free row/column

A "free" row/column has 0 Voltorbs.

## Board Generation

Boards are generated using rejection sampling:
1. Randomly place Voltorbs, 2s, and 3s
2. Check legality constraints
3. If illegal, regenerate

This ensures multipliers aren't concentrated in obviously safe areas.

## Strategy Tips

1. **Always flip safe lines first** - Rows/columns with 0 Voltorbs
2. **Look for forced panels** - Some panels have deterministic values
3. **Use probability** - Calculate which panels are least likely to be Voltorbs
4. **Consider the minimax** - Optimal play maximizes expected win probability
