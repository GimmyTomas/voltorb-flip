#include "voltorb/board.hpp"

#include <sstream>

namespace voltorb {

Board::Board() : level_(1) {
    for (auto& row : panels_) {
        for (auto& panel : row) {
            panel = PanelValue::Unknown;
        }
    }
    for (auto& hint : rowHints_) {
        hint = {0, 0};
    }
    for (auto& hint : colHints_) {
        hint = {0, 0};
    }
}

Board::Board(Level level) : Board() {
    level_ = level;
}

PanelValue Board::get(Position pos) const {
    return panels_[pos.row][pos.col];
}

PanelValue Board::get(uint8_t row, uint8_t col) const {
    return panels_[row][col];
}

void Board::set(Position pos, PanelValue value) {
    panels_[pos.row][pos.col] = value;
}

void Board::set(uint8_t row, uint8_t col, PanelValue value) {
    panels_[row][col] = value;
}

void Board::setRowHint(uint8_t row, LineHint hint) {
    rowHints_[row] = hint;
}

void Board::setColHint(uint8_t col, LineHint hint) {
    colHints_[col] = hint;
}

bool Board::isFullyRevealed() const {
    for (const auto& row : panels_) {
        for (const auto& panel : row) {
            if (panel == PanelValue::Unknown) {
                return false;
            }
        }
    }
    return true;
}

bool Board::hasVoltorbFlipped() const {
    for (const auto& row : panels_) {
        for (const auto& panel : row) {
            if (panel == PanelValue::Voltorb) {
                return true;
            }
        }
    }
    return false;
}

GameResult Board::checkGameResult() const {
    if (hasVoltorbFlipped()) {
        return GameResult::Lost;
    }

    // Count revealed multipliers and required multipliers
    size_t revealedMult = countMultipliersRevealed();
    size_t requiredMult = totalMultipliersRequired();

    if (revealedMult >= requiredMult) {
        return GameResult::Won;
    }

    return GameResult::InProgress;
}

size_t Board::countUnknown() const {
    size_t count = 0;
    for (const auto& row : panels_) {
        for (const auto& panel : row) {
            if (panel == PanelValue::Unknown) {
                count++;
            }
        }
    }
    return count;
}

size_t Board::countKnown() const {
    return TOTAL_PANELS - countUnknown();
}

size_t Board::countMultipliersRevealed() const {
    size_t count = 0;
    for (const auto& row : panels_) {
        for (const auto& panel : row) {
            if (isMultiplier(panel)) {
                count++;
            }
        }
    }
    return count;
}

size_t Board::totalMultipliersRequired() const {
    // Calculate from hints: total_sum = n1 + 2*n2 + 3*n3
    // total_voltorbs = n0
    // n0 + n1 + n2 + n3 = 25
    //
    // We want to return the MINIMUM number of multipliers that could exist,
    // so we can declare a win as soon as we've revealed at least that many.
    //
    // Given: extra = n2 + 2*n3 (extra points beyond 1 per non-voltorb)
    // total = n2 + n3 (total multipliers)
    //
    // Minimum multipliers: when all are 3s -> n3 = extra/2, total = extra/2
    // Maximum multipliers: when all are 2s -> n2 = extra, total = extra
    //
    // For the win condition, we use ceil(extra/2) as the minimum.

    int totalSum = 0;
    int totalVoltorbs = 0;
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        totalSum += rowHints_[i].sum;
        totalVoltorbs += rowHints_[i].voltorbCount;
    }

    // extra = total_sum - (25 - n0) = total_sum - 25 + n0
    int extra = totalSum - static_cast<int>(TOTAL_PANELS) + totalVoltorbs;

    if (extra <= 0) {
        return 0;  // No multipliers needed (all panels are 1s or voltorbs)
    }

    // Minimum multipliers = ceil(extra / 2)
    // This assumes the best case where all multipliers are 3s.
    // We use this as the threshold for declaring a win.
    return static_cast<size_t>((extra + 1) / 2);
}

Board Board::withPanelRevealed(Position pos, PanelValue value) const {
    Board copy = *this;
    copy.set(pos, value);
    return copy;
}

bool Board::operator==(const Board& other) const {
    if (level_ != other.level_) return false;
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        if (rowHints_[i] != other.rowHints_[i]) return false;
        if (colHints_[i] != other.colHints_[i]) return false;
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            if (panels_[i][j] != other.panels_[i][j]) return false;
        }
    }
    return true;
}

size_t Board::hash() const {
    // FNV-1a hash
    size_t hash = 14695981039346656037ULL;
    constexpr size_t prime = 1099511628211ULL;

    hash ^= static_cast<size_t>(level_);
    hash *= prime;

    for (size_t i = 0; i < BOARD_SIZE; i++) {
        hash ^= static_cast<size_t>(rowHints_[i].sum);
        hash *= prime;
        hash ^= static_cast<size_t>(rowHints_[i].voltorbCount);
        hash *= prime;
        hash ^= static_cast<size_t>(colHints_[i].sum);
        hash *= prime;
        hash ^= static_cast<size_t>(colHints_[i].voltorbCount);
        hash *= prime;

        for (size_t j = 0; j < BOARD_SIZE; j++) {
            hash ^= static_cast<size_t>(static_cast<int8_t>(panels_[i][j]) + 2);
            hash *= prime;
        }
    }

    return hash;
}

std::string Board::toString(std::optional<Position> highlight) const {
    std::ostringstream ss;

    ss << "       Level " << static_cast<int>(level_) << "\n";

    for (size_t i = 0; i < BOARD_SIZE; i++) {
        ss << "---------------------\n";
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            if (highlight && highlight->row == i && highlight->col == j) {
                ss << "|###";
            } else {
                PanelValue v = panels_[i][j];
                if (v == PanelValue::Unknown) {
                    ss << "|   ";
                } else {
                    ss << "| " << toInt(v) << " ";
                }
            }
        }
        ss << "| " << static_cast<int>(rowHints_[i].sum) << ","
           << static_cast<int>(rowHints_[i].voltorbCount) << "\n";
    }
    ss << "---------------------\n";

    // Column sums
    for (size_t j = 0; j < BOARD_SIZE; j++) {
        int sum = colHints_[j].sum;
        if (sum < 10) {
            ss << "  " << sum << " ";
        } else {
            ss << " " << sum << " ";
        }
    }
    ss << "\n";

    // Column voltorb counts
    for (size_t j = 0; j < BOARD_SIZE; j++) {
        ss << "  " << static_cast<int>(colHints_[j].voltorbCount) << " ";
    }
    ss << "\n";

    return ss.str();
}

void Board::recalculateHints() {
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        uint8_t rowSum = 0;
        uint8_t rowVoltorbs = 0;
        uint8_t colSum = 0;
        uint8_t colVoltorbs = 0;

        for (size_t j = 0; j < BOARD_SIZE; j++) {
            // Row hints
            if (panels_[i][j] != PanelValue::Unknown) {
                int val = toInt(panels_[i][j]);
                if (val >= 0) rowSum += static_cast<uint8_t>(val);
                if (panels_[i][j] == PanelValue::Voltorb) rowVoltorbs++;
            }

            // Column hints
            if (panels_[j][i] != PanelValue::Unknown) {
                int val = toInt(panels_[j][i]);
                if (val >= 0) colSum += static_cast<uint8_t>(val);
                if (panels_[j][i] == PanelValue::Voltorb) colVoltorbs++;
            }
        }

        rowHints_[i] = {rowSum, rowVoltorbs};
        colHints_[i] = {colSum, colVoltorbs};
    }
}

Board Board::toCovered() const {
    Board covered = *this;
    for (auto& row : covered.panels_) {
        for (auto& panel : row) {
            panel = PanelValue::Unknown;
        }
    }
    return covered;
}

} // namespace voltorb
