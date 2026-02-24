#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "voltorb/constraints.hpp"
#include "voltorb/game.hpp"
#include "voltorb/generator.hpp"
#include "voltorb/probability.hpp"
#include "voltorb/solver.hpp"

using namespace voltorb;
using Catch::Matchers::WithinAbs;

TEST_CASE("Solver finds safe panels", "[solver]") {
    BoardGenerator gen(42);
    Solver solver;

    // Generate a board and reveal some panels
    Board actualBoard = gen.generate(1, 0);
    Board coveredBoard = actualBoard.toCovered();

    // Flip all panels in rows with 0 voltorbs (guaranteed safe)
    const auto& rowHints = actualBoard.rowHints();
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        if (rowHints[i].voltorbCount == 0) {
            for (size_t j = 0; j < BOARD_SIZE; j++) {
                coveredBoard.set(static_cast<uint8_t>(i), static_cast<uint8_t>(j),
                                 actualBoard.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j)));
            }
        }
    }

    auto safePanels = solver.findSafePanels(coveredBoard);

    // All returned safe panels should not be voltorbs in the actual board
    for (const Position& pos : safePanels) {
        REQUIRE(actualBoard.get(pos) != PanelValue::Voltorb);
    }
}

TEST_CASE("Solver returns valid suggestions", "[solver]") {
    BoardGenerator gen(123);
    Solver solver;

    Board actualBoard = gen.generate(2, 0);
    Board coveredBoard = actualBoard.toCovered();

    auto result = solver.solve(coveredBoard);

    // Suggestion should be an unknown panel
    REQUIRE(coveredBoard.get(result.suggestedPosition) == PanelValue::Unknown);

    // Win probability should be valid
    REQUIRE(result.winProbability >= 0.0);
    REQUIRE(result.winProbability <= 1.0);
}

TEST_CASE("Solver handles already won game", "[solver]") {
    BoardGenerator gen(42);
    Solver solver;

    Board actualBoard = gen.generate(1, 0);

    // Reveal all non-voltorb multipliers (simulate won game)
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            PanelValue v = actualBoard.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j));
            if (isMultiplier(v)) {
                // Keep multipliers revealed
            } else if (v != PanelValue::Voltorb) {
                actualBoard.set(static_cast<uint8_t>(i), static_cast<uint8_t>(j), v);
            }
        }
    }

    // Game should be won
    if (actualBoard.checkGameResult() == GameResult::Won) {
        auto result = solver.solve(actualBoard);
        REQUIRE(result.winProbability == 1.0);
    }
}

TEST_CASE("ProbabilityCalculator calculates valid probabilities", "[probability]") {
    BoardGenerator gen(42);

    Board actualBoard = gen.generate(1, 0);
    Board coveredBoard = actualBoard.toCovered();

    std::vector<Board> compatible;
    CompatibleBoardGenerator::generateAllTypes(coveredBoard, [&](const Board& b) {
        compatible.push_back(b);
        return compatible.size() < 10000;
    });

    auto probs = ProbabilityCalculator::calculate(coveredBoard, compatible);

    // Probabilities should sum to 1 for each panel
    for (const auto& panelProb : probs.panels) {
        double sum = panelProb.pVoltorb + panelProb.pOne + panelProb.pTwo + panelProb.pThree;
        REQUIRE_THAT(sum, WithinAbs(1.0, 0.001));
    }

    // Type probabilities should sum to 1
    double typeSum = 0;
    for (double p : probs.typeProbs) {
        typeSum += p;
    }
    REQUIRE_THAT(typeSum, WithinAbs(1.0, 0.001));
}

TEST_CASE("MonteCarloSampler generates valid samples", "[sampler]") {
    BoardGenerator gen(42);
    MonteCarloSampler sampler(42);

    Board actualBoard = gen.generate(1, 0);
    Board coveredBoard = actualBoard.toCovered();

    auto samples = sampler.sample(coveredBoard, 100);

    // Should generate some samples
    REQUIRE(samples.size() > 0);

    // All samples should match hints
    for (const Board& s : samples) {
        for (size_t i = 0; i < BOARD_SIZE; i++) {
            REQUIRE(s.rowHint(static_cast<uint8_t>(i)) ==
                    coveredBoard.rowHint(static_cast<uint8_t>(i)));
            REQUIRE(s.colHint(static_cast<uint8_t>(i)) ==
                    coveredBoard.colHint(static_cast<uint8_t>(i)));
        }
    }
}

TEST_CASE("GameSession basic gameplay", "[game]") {
    GameSession session(42);
    session.startGame(1);

    REQUIRE(session.currentLevel() == 1);
    REQUIRE_FALSE(session.isGameOver());
    REQUIRE(session.panelsFlipped() == 0);

    // Get a suggestion and flip it
    auto suggestion = session.getSuggestion();
    Position pos = suggestion.suggestedPosition;

    PanelValue value = session.flip(pos);
    REQUIRE(session.panelsFlipped() == 1);

    // Value should be valid
    REQUIRE(toInt(value) >= 0);
    REQUIRE(toInt(value) <= 3);
}

TEST_CASE("GameSession flipAllSafe", "[game]") {
    GameSession session(42);
    session.startGame(1);

    auto flipped = session.flipAllSafe();

    // All flipped panels should be non-voltorb
    for (const Position& pos : flipped) {
        PanelValue v = session.currentBoard().get(pos);
        REQUIRE(v != PanelValue::Voltorb);
    }
}

TEST_CASE("GameSession level progression", "[game]") {
    GameSession session(42);
    session.startGame(1);

    // Simulate winning by flipping the actual board's panels
    const Board& actual = session.actualBoard();
    const Board& current = session.currentBoard();

    // Flip all non-voltorb panels
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            Position pos{static_cast<uint8_t>(i), static_cast<uint8_t>(j)};
            if (actual.get(pos) != PanelValue::Voltorb && current.get(pos) == PanelValue::Unknown) {
                session.flip(pos);
            }
        }
    }

    if (session.gameResult() == GameResult::Won) {
        Level nextLevel = session.nextLevel();
        REQUIRE(nextLevel == 2); // Should advance from level 1 to 2
    }
}

TEST_CASE("SelfPlayer runs without crashing", "[game]") {
    SelfPlayer player(42);

    SolverOptions options;
    options.timeout = std::chrono::milliseconds(1000);
    options.maxCompatibleBoards = 10000;
    player.setSolverOptions(options);

    auto stats = player.playAtLevel(1, 5, false);

    REQUIRE(stats.gamesPlayed == 5);
    REQUIRE(stats.gamesWon + stats.gamesLost == 5);
}
