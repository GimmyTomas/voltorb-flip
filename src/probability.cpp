#include "voltorb/probability.hpp"

#include "voltorb/board_type.hpp"

namespace voltorb {

std::array<double, NUM_TYPES_PER_LEVEL>
ProbabilityCalculator::calculateTypeProbabilities(
    const Board& board, const std::array<size_t, NUM_TYPES_PER_LEVEL>& countsPerType) {

    std::array<double, NUM_TYPES_PER_LEVEL> probs{};
    Level level = board.level();

    // P(type | board) = P(board | type) * P(type) / P(board)
    // P(board | type) = countsPerType[type] / N_accepted[type]
    // P(type) = 1/10 (uniform prior)
    // P(board) = sum over types of P(board | type) * P(type)

    double pBoardSum = 0.0;
    for (BoardTypeIndex type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
        int64_t nAccepted = BoardTypeData::acceptedCount(level, type);
        if (nAccepted > 0) {
            double pBoardGivenType =
                static_cast<double>(countsPerType[type]) / static_cast<double>(nAccepted);
            probs[type] = pBoardGivenType; // Will normalize later
            pBoardSum += pBoardGivenType;
        }
    }

    // Normalize
    if (pBoardSum > 0) {
        for (auto& p : probs) {
            p /= pBoardSum;
        }
    }

    return probs;
}

BoardProbabilities ProbabilityCalculator::calculate(const Board& board,
                                                    const std::vector<Board>& compatibleBoards) {
    BoardProbabilities result;
    Level level = board.level();

    // Group compatible boards by type
    std::array<std::vector<size_t>, NUM_TYPES_PER_LEVEL> indicesPerType;

    // First, determine type of each compatible board by checking parameters
    for (size_t idx = 0; idx < compatibleBoards.size(); idx++) {
        const Board& cb = compatibleBoards[idx];

        // Count values in this board
        int count0 = 0, count2 = 0, count3 = 0;
        for (size_t i = 0; i < BOARD_SIZE; i++) {
            for (size_t j = 0; j < BOARD_SIZE; j++) {
                PanelValue v = cb.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j));
                if (v == PanelValue::Voltorb) count0++;
                else if (v == PanelValue::Two) count2++;
                else if (v == PanelValue::Three) count3++;
            }
        }

        // Find matching type
        for (BoardTypeIndex type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
            const auto& params = BoardTypeData::params(level, type);
            if (params.n0 == count0 && params.n2 == count2 && params.n3 == count3) {
                indicesPerType[type].push_back(idx);
                break;
            }
        }
    }

    return calculateFromIndices(board, indicesPerType, compatibleBoards);
}

BoardProbabilities ProbabilityCalculator::calculateFromIndices(
    const Board& board,
    const std::array<std::vector<size_t>, NUM_TYPES_PER_LEVEL>& indicesPerType,
    const std::vector<Board>& allBoards) {

    BoardProbabilities result;
    Level level = board.level();

    // Count per type
    result.totalCompatible = 0;
    for (BoardTypeIndex type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
        result.compatibleCounts[type] = indicesPerType[type].size();
        result.totalCompatible += indicesPerType[type].size();
    }

    if (result.totalCompatible == 0) {
        return result;
    }

    // Calculate type probabilities
    result.typeProbs = calculateTypeProbabilities(board, result.compatibleCounts);

    // Calculate P(board) normalization
    double pBoard = 0.0;
    for (BoardTypeIndex type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
        int64_t nAccepted = BoardTypeData::acceptedCount(level, type);
        if (nAccepted > 0) {
            pBoard += static_cast<double>(result.compatibleCounts[type]) /
                      static_cast<double>(nAccepted);
        }
    }

    // Find all unknown panels
    std::vector<Position> unknownPanels;
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            if (board.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j)) == PanelValue::Unknown) {
                unknownPanels.push_back({static_cast<uint8_t>(i), static_cast<uint8_t>(j)});
            }
        }
    }

    // Calculate probabilities for each unknown panel
    for (const Position& pos : unknownPanels) {
        PanelProbabilities panelProbs{pos, 0.0, 0.0, 0.0, 0.0};

        // For each value, calculate P(panel=value | board)
        for (int value = 0; value <= 3; value++) {
            double pValue = 0.0;

            for (BoardTypeIndex type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
                if (indicesPerType[type].empty()) continue;

                // Count boards of this type where panel has this value
                size_t countWithValue = 0;
                for (size_t idx : indicesPerType[type]) {
                    PanelValue v = allBoards[idx].get(pos);
                    if (toInt(v) == value) {
                        countWithValue++;
                    }
                }

                // P(panel=value | type, board) * P(type | board)
                double pValueGivenType = static_cast<double>(countWithValue) /
                                         static_cast<double>(indicesPerType[type].size());
                pValue += pValueGivenType * result.typeProbs[type];
            }

            switch (value) {
            case 0:
                panelProbs.pVoltorb = pValue;
                break;
            case 1:
                panelProbs.pOne = pValue;
                break;
            case 2:
                panelProbs.pTwo = pValue;
                break;
            case 3:
                panelProbs.pThree = pValue;
                break;
            }
        }

        result.panels.push_back(panelProbs);
    }

    return result;
}

bool ProbabilityCalculator::isPanelGuaranteedSafe(const Board& board, Position pos,
                                                  const std::vector<Board>& compatibleBoards) {
    for (const Board& cb : compatibleBoards) {
        if (cb.get(pos) == PanelValue::Voltorb) {
            return false;
        }
    }
    return true;
}

std::vector<Position>
ProbabilityCalculator::findGuaranteedSafePanels(const Board& board,
                                                const std::vector<Board>& compatibleBoards) {
    std::vector<Position> safePanels;

    for (size_t i = 0; i < BOARD_SIZE; i++) {
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            Position pos{static_cast<uint8_t>(i), static_cast<uint8_t>(j)};
            if (board.get(pos) == PanelValue::Unknown) {
                if (isPanelGuaranteedSafe(board, pos, compatibleBoards)) {
                    safePanels.push_back(pos);
                }
            }
        }
    }

    return safePanels;
}

std::optional<Position>
ProbabilityCalculator::findForcedPanel(const Board& board,
                                       const std::vector<Board>& compatibleBoards) {
    if (compatibleBoards.empty()) {
        return std::nullopt;
    }

    // Check if any panel has the same value in all compatible boards
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            Position pos{static_cast<uint8_t>(i), static_cast<uint8_t>(j)};
            if (board.get(pos) != PanelValue::Unknown) continue;

            PanelValue firstValue = compatibleBoards[0].get(pos);
            bool allSame = true;

            for (size_t k = 1; k < compatibleBoards.size(); k++) {
                if (compatibleBoards[k].get(pos) != firstValue) {
                    allSame = false;
                    break;
                }
            }

            if (allSame && firstValue != PanelValue::Voltorb) {
                return pos;
            }
        }
    }

    return std::nullopt;
}

} // namespace voltorb
