#include <catch2/catch_test_macros.hpp>

#include "voltorb/board.hpp"
#include "voltorb/board_type.hpp"

using namespace voltorb;

TEST_CASE("Board default construction", "[board]") {
    Board board;

    REQUIRE(board.level() == 1);

    for (size_t i = 0; i < BOARD_SIZE; i++) {
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            REQUIRE(board.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j)) ==
                    PanelValue::Unknown);
        }
    }
}

TEST_CASE("Board with level", "[board]") {
    Board board(5);
    REQUIRE(board.level() == 5);
}

TEST_CASE("Board panel get/set", "[board]") {
    Board board;

    board.set(2, 3, PanelValue::Two);
    REQUIRE(board.get(2, 3) == PanelValue::Two);

    board.set(Position{1, 4}, PanelValue::Voltorb);
    REQUIRE(board.get(Position{1, 4}) == PanelValue::Voltorb);
}

TEST_CASE("Board hints", "[board]") {
    Board board;

    board.setRowHint(0, {10, 2});
    REQUIRE(board.rowHint(0).sum == 10);
    REQUIRE(board.rowHint(0).voltorbCount == 2);

    board.setColHint(3, {7, 1});
    REQUIRE(board.colHint(3).sum == 7);
    REQUIRE(board.colHint(3).voltorbCount == 1);
}

TEST_CASE("Board isFullyRevealed", "[board]") {
    Board board;

    REQUIRE_FALSE(board.isFullyRevealed());

    // Fill all panels
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            board.set(static_cast<uint8_t>(i), static_cast<uint8_t>(j), PanelValue::One);
        }
    }

    REQUIRE(board.isFullyRevealed());
}

TEST_CASE("Board hasVoltorbFlipped", "[board]") {
    Board board;

    REQUIRE_FALSE(board.hasVoltorbFlipped());

    board.set(2, 2, PanelValue::One);
    REQUIRE_FALSE(board.hasVoltorbFlipped());

    board.set(3, 3, PanelValue::Voltorb);
    REQUIRE(board.hasVoltorbFlipped());
}

TEST_CASE("Board counting methods", "[board]") {
    Board board;

    REQUIRE(board.countUnknown() == 25);
    REQUIRE(board.countKnown() == 0);

    board.set(0, 0, PanelValue::One);
    board.set(0, 1, PanelValue::Two);
    board.set(0, 2, PanelValue::Three);

    REQUIRE(board.countUnknown() == 22);
    REQUIRE(board.countKnown() == 3);
    REQUIRE(board.countMultipliersRevealed() == 2);
}

TEST_CASE("Board withPanelRevealed", "[board]") {
    Board board;
    board.set(0, 0, PanelValue::One);

    Board newBoard = board.withPanelRevealed({1, 1}, PanelValue::Three);

    // Original unchanged
    REQUIRE(board.get(1, 1) == PanelValue::Unknown);

    // New board has both
    REQUIRE(newBoard.get(0, 0) == PanelValue::One);
    REQUIRE(newBoard.get(1, 1) == PanelValue::Three);
}

TEST_CASE("Board equality", "[board]") {
    Board board1(3);
    Board board2(3);

    REQUIRE(board1 == board2);

    board1.set(2, 2, PanelValue::Two);
    REQUIRE(board1 != board2);

    board2.set(2, 2, PanelValue::Two);
    REQUIRE(board1 == board2);
}

TEST_CASE("Board hash consistency", "[board]") {
    Board board1(3);
    Board board2(3);

    board1.set(1, 1, PanelValue::Two);
    board2.set(1, 1, PanelValue::Two);

    REQUIRE(board1.hash() == board2.hash());

    board2.set(2, 2, PanelValue::Three);
    REQUIRE(board1.hash() != board2.hash());
}

TEST_CASE("Board recalculateHints", "[board]") {
    Board board;

    // Set up a simple board
    board.set(0, 0, PanelValue::One);
    board.set(0, 1, PanelValue::Two);
    board.set(0, 2, PanelValue::Three);
    board.set(0, 3, PanelValue::One);
    board.set(0, 4, PanelValue::One);

    for (size_t i = 1; i < BOARD_SIZE; i++) {
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            board.set(static_cast<uint8_t>(i), static_cast<uint8_t>(j), PanelValue::One);
        }
    }

    board.recalculateHints();

    // Row 0: 1+2+3+1+1 = 8, 0 voltorbs
    REQUIRE(board.rowHint(0).sum == 8);
    REQUIRE(board.rowHint(0).voltorbCount == 0);

    // Other rows: 5 ones = 5, 0 voltorbs
    REQUIRE(board.rowHint(1).sum == 5);
    REQUIRE(board.rowHint(1).voltorbCount == 0);
}

TEST_CASE("Board toCovered", "[board]") {
    Board board;
    board.set(0, 0, PanelValue::One);
    board.set(1, 1, PanelValue::Two);
    board.setRowHint(0, {5, 1});

    Board covered = board.toCovered();

    // All panels should be unknown
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            REQUIRE(covered.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j)) ==
                    PanelValue::Unknown);
        }
    }

    // Hints should be preserved
    REQUIRE(covered.rowHint(0).sum == 5);
    REQUIRE(covered.rowHint(0).voltorbCount == 1);
}

TEST_CASE("BoardTypeParams calculations", "[board_type]") {
    // Level 1, Type 0: n0=6, n2=3, n3=1
    const auto& params = BoardTypeData::params(1, 0);

    REQUIRE(params.n0 == 6);
    REQUIRE(params.n2 == 3);
    REQUIRE(params.n3 == 1);
    REQUIRE(params.n1() == 25 - 6 - 3 - 1); // 15

    // Total sum = 15*1 + 3*2 + 1*3 = 15 + 6 + 3 = 24
    REQUIRE(params.totalSum() == 24);
}

TEST_CASE("BoardTypeData static tables", "[board_type]") {
    // Verify some known values from legacy code
    REQUIRE(BoardTypeData::permutations(1, 0) == 2745758400);
    REQUIRE(BoardTypeData::acceptedCount(1, 0) == 1732660000);

    REQUIRE(BoardTypeData::permutations(8, 0) == 21034470600);
    REQUIRE(BoardTypeData::acceptedCount(8, 0) == 15998354400);
}

TEST_CASE("BoardTypeData compatibility check", "[board_type]") {
    // Level 1, Type 0: n0=6, sum=24
    REQUIRE(BoardTypeData::isCompatible(1, 0, 6, 24));
    REQUIRE_FALSE(BoardTypeData::isCompatible(1, 0, 7, 24)); // Wrong voltorb count
    REQUIRE_FALSE(BoardTypeData::isCompatible(1, 0, 6, 25)); // Wrong sum
}

TEST_CASE("PanelValue helpers", "[types]") {
    REQUIRE(isMultiplier(PanelValue::Two));
    REQUIRE(isMultiplier(PanelValue::Three));
    REQUIRE_FALSE(isMultiplier(PanelValue::One));
    REQUIRE_FALSE(isMultiplier(PanelValue::Voltorb));
    REQUIRE_FALSE(isMultiplier(PanelValue::Unknown));

    REQUIRE(isKnown(PanelValue::One));
    REQUIRE(isKnown(PanelValue::Voltorb));
    REQUIRE_FALSE(isKnown(PanelValue::Unknown));

    REQUIRE(toInt(PanelValue::Voltorb) == 0);
    REQUIRE(toInt(PanelValue::One) == 1);
    REQUIRE(toInt(PanelValue::Two) == 2);
    REQUIRE(toInt(PanelValue::Three) == 3);
}

TEST_CASE("Position helpers", "[types]") {
    Position pos{2, 3};
    REQUIRE(pos.toIndex() == 13);

    Position fromIdx = Position::fromIndex(13);
    REQUIRE(fromIdx.row == 2);
    REQUIRE(fromIdx.col == 3);
}
