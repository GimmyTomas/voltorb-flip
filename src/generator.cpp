#include "voltorb/generator.hpp"

#include "voltorb/board_type.hpp"

#include <algorithm>
#include <numeric>

namespace voltorb {

BoardGenerator::BoardGenerator(uint64_t seed) : rng_(seed) {
    if (seed == 0) {
        std::random_device rd;
        rng_.seed(rd());
    }
}

void BoardGenerator::seed(uint64_t seed) {
    rng_.seed(seed);
}

bool BoardGenerator::isLegal(const Board& board, const BoardTypeParams& params) const {
    const auto& rowHints = board.rowHints();
    const auto& colHints = board.colHints();

    // Count multipliers in free rows/columns
    int totalFreeMultipliers = 0;

    for (size_t i = 0; i < BOARD_SIZE; i++) {
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            PanelValue v = board.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j));
            if (isMultiplier(v)) {
                if (rowHints[i].voltorbCount == 0 || colHints[j].voltorbCount == 0) {
                    totalFreeMultipliers++;
                }
            }
        }
    }

    if (totalFreeMultipliers >= params.maxTotalFree) {
        return false;
    }

    // Check per-row constraints
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

    // Check per-column constraints
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

Board BoardGenerator::generate(Level level) {
    std::uniform_int_distribution<int> typeDist(0, NUM_TYPES_PER_LEVEL - 1);
    BoardTypeIndex type = static_cast<BoardTypeIndex>(typeDist(rng_));
    return generate(level, type);
}

Board BoardGenerator::generate(Level level, BoardTypeIndex type) {
    const auto& params = BoardTypeData::params(level, type);

    lastRejectionCount_ = 0;

    while (true) {
        Board board(level);

        // Initialize all panels to 1
        for (size_t i = 0; i < BOARD_SIZE; i++) {
            for (size_t j = 0; j < BOARD_SIZE; j++) {
                board.set(static_cast<uint8_t>(i), static_cast<uint8_t>(j), PanelValue::One);
            }
        }

        // Create shuffled position array
        std::array<size_t, TOTAL_PANELS> positions;
        std::iota(positions.begin(), positions.end(), 0);
        std::shuffle(positions.begin(), positions.end(), rng_);

        size_t idx = 0;

        // Place voltorbs
        for (int n = 0; n < params.n0; n++) {
            Position pos = Position::fromIndex(positions[idx++]);
            board.set(pos, PanelValue::Voltorb);
        }

        // Place 2s
        for (int n = 0; n < params.n2; n++) {
            Position pos = Position::fromIndex(positions[idx++]);
            board.set(pos, PanelValue::Two);
        }

        // Place 3s
        for (int n = 0; n < params.n3; n++) {
            Position pos = Position::fromIndex(positions[idx++]);
            board.set(pos, PanelValue::Three);
        }

        // Calculate hints
        board.recalculateHints();

        // Check legality
        if (isLegal(board, params)) {
            return board;
        }

        lastRejectionCount_++;
    }
}

} // namespace voltorb
