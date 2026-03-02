#pragma once

#include "board.hpp"
#include "sampler.hpp"
#include "zobrist.hpp"

#include <chrono>
#include <functional>
#include <optional>
#include <unordered_map>
#include <vector>

namespace voltorb {

/**
 * Result of solver computation.
 */
struct SolverResult {
    Position suggestedPosition;
    double winProbability;
    double winProbabilityUpper;  // Upper bound on win probability (h=1 at leaves)
    bool isExact;         // true = fully searched, false = depth-limited
    size_t boardsEvaluated;
    std::chrono::milliseconds computeTime;
    int searchDepth;      // Depth at which this result was computed

    // Additional info
    std::optional<std::string> reason; // Why this panel was chosen
};

/**
 * Progress update during iterative deepening.
 * Sent at each completed depth level.
 */
struct SolverProgress {
    Position bestPanel;
    double winProbability;
    double winProbabilityUpper;  // Upper bound on win probability
    int depth;            // Current search depth
    bool isExact;         // True when fully converged (no depth limit hit)
    size_t nodesSearched; // Nodes evaluated so far
    std::chrono::milliseconds elapsed;
};

/**
 * Callback for progress updates during iterative deepening.
 */
using ProgressCallback = std::function<void(const SolverProgress&)>;

/**
 * Solver options.
 */
struct SolverOptions {
    std::chrono::milliseconds timeout{10000};  // Max time for solving
    size_t maxCompatibleBoards{500000};        // Max boards to enumerate
    size_t transpositionTableSize{1 << 20};    // 1M entries (~32MB)
    bool enableIterativeDeepening{true};       // Use iterative deepening
    int maxDepth{100};                         // Max search depth (effectively unlimited)
    size_t sampleSize{10000};                  // Deprecated: kept for API compatibility
};

/**
 * Internal state for a board during search.
 * Tracks compatible boards grouped by type for efficient filtering.
 */
struct SearchState {
    Board board;
    uint64_t zobristHash;
    std::array<std::vector<size_t>, NUM_TYPES_PER_LEVEL> indicesPerType;
    double pBoardNorm;  // Normalization factor for probability calculation

    // Count total compatible boards
    size_t totalCompatible() const {
        size_t total = 0;
        for (const auto& indices : indicesPerType) {
            total += indices.size();
        }
        return total;
    }
};

/**
 * Result from depth-limited search.
 */
struct DepthLimitedResult {
    Position bestPanel;
    double winProbability;
    double winProbabilityUpper;  // Upper bound (h=1 at leaves)
    bool fullyExplored;  // True if search completed without hitting depth limit
};

struct MemoEntry {
    double winProbability;
    double winProbabilityUpper;
    Position bestPanel;
    int depth;
    bool fullyExplored;
};

/**
 * Main solver using iterative deepening with minimax.
 *
 * The solver computes the exact win probability by searching all possible
 * game outcomes. It uses several optimizations:
 *
 * 1. Zobrist hashing for O(1) incremental hash updates
 * 2. Transposition table for memoization with depth tracking
 * 3. Move ordering (safe panels first, then by survival probability)
 * 4. Upper-bound pruning (skip panels that can't beat current best)
 * 5. Iterative deepening for anytime results
 *
 * The algorithm is provably optimal: it maximizes win probability by
 * computing the exact expectimax tree.
 */
class Solver {
public:
    explicit Solver(SolverOptions options = {});

    /**
     * Find the best panel to flip.
     * Uses iterative deepening with progress callbacks.
     *
     * @param board The current board state
     * @param onProgress Optional callback for progress updates
     * @return The best panel and win probability
     */
    SolverResult solve(const Board& board,
                       ProgressCallback onProgress = nullptr);

    /**
     * Find all guaranteed safe panels (can be flipped without risk).
     */
    std::vector<Position> findSafePanels(const Board& board);

    /**
     * Get probability distribution for all panels.
     * Returns (position, win_probability_if_chosen) pairs.
     */
    std::vector<std::pair<Position, double>> getPanelWinProbabilities(const Board& board);

    /**
     * Reset solver state (clears transposition table and statistics).
     */
    void reset();

    /**
     * Clear memoization cache. Alias for reset() for API compatibility.
     */
    void clearCache() { reset(); }

    /**
     * Get statistics.
     */
    size_t getCacheHits() const { return cacheHits_; }
    size_t getCacheMisses() const { return cacheMisses_; }
    size_t getNodesEvaluated() const { return nodesEvaluated_; }

    /**
     * Get compatible boards from last solve() call.
     * Used by WASM bindings for probability calculation.
     */
    const std::vector<Board>& getCompatibleBoards() const { return allBoards_; }

    /**
     * Check if compatible board generation was capped.
     */
    bool isCapped() const { return allBoards_.size() >= options_.maxCompatibleBoards; }

private:
    SolverOptions options_;
    std::unordered_map<uint64_t, MemoEntry> memo_;

    // Statistics
    size_t nodesEvaluated_ = 0;
    size_t cacheHits_ = 0;
    size_t cacheMisses_ = 0;

    // Timeout tracking
    std::chrono::steady_clock::time_point startTime_;
    bool timedOut_ = false;

    // All compatible boards (stored during solve)
    std::vector<Board> allBoards_;

    // Check if we've exceeded timeout
    bool checkTimeout();

    /**
     * Generate compatible boards and group by type.
     * Returns the initial search state.
     */
    std::optional<SearchState> initializeSearch(const Board& board);

    /**
     * Iterative deepening search.
     * Searches at increasing depths, calling onProgress after each depth.
     */
    SolverResult iterativeDeepening(const SearchState& initialState,
                                    ProgressCallback onProgress);

    /**
     * Depth-limited minimax search.
     *
     * @param state Current search state
     * @param depthLimit Maximum remaining depth
     * @return Best panel and win probability
     */
    DepthLimitedResult depthLimitedSearch(const SearchState& state, int depthLimit);

    /**
     * Result of heuristic evaluation: lower and upper bounds.
     */
    struct HeuristicResult {
        double lower;
        double upper;
    };

    /**
     * Heuristic evaluation for leaf nodes when depth limit is reached.
     * Returns lower and upper bounds on win probability based on:
     * - Number of remaining multipliers to reveal (max across boards for lower, min for upper)
     * - Distribution of voltorb probabilities among unknown panels
     */
    HeuristicResult heuristicEval(const SearchState& state);

    /**
     * Check if the game is won (all multipliers revealed).
     */
    bool isWon(const SearchState& state) const;

    /**
     * Check if any compatible board has a multiplier (2 or 3) at this position.
     * Panels without multiplier potential are useless to flip.
     */
    bool hasMultiplierPotential(const SearchState& state, Position pos) const;

    /**
     * Find a free (guaranteed safe) panel.
     */
    std::optional<Position> findFreePanel(const SearchState& state) const;

    /**
     * Get unknown panels sorted by heuristic (move ordering).
     * Panels with lower voltorb probability come first.
     */
    std::vector<Position> getOrderedUnknownPanels(const SearchState& state) const;

    /**
     * Calculate P(board) normalization factor.
     */
    double calculateProbNorm(const SearchState& state) const;

    /**
     * Create a new search state after revealing a panel.
     */
    SearchState revealPanel(const SearchState& state, Position pos, PanelValue value) const;

    /**
     * Calculate probability of a specific value at a position.
     */
    double probabilityOf(const SearchState& state, Position pos, PanelValue value) const;

    /**
     * Get upper bound on win probability for a panel.
     * Used for pruning: if upper_bound <= current_best, skip this panel.
     */
    double getUpperBound(const SearchState& state, Position pos) const;
};

} // namespace voltorb
