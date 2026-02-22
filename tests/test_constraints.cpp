#include <catch2/catch_test_macros.hpp>

#include "voltorb/board_type.hpp"
#include "voltorb/constraints.hpp"
#include "voltorb/generator.hpp"

using namespace voltorb;

TEST_CASE("LegalityChecker basic checks", "[constraints]") {
    BoardGenerator gen(42);

    // Generate a valid board and verify it passes legality
    for (Level level = 1; level <= MAX_LEVEL; level++) {
        for (BoardTypeIndex type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
            Board board = gen.generate(level, type);
            const auto& params = BoardTypeData::params(level, type);

            REQUIRE(LegalityChecker::isLegal(board, params));
        }
    }
}

TEST_CASE("VoltorbPositionGenerator basic", "[constraints]") {
    // Create a simple board with known hints
    Board board(1);

    // Set up hints: each row and column has 1 voltorb
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        board.setRowHint(static_cast<uint8_t>(i), {4, 1}); // sum=4, 1 voltorb
        board.setColHint(static_cast<uint8_t>(i), {4, 1});
    }

    auto positions = VoltorbPositionGenerator::generateAll(board);

    // Should generate valid voltorb configurations
    REQUIRE(positions.size() > 0);

    // Verify each configuration has correct voltorb counts
    for (const auto& pos : positions) {
        // Check row counts
        for (size_t i = 0; i < BOARD_SIZE; i++) {
            int count = 0;
            for (size_t j = 0; j < BOARD_SIZE; j++) {
                if (pos.isVoltorb[i][j]) count++;
            }
            REQUIRE(count == 1);
        }

        // Check column counts
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            int count = 0;
            for (size_t i = 0; i < BOARD_SIZE; i++) {
                if (pos.isVoltorb[i][j]) count++;
            }
            REQUIRE(count == 1);
        }
    }
}

TEST_CASE("VoltorbPositionGenerator respects revealed panels", "[constraints]") {
    Board board(1);

    // Simple hints
    board.setRowHint(0, {4, 1});
    board.setRowHint(1, {5, 0});
    board.setRowHint(2, {4, 1});
    board.setRowHint(3, {4, 1});
    board.setRowHint(4, {4, 1});

    board.setColHint(0, {4, 1});
    board.setColHint(1, {4, 1});
    board.setColHint(2, {5, 0});
    board.setColHint(3, {4, 1});
    board.setColHint(4, {4, 1});

    // Reveal a voltorb at (0,0)
    board.set(0, 0, PanelValue::Voltorb);

    auto positions = VoltorbPositionGenerator::generateAll(board);

    // All valid configurations should have voltorb at (0,0)
    for (const auto& pos : positions) {
        REQUIRE(pos.isVoltorb[0][0] == true);
    }
}

TEST_CASE("CompatibleBoardGenerator generates valid boards", "[constraints]") {
    BoardGenerator gen(42);

    // Generate a random board
    Board actualBoard = gen.generate(3, 0);
    Board coveredBoard = actualBoard.toCovered();

    // Generate compatible boards
    std::vector<Board> compatible;
    CompatibleBoardGenerator::generate(coveredBoard, 0, [&](const Board& b) {
        compatible.push_back(b);
        return compatible.size() < 1000; // Limit for testing
    });

    REQUIRE(compatible.size() > 0);

    // Verify each compatible board matches the constraints
    const auto& params = BoardTypeData::params(3, 0);
    for (const auto& cb : compatible) {
        // Check counts
        int count0 = 0, count1 = 0, count2 = 0, count3 = 0;
        for (size_t i = 0; i < BOARD_SIZE; i++) {
            for (size_t j = 0; j < BOARD_SIZE; j++) {
                PanelValue v = cb.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j));
                switch (toInt(v)) {
                case 0:
                    count0++;
                    break;
                case 1:
                    count1++;
                    break;
                case 2:
                    count2++;
                    break;
                case 3:
                    count3++;
                    break;
                }
            }
        }

        REQUIRE(count0 == params.n0);
        REQUIRE(count2 == params.n2);
        REQUIRE(count3 == params.n3);
        REQUIRE(count1 == params.n1());

        // Check legality
        REQUIRE(LegalityChecker::isLegal(cb, params));
    }
}

TEST_CASE("CompatibleBoardGenerator respects revealed panels", "[constraints]") {
    BoardGenerator gen(42);

    // Generate actual board and reveal some panels
    Board actualBoard = gen.generate(2, 3);
    Board coveredBoard = actualBoard.toCovered();

    // Reveal panel at (1, 1)
    PanelValue revealedValue = actualBoard.get(1, 1);
    coveredBoard.set(1, 1, revealedValue);

    std::vector<Board> compatible;
    CompatibleBoardGenerator::generate(coveredBoard, 3, [&](const Board& b) {
        compatible.push_back(b);
        return compatible.size() < 1000;
    });

    // All compatible boards should have the same value at (1, 1)
    for (const auto& cb : compatible) {
        REQUIRE(cb.get(1, 1) == revealedValue);
    }
}

TEST_CASE("panelsDontExceedConstraints", "[constraints]") {
    Board board;
    board.setRowHint(0, {5, 0});

    // Fill row 0 with valid values summing to 5
    board.set(0, 0, PanelValue::One);
    board.set(0, 1, PanelValue::One);
    board.set(0, 2, PanelValue::One);
    board.set(0, 3, PanelValue::One);
    board.set(0, 4, PanelValue::One);

    REQUIRE(LegalityChecker::panelsDontExceedConstraints(board));

    // Now exceed the sum
    board.set(0, 4, PanelValue::Two); // Sum is now 6
    REQUIRE_FALSE(LegalityChecker::panelsDontExceedConstraints(board));
}
