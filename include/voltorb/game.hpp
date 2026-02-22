#pragma once

#include "board.hpp"
#include "generator.hpp"
#include "solver.hpp"

#include <functional>

namespace voltorb {

// Represents a complete game session with state tracking
class GameSession {
public:
    // Callback types
    using MoveCallback = std::function<void(Position pos, PanelValue value, GameResult result)>;
    using SuggestionCallback = std::function<void(const SolverResult& suggestion)>;

    explicit GameSession(uint64_t seed = 0);

    // Start a new game at the given level
    void startGame(Level level);

    // Start a new game with a specific board (for testing)
    void startGame(const Board& board);

    // Flip a panel (returns the value and updates game state)
    PanelValue flip(Position pos);
    PanelValue flip(uint8_t row, uint8_t col);

    // Get solver suggestion for next move
    SolverResult getSuggestion();

    // Auto-play: flip all safe panels
    std::vector<Position> flipAllSafe();

    // State queries
    const Board& currentBoard() const { return coveredBoard_; }
    const Board& actualBoard() const { return actualBoard_; }
    GameResult gameResult() const;
    Level currentLevel() const { return currentLevel_; }
    bool isGameOver() const;

    // Statistics
    size_t panelsFlipped() const { return panelsFlipped_; }
    int currentScore() const; // Product of flipped values

    // Level progression (call after game ends)
    Level nextLevel() const;

    // Set callbacks
    void setMoveCallback(MoveCallback cb) { moveCallback_ = std::move(cb); }
    void setSuggestionCallback(SuggestionCallback cb) { suggestionCallback_ = std::move(cb); }

    // Configure solver
    void setSolverOptions(SolverOptions options);

private:
    BoardGenerator generator_;
    Solver solver_;

    Board actualBoard_;   // True board state (fully revealed)
    Board coveredBoard_;  // What player sees (partially revealed)
    Level currentLevel_ = 1;
    size_t panelsFlipped_ = 0;

    MoveCallback moveCallback_;
    SuggestionCallback suggestionCallback_;

    // Update level based on game result
    void updateLevel();
};

// Self-play statistics
struct SelfPlayStats {
    size_t gamesPlayed = 0;
    size_t gamesWon = 0;
    size_t gamesLost = 0;
    int64_t totalCoins = 0;
    std::array<size_t, MAX_LEVEL> winsPerLevel{};
    std::array<size_t, MAX_LEVEL> lossesPerLevel{};
    std::array<size_t, MAX_LEVEL> exactSolves{};
    std::array<size_t, MAX_LEVEL> sampledSolves{};

    double winRate() const {
        return gamesPlayed > 0 ? static_cast<double>(gamesWon) / gamesPlayed : 0.0;
    }

    double winRateForLevel(Level level) const {
        size_t total = winsPerLevel[level - 1] + lossesPerLevel[level - 1];
        return total > 0 ? static_cast<double>(winsPerLevel[level - 1]) / total : 0.0;
    }
};

// Run self-play games
class SelfPlayer {
public:
    explicit SelfPlayer(uint64_t seed = 0);

    // Play N games starting from level 1
    SelfPlayStats play(size_t numGames, bool verbose = false);

    // Play N games at a specific level
    SelfPlayStats playAtLevel(Level level, size_t numGames, bool verbose = false);

    // Configure solver
    void setSolverOptions(SolverOptions options) { solverOptions_ = options; }

private:
    uint64_t seed_;
    SolverOptions solverOptions_;
};

} // namespace voltorb
