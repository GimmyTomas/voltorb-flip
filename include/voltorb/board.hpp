#pragma once

#include "types.hpp"

#include <functional>
#include <string>

namespace voltorb {

// Represents a Voltorb Flip board state (can be fully revealed or partially covered)
class Board {
public:
    Board();
    explicit Board(Level level);

    // Panel access
    PanelValue get(Position pos) const;
    PanelValue get(uint8_t row, uint8_t col) const;
    void set(Position pos, PanelValue value);
    void set(uint8_t row, uint8_t col, PanelValue value);

    // Hints
    const HintArray& rowHints() const { return rowHints_; }
    const HintArray& colHints() const { return colHints_; }
    LineHint rowHint(uint8_t row) const { return rowHints_[row]; }
    LineHint colHint(uint8_t col) const { return colHints_[col]; }
    void setRowHint(uint8_t row, LineHint hint);
    void setColHint(uint8_t col, LineHint hint);

    // Level
    Level level() const { return level_; }
    void setLevel(Level level) { level_ = level; }

    // Board state queries
    bool isFullyRevealed() const;
    bool hasVoltorbFlipped() const;
    GameResult checkGameResult() const;

    // Count methods
    size_t countUnknown() const;
    size_t countKnown() const;
    size_t countMultipliersRevealed() const;
    size_t totalMultipliersRequired() const; // Based on hints

    // Panel grid access
    const PanelGrid& panels() const { return panels_; }

    // Create a copy with a panel revealed
    Board withPanelRevealed(Position pos, PanelValue value) const;

    // Comparison
    bool operator==(const Board& other) const;
    bool operator!=(const Board& other) const { return !(*this == other); }

    // Hashing for memoization
    size_t hash() const;

    // Debug/display
    std::string toString(std::optional<Position> highlight = std::nullopt) const;

    // Recalculate hints from fully revealed panel state
    void recalculateHints();

    // Cover all panels (set to Unknown while keeping hints)
    Board toCovered() const;

private:
    PanelGrid panels_;
    HintArray rowHints_;
    HintArray colHints_;
    Level level_;
};

} // namespace voltorb

// Hash specialization for use in unordered containers
namespace std {
template <>
struct hash<voltorb::Board> {
    size_t operator()(const voltorb::Board& board) const { return board.hash(); }
};
} // namespace std
