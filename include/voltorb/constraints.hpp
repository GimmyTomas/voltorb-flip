#pragma once

#include "board.hpp"
#include "board_type.hpp"

#include <functional>
#include <vector>

namespace voltorb {

// Checks legality of boards against board type constraints
class LegalityChecker {
public:
    // Check if a board configuration is legal for a given type
    static bool isLegal(const Board& board, const BoardTypeParams& params);

    // Check if a partially revealed board is compatible with a type
    // (could potentially be completed to a legal board of that type)
    static bool isCompatibleWithType(const Board& board, Level level, BoardTypeIndex type);

    // Check if panels exceed constraints (sum check, count check)
    static bool panelsDontExceedConstraints(const Board& board);
};

// Represents possible voltorb positions consistent with hints
struct VoltorbPositions {
    // For each cell, true if it must be a voltorb in this configuration
    std::array<std::array<bool, BOARD_SIZE>, BOARD_SIZE> isVoltorb;

    // Create board template from voltorb positions
    // Non-voltorb positions are set to Unknown
    Board toTemplate(Level level, const HintArray& rowHints, const HintArray& colHints) const;
};

// Generates all valid voltorb position configurations
class VoltorbPositionGenerator {
public:
    using Callback = std::function<void(const VoltorbPositions&)>;

    // Generate all valid voltorb positions for a board and call callback for each
    // Returns the count of valid configurations
    static size_t generate(const Board& board, const Callback& callback);

    // Generate and collect all valid voltorb positions
    static std::vector<VoltorbPositions> generateAll(const Board& board);

private:
    // Recursive generation with early pruning
    static void generateRecursive(const Board& board, VoltorbPositions& current, uint8_t row,
                                  std::array<uint8_t, BOARD_SIZE>& colCounts,
                                  const Callback& callback, size_t& count);

    // Check if current partial assignment is valid for column constraints
    static bool validForColumns(const Board& board, const VoltorbPositions& current, uint8_t row,
                                const std::array<uint8_t, BOARD_SIZE>& colCounts);
};

// Generates all complete boards compatible with a partial board state
class CompatibleBoardGenerator {
public:
    using Callback = std::function<bool(const Board&)>; // return false to stop

    // Generate all boards of a specific type compatible with the given board
    // Returns count of boards generated
    static size_t generate(const Board& board, BoardTypeIndex type, const Callback& callback);

    // Generate boards for all compatible types, weighted by acceptance counts
    static size_t generateAllTypes(const Board& board, const Callback& callback);

    // Count compatible boards per type without full enumeration (upper bound estimate)
    static std::array<size_t, NUM_TYPES_PER_LEVEL> countPerType(const Board& board);

private:
    // Fill in remaining panels for a given voltorb configuration
    static void fillNonVoltorbs(const Board& board, const VoltorbPositions& voltorbs,
                                BoardTypeIndex type, const Callback& callback, size_t& count);

    // Recursive filling with row/column constraint checking
    static void fillRecursive(Board& current, const Board& original, const BoardTypeParams& params,
                              uint8_t row, uint8_t col, int remaining2s, int remaining3s,
                              const Callback& callback, size_t& count);
};

} // namespace voltorb
