#include "voltorb/solver.hpp"

#include "voltorb/board_type.hpp"
#include "voltorb/constraints.hpp"
#include "voltorb/probability.hpp"

namespace voltorb {

Solver::Solver(SolverOptions options) : options_(std::move(options)) {}

void Solver::clearCache() {
    cache_.clear();
    cacheHits_ = 0;
    cacheMisses_ = 0;
}

bool Solver::checkTimeout() {
    if (timedOut_) return true;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime_);

    if (elapsed >= options_.timeout) {
        timedOut_ = true;
        return true;
    }

    return false;
}

SolverResult Solver::solve(const Board& board) {
    startTime_ = std::chrono::steady_clock::now();
    timedOut_ = false;
    clearCache();

    // Check if game is already won
    if (board.checkGameResult() == GameResult::Won) {
        return {
            {0, 0}, 1.0, true, 0,
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime_),
            "Game already won"};
    }

    // Generate all compatible boards
    std::vector<Board> compatibleBoards;
    size_t boardCount = 0;

    CompatibleBoardGenerator::generateAllTypes(
        board, [&](const Board& b) {
            compatibleBoards.push_back(b);
            boardCount++;

            // Check if we exceed threshold
            if (boardCount > options_.maxCompatibleBoards) {
                return false; // Stop enumeration
            }
            return true;
        });

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime_);

    // If too many boards or timeout, fall back to Monte Carlo
    if (boardCount > options_.maxCompatibleBoards || checkTimeout()) {
        return solveMonteCarlo(board);
    }

    return solveExhaustive(board, compatibleBoards);
}

std::vector<Position> Solver::findSafePanels(const Board& board) {
    std::vector<Board> compatibleBoards;

    CompatibleBoardGenerator::generateAllTypes(board, [&](const Board& b) {
        compatibleBoards.push_back(b);
        return compatibleBoards.size() < options_.maxCompatibleBoards;
    });

    return ProbabilityCalculator::findGuaranteedSafePanels(board, compatibleBoards);
}

SolverResult Solver::solveExhaustive(const Board& board,
                                     const std::vector<Board>& compatibleBoards) {
    Level level = board.level();

    // Group by type
    std::array<std::vector<size_t>, NUM_TYPES_PER_LEVEL> indicesPerType;

    for (size_t idx = 0; idx < compatibleBoards.size(); idx++) {
        const Board& cb = compatibleBoards[idx];

        int count0 = 0, count2 = 0, count3 = 0;
        for (size_t i = 0; i < BOARD_SIZE; i++) {
            for (size_t j = 0; j < BOARD_SIZE; j++) {
                PanelValue v = cb.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j));
                if (v == PanelValue::Voltorb) count0++;
                else if (v == PanelValue::Two) count2++;
                else if (v == PanelValue::Three) count3++;
            }
        }

        for (BoardTypeIndex type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
            const auto& params = BoardTypeData::params(level, type);
            if (params.n0 == count0 && params.n2 == count2 && params.n3 == count3) {
                indicesPerType[type].push_back(idx);
                break;
            }
        }
    }

    // Check for free panels first (guaranteed safe)
    auto freePanel = findFreePanel(board, indicesPerType, compatibleBoards);
    if (freePanel) {
        // Calculate win probability assuming we flip the free panel
        auto [bestPanel, winProb] = computeBestPanel(board, indicesPerType, compatibleBoards, 0);

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime_);

        return {*freePanel,
                winProb,
                true,
                compatibleBoards.size(),
                elapsed,
                "Free panel (guaranteed safe)"};
    }

    // Full recursive search
    auto [bestPanel, winProb] = computeBestPanel(board, indicesPerType, compatibleBoards, 0);

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime_);

    return {bestPanel, winProb, !timedOut_, compatibleBoards.size(), elapsed, std::nullopt};
}

SolverResult Solver::solveMonteCarlo(const Board& board) {
    MonteCarloSampler sampler;

    auto sampledProbs = sampler.sampleProbabilities(board, options_.sampleSize);

    if (sampledProbs.samplesUsed == 0 || sampledProbs.unknownPanels.empty()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime_);
        return {{0, 0}, 0.0, false, 0, elapsed, "No samples generated"};
    }

    // Find panel with lowest voltorb probability
    size_t bestIdx = 0;
    double lowestVoltorbProb = 1.0;

    for (size_t i = 0; i < sampledProbs.unknownPanels.size(); i++) {
        double pVoltorb = sampledProbs.probs[i][0];
        if (pVoltorb < lowestVoltorbProb) {
            lowestVoltorbProb = pVoltorb;
            bestIdx = i;
        }
    }

    // Estimate win probability (simplified: just survival probability for now)
    double survivalProb = 1.0 - lowestVoltorbProb;

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime_);

    return {sampledProbs.unknownPanels[bestIdx],
            survivalProb,
            false,
            sampledProbs.samplesUsed,
            elapsed,
            "Monte Carlo sampling (approximate)"};
}

std::optional<Position>
Solver::findFreePanel(const Board& board,
                      const std::array<std::vector<size_t>, NUM_TYPES_PER_LEVEL>& indicesPerType,
                      const std::vector<Board>& allBoards) {

    for (size_t i = 0; i < BOARD_SIZE; i++) {
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            Position pos{static_cast<uint8_t>(i), static_cast<uint8_t>(j)};

            if (board.get(pos) != PanelValue::Unknown) {
                continue;
            }

            bool isFree = true;

            for (BoardTypeIndex type = 0; type < NUM_TYPES_PER_LEVEL && isFree; type++) {
                for (size_t idx : indicesPerType[type]) {
                    if (allBoards[idx].get(pos) == PanelValue::Voltorb) {
                        isFree = false;
                        break;
                    }
                }
            }

            if (isFree) {
                return pos;
            }
        }
    }

    return std::nullopt;
}

double Solver::calculateProbNorm(
    const Board& board,
    const std::array<std::vector<size_t>, NUM_TYPES_PER_LEVEL>& indicesPerType) {

    Level level = board.level();
    double pBoard = 0.0;

    for (BoardTypeIndex type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
        int64_t nAccepted = BoardTypeData::acceptedCount(level, type);
        if (nAccepted > 0) {
            pBoard += static_cast<double>(indicesPerType[type].size()) /
                      static_cast<double>(nAccepted);
        }
    }

    return pBoard;
}

std::array<std::vector<size_t>, NUM_TYPES_PER_LEVEL>
Solver::filterIndices(const Board& revealedBoard,
                      const std::array<std::vector<size_t>, NUM_TYPES_PER_LEVEL>& currentIndices,
                      const std::vector<Board>& allBoards) {

    std::array<std::vector<size_t>, NUM_TYPES_PER_LEVEL> filtered;

    for (BoardTypeIndex type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
        for (size_t idx : currentIndices[type]) {
            bool compatible = true;

            for (size_t i = 0; i < BOARD_SIZE && compatible; i++) {
                for (size_t j = 0; j < BOARD_SIZE && compatible; j++) {
                    PanelValue revealed =
                        revealedBoard.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j));
                    if (isKnown(revealed)) {
                        PanelValue actual =
                            allBoards[idx].get(static_cast<uint8_t>(i), static_cast<uint8_t>(j));
                        if (actual != revealed) {
                            compatible = false;
                        }
                    }
                }
            }

            if (compatible) {
                filtered[type].push_back(idx);
            }
        }
    }

    return filtered;
}

std::pair<Position, double>
Solver::computeBestPanel(const Board& board,
                         const std::array<std::vector<size_t>, NUM_TYPES_PER_LEVEL>& indicesPerType,
                         const std::vector<Board>& allBoards, int depth) {

    // Check for win: all remaining unknown panels are non-multipliers in ALL compatible boards
    // This means all 2s and 3s have already been revealed
    bool allMultipliersRevealed = true;
    for (size_t i = 0; i < BOARD_SIZE && allMultipliersRevealed; i++) {
        for (size_t j = 0; j < BOARD_SIZE && allMultipliersRevealed; j++) {
            PanelValue v = board.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j));
            if (v == PanelValue::Unknown) {
                // Check if any compatible board has a multiplier here
                for (BoardTypeIndex type = 0; type < NUM_TYPES_PER_LEVEL && allMultipliersRevealed; type++) {
                    for (size_t idx : indicesPerType[type]) {
                        PanelValue actual = allBoards[idx].get(static_cast<uint8_t>(i), static_cast<uint8_t>(j));
                        if (isMultiplier(actual)) {
                            allMultipliersRevealed = false;
                            break;
                        }
                    }
                }
            }
        }
    }
    if (allMultipliersRevealed) {
        return {{0, 0}, 1.0};
    }

    // Check memoization cache
    size_t boardHash = board.hash();
    auto cacheIt = cache_.find(boardHash);
    if (cacheIt != cache_.end()) {
        cacheHits_++;
        return {cacheIt->second.bestPanel, cacheIt->second.winProb};
    }
    cacheMisses_++;

    // Check timeout
    if (checkTimeout()) {
        return {{0, 0}, 0.0};
    }

    double pBoard = calculateProbNorm(board, indicesPerType);

    if (pBoard <= 0) {
        return {{0, 0}, 0.0};
    }

    // Look for free panel first
    auto freePanel = findFreePanel(board, indicesPerType, allBoards);
    if (freePanel) {
        double winChance = 0.0;

        for (int value = 1; value <= 3; value++) {
            Board nextBoard = board.withPanelRevealed(*freePanel, static_cast<PanelValue>(value));
            auto nextIndices = filterIndices(nextBoard, indicesPerType, allBoards);

            double pNext = calculateProbNorm(nextBoard, nextIndices);
            if (pNext > 0) {
                auto [_, nextWinProb] =
                    computeBestPanel(nextBoard, nextIndices, allBoards, depth + 1);
                winChance += (pNext / pBoard) * nextWinProb;
            }
        }

        cache_[boardHash] = {*freePanel, winChance};
        return {*freePanel, winChance};
    }

    // Find unknown panels
    std::vector<Position> unknownPanels;
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            if (board.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j)) == PanelValue::Unknown) {
                unknownPanels.push_back({static_cast<uint8_t>(i), static_cast<uint8_t>(j)});
            }
        }
    }

    Position bestPanel{0, 0};
    double bestWinProb = 0.0;

    for (const Position& pos : unknownPanels) {
        if (checkTimeout()) break;

        double panelWinProb = 0.0;

        // Try each non-voltorb value
        for (int value = 1; value <= 3; value++) {
            Board nextBoard = board.withPanelRevealed(pos, static_cast<PanelValue>(value));
            auto nextIndices = filterIndices(nextBoard, indicesPerType, allBoards);

            double pNext = calculateProbNorm(nextBoard, nextIndices);
            if (pNext > 0) {
                auto [_, nextWinProb] =
                    computeBestPanel(nextBoard, nextIndices, allBoards, depth + 1);
                panelWinProb += (pNext / pBoard) * nextWinProb;
            }
        }

        if (panelWinProb > bestWinProb) {
            bestWinProb = panelWinProb;
            bestPanel = pos;
        }
    }

    cache_[boardHash] = {bestPanel, bestWinProb};
    return {bestPanel, bestWinProb};
}

std::vector<std::pair<Position, double>> Solver::getPanelWinProbabilities(const Board& board) {
    std::vector<std::pair<Position, double>> result;

    // This is a simplified version that doesn't do full minimax
    // Just returns the immediate survival probabilities

    std::vector<Board> compatibleBoards;
    CompatibleBoardGenerator::generateAllTypes(board, [&](const Board& b) {
        compatibleBoards.push_back(b);
        return compatibleBoards.size() < options_.maxCompatibleBoards;
    });

    auto probs = ProbabilityCalculator::calculate(board, compatibleBoards);

    for (const auto& panelProb : probs.panels) {
        result.emplace_back(panelProb.pos, panelProb.pSafe());
    }

    return result;
}

} // namespace voltorb
