#include "voltorb/game.hpp"

#include <iostream>

namespace voltorb {

GameSession::GameSession(uint64_t seed) : generator_(seed), solver_() {}

void GameSession::startGame(Level level) {
    currentLevel_ = level;
    actualBoard_ = generator_.generate(level);
    coveredBoard_ = actualBoard_.toCovered();
    panelsFlipped_ = 0;
}

void GameSession::startGame(const Board& board) {
    actualBoard_ = board;
    currentLevel_ = board.level();
    coveredBoard_ = board.toCovered();
    panelsFlipped_ = 0;
}

PanelValue GameSession::flip(Position pos) {
    return flip(pos.row, pos.col);
}

PanelValue GameSession::flip(uint8_t row, uint8_t col) {
    Position pos{row, col};

    // Already flipped?
    if (coveredBoard_.get(row, col) != PanelValue::Unknown) {
        return coveredBoard_.get(row, col);
    }

    PanelValue value = actualBoard_.get(row, col);
    coveredBoard_.set(row, col, value);
    panelsFlipped_++;

    GameResult result = gameResult();

    if (moveCallback_) {
        moveCallback_(pos, value, result);
    }

    return value;
}

SolverResult GameSession::getSuggestion() {
    auto result = solver_.solve(coveredBoard_);

    if (suggestionCallback_) {
        suggestionCallback_(result);
    }

    return result;
}

std::vector<Position> GameSession::flipAllSafe() {
    std::vector<Position> flipped;

    auto safePanels = solver_.findSafePanels(coveredBoard_);

    for (const Position& pos : safePanels) {
        if (coveredBoard_.get(pos) == PanelValue::Unknown) {
            flip(pos);
            flipped.push_back(pos);
        }
    }

    return flipped;
}

GameResult GameSession::gameResult() const {
    return coveredBoard_.checkGameResult();
}

bool GameSession::isGameOver() const {
    GameResult result = gameResult();
    return result == GameResult::Won || result == GameResult::Lost;
}

int GameSession::currentScore() const {
    int score = 1;

    for (size_t i = 0; i < BOARD_SIZE; i++) {
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            PanelValue v = coveredBoard_.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j));
            if (isKnown(v) && v != PanelValue::Voltorb) {
                score *= toInt(v);
            }
        }
    }

    return score;
}

Level GameSession::nextLevel() const {
    GameResult result = gameResult();

    if (result == GameResult::Won) {
        return std::min(static_cast<Level>(currentLevel_ + 1), MAX_LEVEL);
    } else if (result == GameResult::Lost) {
        // If fewer panels flipped than current level, drop to that number
        // Otherwise stay at current level
        if (panelsFlipped_ < currentLevel_) {
            return std::max(static_cast<Level>(panelsFlipped_), MIN_LEVEL);
        }
        return currentLevel_;
    }

    return currentLevel_;
}

void GameSession::setSolverOptions(SolverOptions options) {
    solver_ = Solver(options);
}

// SelfPlayer implementation

SelfPlayer::SelfPlayer(uint64_t seed) : seed_(seed) {}

SelfPlayStats SelfPlayer::play(size_t numGames, bool verbose) {
    SelfPlayStats stats;
    GameSession session(seed_);
    session.setSolverOptions(solverOptions_);

    Level currentLevel = 1;

    for (size_t game = 0; game < numGames; game++) {
        session.startGame(currentLevel);

        if (verbose) {
            std::cout << "Game " << (game + 1) << " at level " << static_cast<int>(currentLevel)
                      << std::endl;
        }

        while (!session.isGameOver()) {
            auto suggestion = session.getSuggestion();

            if (suggestion.isExact) {
                stats.exactSolves[currentLevel - 1]++;
            } else {
                stats.sampledSolves[currentLevel - 1]++;
            }

            session.flip(suggestion.suggestedPosition);
        }

        stats.gamesPlayed++;

        if (session.gameResult() == GameResult::Won) {
            stats.gamesWon++;
            stats.winsPerLevel[currentLevel - 1]++;
            stats.totalCoins += session.currentScore();

            if (verbose) {
                std::cout << "  Won! Score: " << session.currentScore() << std::endl;
            }
        } else {
            stats.gamesLost++;
            stats.lossesPerLevel[currentLevel - 1]++;

            if (verbose) {
                std::cout << "  Lost after " << session.panelsFlipped() << " panels" << std::endl;
            }
        }

        currentLevel = session.nextLevel();
    }

    return stats;
}

SelfPlayStats SelfPlayer::playAtLevel(Level level, size_t numGames, bool verbose) {
    SelfPlayStats stats;
    GameSession session(seed_);
    session.setSolverOptions(solverOptions_);

    for (size_t game = 0; game < numGames; game++) {
        session.startGame(level);

        if (verbose && (game + 1) % 100 == 0) {
            std::cout << "Game " << (game + 1) << "/" << numGames << std::endl;
        }

        while (!session.isGameOver()) {
            auto suggestion = session.getSuggestion();

            if (suggestion.isExact) {
                stats.exactSolves[level - 1]++;
            } else {
                stats.sampledSolves[level - 1]++;
            }

            session.flip(suggestion.suggestedPosition);
        }

        stats.gamesPlayed++;

        if (session.gameResult() == GameResult::Won) {
            stats.gamesWon++;
            stats.winsPerLevel[level - 1]++;
            stats.totalCoins += session.currentScore();
        } else {
            stats.gamesLost++;
            stats.lossesPerLevel[level - 1]++;
        }
    }

    return stats;
}

} // namespace voltorb
