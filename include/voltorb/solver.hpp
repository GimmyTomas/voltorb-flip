#pragma once

#include "board.hpp"
#include "sampler.hpp"

#include <chrono>
#include <optional>
#include <unordered_map>
#include <vector>

namespace voltorb {

// Result of solver computation
struct SolverResult {
    Position suggestedPosition;
    double winProbability;
    bool isExact;         // true = exhaustive, false = sampled
    size_t boardsEvaluated;
    std::chrono::milliseconds computeTime;

    // Additional info
    std::optional<std::string> reason; // Why this panel was chosen
};

// Solver options
struct SolverOptions {
    std::chrono::milliseconds timeout{5000};
    size_t maxCompatibleBoards{100000};  // Fallback threshold
    size_t sampleSize{10000};            // For Monte Carlo
    bool enableParallel{true};           // Use multiple threads
};

// Memoization entry
struct MemoEntry {
    Position bestPanel;
    double winProb;
};

// Main solver using recursive minimax with memoization
class Solver {
public:
    explicit Solver(SolverOptions options = {});

    // Find the best panel to flip
    SolverResult solve(const Board& board);

    // Find all guaranteed safe panels (can be flipped without risk)
    std::vector<Position> findSafePanels(const Board& board);

    // Get probability distribution for all panels
    std::vector<std::pair<Position, double>> getPanelWinProbabilities(const Board& board);

    // Reset memoization cache
    void clearCache();

    // Get statistics
    size_t getCacheHits() const { return cacheHits_; }
    size_t getCacheMisses() const { return cacheMisses_; }

private:
    SolverOptions options_;
    std::unordered_map<size_t, MemoEntry> cache_;
    size_t cacheHits_ = 0;
    size_t cacheMisses_ = 0;

    // Timeout tracking
    std::chrono::steady_clock::time_point startTime_;
    bool timedOut_ = false;

    // Check if we've exceeded timeout
    bool checkTimeout();

    // Exhaustive solver
    SolverResult solveExhaustive(const Board& board,
                                 const std::vector<Board>& compatibleBoards);

    // Monte Carlo fallback solver
    SolverResult solveMonteCarlo(const Board& board);

    // Recursive computation of win probability
    // Returns (best_panel, win_probability)
    std::pair<Position, double>
    computeBestPanel(const Board& board,
                     const std::array<std::vector<size_t>, NUM_TYPES_PER_LEVEL>& indicesPerType,
                     const std::vector<Board>& allBoards, int depth);

    // Check for free panel (guaranteed safe)
    std::optional<Position>
    findFreePanel(const Board& board,
                  const std::array<std::vector<size_t>, NUM_TYPES_PER_LEVEL>& indicesPerType,
                  const std::vector<Board>& allBoards);

    // Calculate P(board) normalization factor
    double calculateProbNorm(
        const Board& board,
        const std::array<std::vector<size_t>, NUM_TYPES_PER_LEVEL>& indicesPerType);

    // Filter compatible boards after revealing a panel
    std::array<std::vector<size_t>, NUM_TYPES_PER_LEVEL>
    filterIndices(const Board& revealedBoard,
                  const std::array<std::vector<size_t>, NUM_TYPES_PER_LEVEL>& currentIndices,
                  const std::vector<Board>& allBoards);
};

} // namespace voltorb
