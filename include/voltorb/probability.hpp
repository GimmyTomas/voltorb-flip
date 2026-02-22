#pragma once

#include "board.hpp"

#include <array>
#include <unordered_map>
#include <vector>

namespace voltorb {

// Panel probability distribution
struct PanelProbabilities {
    Position pos;
    double pVoltorb;
    double pOne;
    double pTwo;
    double pThree;

    double pMultiplier() const { return pTwo + pThree; }
    double pSafe() const { return 1.0 - pVoltorb; }
};

// Complete probability state for a board
struct BoardProbabilities {
    std::vector<PanelProbabilities> panels; // Only unknown panels
    std::array<double, NUM_TYPES_PER_LEVEL> typeProbs; // P(type | board)
    std::array<size_t, NUM_TYPES_PER_LEVEL> compatibleCounts; // # compatible per type
    size_t totalCompatible;
};

// Calculates probabilities using Bayesian inference
class ProbabilityCalculator {
public:
    // Calculate probabilities for all unknown panels
    static BoardProbabilities calculate(const Board& board,
                                        const std::vector<Board>& compatibleBoards);

    // Calculate probabilities from pre-computed compatible board indices per type
    static BoardProbabilities calculateFromIndices(
        const Board& board,
        const std::array<std::vector<size_t>, NUM_TYPES_PER_LEVEL>& indicesPerType,
        const std::vector<Board>& allBoards);

    // Check if a panel is guaranteed safe (non-voltorb in all compatible boards)
    static bool isPanelGuaranteedSafe(const Board& board, Position pos,
                                      const std::vector<Board>& compatibleBoards);

    // Find all guaranteed safe panels
    static std::vector<Position> findGuaranteedSafePanels(
        const Board& board, const std::vector<Board>& compatibleBoards);

    // Find panel guaranteed to be a specific value (useful for forced moves)
    static std::optional<Position> findForcedPanel(const Board& board,
                                                   const std::vector<Board>& compatibleBoards);

private:
    // Calculate P(type | board) using Bayes' theorem
    static std::array<double, NUM_TYPES_PER_LEVEL>
    calculateTypeProbabilities(const Board& board,
                               const std::array<size_t, NUM_TYPES_PER_LEVEL>& countsPerType);
};

} // namespace voltorb
