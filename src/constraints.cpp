#include "voltorb/constraints.hpp"

#include <algorithm>

namespace voltorb {

bool LegalityChecker::isLegal(const Board& board, const BoardTypeParams& params) {
    // Count multipliers in "free" rows/columns (those with 0 voltorbs)
    int totalFreeMultipliers = 0;

    const auto& rowHints = board.rowHints();
    const auto& colHints = board.colHints();

    for (size_t i = 0; i < BOARD_SIZE; i++) {
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            PanelValue v = board.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j));
            if (isMultiplier(v)) {
                // Check if this cell is in a free row or column
                if (rowHints[i].voltorbCount == 0 || colHints[j].voltorbCount == 0) {
                    totalFreeMultipliers++;
                }
            }
        }
    }

    if (totalFreeMultipliers >= params.maxTotalFree) {
        return false;
    }

    // Check per-row constraints for free rows
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        if (rowHints[i].voltorbCount == 0) {
            int rowMultipliers = 0;
            for (size_t j = 0; j < BOARD_SIZE; j++) {
                if (isMultiplier(board.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j)))) {
                    rowMultipliers++;
                }
            }
            if (rowMultipliers >= params.maxRowFree) {
                return false;
            }
        }
    }

    // Check per-column constraints for free columns
    for (size_t j = 0; j < BOARD_SIZE; j++) {
        if (colHints[j].voltorbCount == 0) {
            int colMultipliers = 0;
            for (size_t i = 0; i < BOARD_SIZE; i++) {
                if (isMultiplier(board.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j)))) {
                    colMultipliers++;
                }
            }
            if (colMultipliers >= params.maxRowFree) {
                return false;
            }
        }
    }

    return true;
}

bool LegalityChecker::isCompatibleWithType(const Board& board, Level level, BoardTypeIndex type) {
    const auto& params = BoardTypeData::params(level, type);

    // Count known values
    int minCount[4] = {0, 0, 0, 0};
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            PanelValue v = board.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j));
            if (isKnown(v) && toInt(v) >= 0) {
                minCount[toInt(v)]++;
            }
        }
    }

    // Check counts don't exceed type limits
    if (minCount[1] > params.n1()) return false;
    if (minCount[2] > params.n2) return false;
    if (minCount[3] > params.n3) return false;

    // Check legality constraints
    if (!isLegal(board, params)) return false;

    // Check hint compatibility
    const auto& rowHints = board.rowHints();
    int totalVoltorbs = 0;
    int totalSum = 0;
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        totalVoltorbs += rowHints[i].voltorbCount;
        totalSum += rowHints[i].sum;
    }

    if (totalVoltorbs != params.n0) return false;
    if (totalSum != params.totalSum()) return false;

    return true;
}

bool LegalityChecker::panelsDontExceedConstraints(const Board& board) {
    const auto& rowHints = board.rowHints();
    const auto& colHints = board.colHints();

    // Check row constraints
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        int sum = 0;
        int sumAbs = 0;
        bool hasUnknown = false;

        for (size_t j = 0; j < BOARD_SIZE; j++) {
            PanelValue v = board.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j));
            if (v == PanelValue::Unknown) {
                hasUnknown = true;
            } else {
                int val = toInt(v);
                sum += val;
                sumAbs += (val >= 0 ? val : -val);
            }
        }

        if (sumAbs > rowHints[i].sum) return false;
        if (!hasUnknown && sum != rowHints[i].sum) return false;
    }

    // Check column constraints
    for (size_t j = 0; j < BOARD_SIZE; j++) {
        int sum = 0;
        int sumAbs = 0;
        bool hasUnknown = false;

        for (size_t i = 0; i < BOARD_SIZE; i++) {
            PanelValue v = board.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j));
            if (v == PanelValue::Unknown) {
                hasUnknown = true;
            } else {
                int val = toInt(v);
                sum += val;
                sumAbs += (val >= 0 ? val : -val);
            }
        }

        if (sumAbs > colHints[j].sum) return false;
        if (!hasUnknown && sum != colHints[j].sum) return false;
    }

    return true;
}

Board VoltorbPositions::toTemplate(Level level, const HintArray& rowHints,
                                   const HintArray& colHints) const {
    Board board(level);
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        board.setRowHint(static_cast<uint8_t>(i), rowHints[i]);
        board.setColHint(static_cast<uint8_t>(i), colHints[i]);
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            if (isVoltorb[i][j]) {
                board.set(static_cast<uint8_t>(i), static_cast<uint8_t>(j), PanelValue::Voltorb);
            } else {
                board.set(static_cast<uint8_t>(i), static_cast<uint8_t>(j), PanelValue::Unknown);
            }
        }
    }
    return board;
}

bool VoltorbPositionGenerator::validForColumns(const Board& board, const VoltorbPositions& current,
                                               uint8_t row,
                                               const std::array<uint8_t, BOARD_SIZE>& colCounts) {
    const auto& colHints = board.colHints();

    for (size_t j = 0; j < BOARD_SIZE; j++) {
        // Too many voltorbs in this column already?
        if (colCounts[j] > colHints[j].voltorbCount) {
            return false;
        }

        // If we're past the last row, check exact match
        if (row == BOARD_SIZE - 1 && colCounts[j] != colHints[j].voltorbCount) {
            return false;
        }

        // Can we still fit remaining voltorbs?
        uint8_t remainingRows = static_cast<uint8_t>(BOARD_SIZE - 1 - row);
        if (colCounts[j] + remainingRows < colHints[j].voltorbCount) {
            return false;
        }
    }

    return true;
}

void VoltorbPositionGenerator::generateRecursive(const Board& board, VoltorbPositions& current,
                                                 uint8_t row,
                                                 std::array<uint8_t, BOARD_SIZE>& colCounts,
                                                 const Callback& callback, size_t& count) {
    if (row == BOARD_SIZE) {
        // Check final column counts match exactly
        const auto& colHints = board.colHints();
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            if (colCounts[j] != colHints[j].voltorbCount) {
                return;
            }
        }

        // Check for conflicts with revealed panels
        for (size_t i = 0; i < BOARD_SIZE; i++) {
            for (size_t j = 0; j < BOARD_SIZE; j++) {
                PanelValue revealed = board.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j));
                if (isKnown(revealed)) {
                    bool shouldBeVoltorb = current.isVoltorb[i][j];
                    bool isVoltorb = (revealed == PanelValue::Voltorb);
                    if (shouldBeVoltorb != isVoltorb) {
                        return;
                    }
                }
            }
        }

        callback(current);
        count++;
        return;
    }

    const auto& rowHints = board.rowHints();
    uint8_t voltorbsNeeded = rowHints[row].voltorbCount;

    // Generate all combinations of voltorbsNeeded positions in this row
    // Using bit manipulation for efficiency
    std::string positions(BOARD_SIZE, '0');
    std::fill(positions.begin(), positions.begin() + voltorbsNeeded, '1');

    do {
        // Set voltorb positions for this row
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            current.isVoltorb[row][j] = (positions[j] == '1');
            if (current.isVoltorb[row][j]) {
                colCounts[j]++;
            }
        }

        // Check if this is valid for column constraints so far
        if (validForColumns(board, current, row, colCounts)) {
            generateRecursive(board, current, static_cast<uint8_t>(row + 1), colCounts, callback,
                              count);
        }

        // Reset column counts for this row
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            if (current.isVoltorb[row][j]) {
                colCounts[j]--;
            }
        }

    } while (std::prev_permutation(positions.begin(), positions.end()));
}

size_t VoltorbPositionGenerator::generate(const Board& board, const Callback& callback) {
    VoltorbPositions current{};
    std::array<uint8_t, BOARD_SIZE> colCounts{};
    size_t count = 0;

    generateRecursive(board, current, 0, colCounts, callback, count);

    return count;
}

std::vector<VoltorbPositions> VoltorbPositionGenerator::generateAll(const Board& board) {
    std::vector<VoltorbPositions> result;
    generate(board, [&result](const VoltorbPositions& pos) { result.push_back(pos); });
    return result;
}

void CompatibleBoardGenerator::fillNonVoltorbs(const Board& board, const VoltorbPositions& voltorbs,
                                               BoardTypeIndex type, const Callback& callback,
                                               size_t& count) {
    Level level = board.level();
    const auto& params = BoardTypeData::params(level, type);

    // Create template board with voltorbs filled in
    Board current = voltorbs.toTemplate(level, board.rowHints(), board.colHints());

    // Copy any revealed panels from original board
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            PanelValue revealed = board.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j));
            if (isKnown(revealed) && revealed != PanelValue::Voltorb) {
                current.set(static_cast<uint8_t>(i), static_cast<uint8_t>(j), revealed);
            }
        }
    }

    // Count already placed 2s and 3s
    int placed2s = 0, placed3s = 0;
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            PanelValue v = current.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j));
            if (v == PanelValue::Two) placed2s++;
            if (v == PanelValue::Three) placed3s++;
        }
    }

    int remaining2s = params.n2 - placed2s;
    int remaining3s = params.n3 - placed3s;

    if (remaining2s < 0 || remaining3s < 0) {
        return; // Incompatible with this type
    }

    fillRecursive(current, board, params, 0, 0, remaining2s, remaining3s, callback, count);
}

void CompatibleBoardGenerator::fillRecursive(Board& current, const Board& original,
                                             const BoardTypeParams& params, uint8_t row,
                                             uint8_t col, int remaining2s, int remaining3s,
                                             const Callback& callback, size_t& count) {
    // Find next unknown panel
    while (row < BOARD_SIZE) {
        while (col < BOARD_SIZE) {
            if (current.get(row, col) == PanelValue::Unknown) {
                break;
            }
            col++;
        }
        if (col < BOARD_SIZE) break;
        col = 0;
        row++;
    }

    if (row >= BOARD_SIZE) {
        // All panels filled - verify and emit
        if (LegalityChecker::panelsDontExceedConstraints(current) &&
            LegalityChecker::isLegal(current, params)) {
            if (!callback(current)) {
                // Callback returned false - stop enumeration
                return;
            }
            count++;
        }
        return;
    }

    // Count remaining unknown panels after this one
    int remainingUnknown = 0;
    for (size_t i = row; i < BOARD_SIZE; i++) {
        for (size_t j = (i == row ? col + 1 : 0); j < BOARD_SIZE; j++) {
            if (current.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j)) ==
                PanelValue::Unknown) {
                remainingUnknown++;
            }
        }
    }

    // Try placing a 1
    current.set(row, col, PanelValue::One);
    if (LegalityChecker::panelsDontExceedConstraints(current)) {
        fillRecursive(current, original, params, row, static_cast<uint8_t>(col + 1), remaining2s,
                      remaining3s, callback, count);
    }

    // Try placing a 2
    if (remaining2s > 0) {
        current.set(row, col, PanelValue::Two);
        if (LegalityChecker::panelsDontExceedConstraints(current)) {
            fillRecursive(current, original, params, row, static_cast<uint8_t>(col + 1),
                          remaining2s - 1, remaining3s, callback, count);
        }
    }

    // Try placing a 3
    if (remaining3s > 0) {
        current.set(row, col, PanelValue::Three);
        if (LegalityChecker::panelsDontExceedConstraints(current)) {
            fillRecursive(current, original, params, row, static_cast<uint8_t>(col + 1),
                          remaining2s, remaining3s - 1, callback, count);
        }
    }

    // Reset for backtracking
    current.set(row, col, PanelValue::Unknown);
}

size_t CompatibleBoardGenerator::generate(const Board& board, BoardTypeIndex type,
                                          const Callback& callback) {
    if (!LegalityChecker::isCompatibleWithType(board, board.level(), type)) {
        return 0;
    }

    size_t count = 0;

    VoltorbPositionGenerator::generate(
        board, [&](const VoltorbPositions& voltorbs) {
            fillNonVoltorbs(board, voltorbs, type, callback, count);
        });

    return count;
}

size_t CompatibleBoardGenerator::generateAllTypes(const Board& board, const Callback& callback) {
    size_t totalCount = 0;

    for (BoardTypeIndex type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
        totalCount += generate(board, type, callback);
    }

    return totalCount;
}

std::array<size_t, NUM_TYPES_PER_LEVEL> CompatibleBoardGenerator::countPerType(const Board& board) {
    std::array<size_t, NUM_TYPES_PER_LEVEL> counts{};

    for (BoardTypeIndex type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
        counts[type] = generate(board, type, [](const Board&) { return true; });
    }

    return counts;
}

} // namespace voltorb
