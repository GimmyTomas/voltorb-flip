#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace voltorb {

// Panel values: 0 (Voltorb), 1, 2, 3, or Unknown (-1)
enum class PanelValue : int8_t {
    Unknown = -1,
    Voltorb = 0,
    One = 1,
    Two = 2,
    Three = 3
};

constexpr bool isMultiplier(PanelValue v) {
    return v == PanelValue::Two || v == PanelValue::Three;
}

constexpr bool isKnown(PanelValue v) {
    return v != PanelValue::Unknown;
}

constexpr int toInt(PanelValue v) {
    return static_cast<int>(v);
}

// Position on the 5x5 board
struct Position {
    uint8_t row;
    uint8_t col;

    constexpr bool operator==(const Position& other) const {
        return row == other.row && col == other.col;
    }

    constexpr bool operator!=(const Position& other) const {
        return !(*this == other);
    }

    constexpr size_t toIndex() const {
        return static_cast<size_t>(row * 5 + col);
    }

    static constexpr Position fromIndex(size_t idx) {
        return {static_cast<uint8_t>(idx / 5), static_cast<uint8_t>(idx % 5)};
    }
};

// Hint for a row or column: (sum of values, count of voltorbs)
struct LineHint {
    uint8_t sum;
    uint8_t voltorbCount;

    constexpr bool operator==(const LineHint& other) const {
        return sum == other.sum && voltorbCount == other.voltorbCount;
    }
};

// Game result after a move or at game end
enum class GameResult {
    InProgress, // Game still ongoing
    Won,        // All multipliers flipped
    Lost        // Flipped a voltorb
};

// Board level (1-8)
using Level = uint8_t;
constexpr Level MIN_LEVEL = 1;
constexpr Level MAX_LEVEL = 8;

// Board type within a level (0-9)
using BoardTypeIndex = uint8_t;
constexpr BoardTypeIndex NUM_TYPES_PER_LEVEL = 10;

// Constants
constexpr size_t BOARD_SIZE = 5;
constexpr size_t TOTAL_PANELS = BOARD_SIZE * BOARD_SIZE;

// Array types
using PanelGrid = std::array<std::array<PanelValue, BOARD_SIZE>, BOARD_SIZE>;
using HintArray = std::array<LineHint, BOARD_SIZE>;

} // namespace voltorb
