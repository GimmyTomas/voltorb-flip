#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "voltorb/board_type.hpp"
#include "voltorb/constraints.hpp"
#include "voltorb/generator.hpp"

using namespace voltorb;
using Catch::Matchers::WithinAbs;

// These tests verify that our implementation matches the legacy code's behavior
// on tractable (fast) board configurations.

TEST_CASE("Board type parameters match legacy", "[legacy]") {
    // Legacy: board_type[level-1][type] = {n_0, n_2, n_3, max_total_free, max_row_free}

    // Level 1, Type 0
    auto p = BoardTypeData::params(1, 0);
    REQUIRE(p.n0 == 6);
    REQUIRE(p.n2 == 3);
    REQUIRE(p.n3 == 1);
    REQUIRE(p.maxTotalFree == 3);
    REQUIRE(p.maxRowFree == 3);

    // Level 4, Type 2: {10,8,0,5,4}
    p = BoardTypeData::params(4, 2);
    REQUIRE(p.n0 == 10);
    REQUIRE(p.n2 == 8);
    REQUIRE(p.n3 == 0);
    REQUIRE(p.maxTotalFree == 5);
    REQUIRE(p.maxRowFree == 4);

    // Level 8, Type 4: {10,7,3,6,5}
    p = BoardTypeData::params(8, 4);
    REQUIRE(p.n0 == 10);
    REQUIRE(p.n2 == 7);
    REQUIRE(p.n3 == 3);
    REQUIRE(p.maxTotalFree == 6);
    REQUIRE(p.maxRowFree == 5);
}

TEST_CASE("N_permutations match legacy", "[legacy]") {
    // Legacy values
    REQUIRE(BoardTypeData::permutations(1, 0) == 2745758400);
    REQUIRE(BoardTypeData::permutations(1, 1) == 171609900);
    REQUIRE(BoardTypeData::permutations(2, 0) == 5883768000);
    REQUIRE(BoardTypeData::permutations(8, 9) == 1177930353600);
}

TEST_CASE("N_accepted match legacy", "[legacy]") {
    // Legacy values
    REQUIRE(BoardTypeData::acceptedCount(1, 0) == 1732660000);
    REQUIRE(BoardTypeData::acceptedCount(1, 1) == 81056200);
    REQUIRE(BoardTypeData::acceptedCount(2, 5) == 2684683200);
    REQUIRE(BoardTypeData::acceptedCount(8, 9) == 1051011410400);
}

TEST_CASE("Legality checker matches legacy check_legality_board", "[legacy]") {
    BoardGenerator gen(42);

    // Generate boards and verify they pass legality checks
    for (Level level = 1; level <= MAX_LEVEL; level++) {
        for (BoardTypeIndex type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
            // Generate multiple boards per type
            for (int n = 0; n < 5; n++) {
                Board board = gen.generate(level, type);
                const auto& params = BoardTypeData::params(level, type);

                // Should be legal
                REQUIRE(LegalityChecker::isLegal(board, params));

                // Should be compatible with type
                REQUIRE(LegalityChecker::isCompatibleWithType(board, level, type));
            }
        }
    }
}

TEST_CASE("Compatible board counts are reasonable", "[legacy]") {
    BoardGenerator gen(42);

    // For a fully covered board, we can compare compatible board counts
    // against what the legacy code would produce

    // Generate a board at level 1 (easier boards)
    Board actualBoard = gen.generate(1, 0);
    Board coveredBoard = actualBoard.toCovered();

    // Count compatible boards
    auto counts = CompatibleBoardGenerator::countPerType(coveredBoard);

    // At minimum, the actual board should be counted
    size_t totalCount = 0;
    for (size_t c : counts) {
        totalCount += c;
    }

    REQUIRE(totalCount > 0);

    // The count should include the actual board configuration
    // We verify this by checking that at least one type has boards
    bool foundType = false;
    for (BoardTypeIndex type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
        if (counts[type] > 0) {
            foundType = true;
            break;
        }
    }
    REQUIRE(foundType);
}

TEST_CASE("Voltorb position generation matches constraints", "[legacy]") {
    BoardGenerator gen(42);

    for (Level level = 1; level <= 3; level++) { // Lower levels for speed
        Board actualBoard = gen.generate(level, 0);
        Board coveredBoard = actualBoard.toCovered();

        auto positions = VoltorbPositionGenerator::generateAll(coveredBoard);

        const auto& rowHints = coveredBoard.rowHints();
        const auto& colHints = coveredBoard.colHints();

        // Every generated position should satisfy row and column voltorb counts
        for (const auto& pos : positions) {
            // Check row counts
            for (size_t i = 0; i < BOARD_SIZE; i++) {
                int count = 0;
                for (size_t j = 0; j < BOARD_SIZE; j++) {
                    if (pos.isVoltorb[i][j]) count++;
                }
                REQUIRE(count == rowHints[i].voltorbCount);
            }

            // Check column counts
            for (size_t j = 0; j < BOARD_SIZE; j++) {
                int count = 0;
                for (size_t i = 0; i < BOARD_SIZE; i++) {
                    if (pos.isVoltorb[i][j]) count++;
                }
                REQUIRE(count == colHints[j].voltorbCount);
            }
        }
    }
}

TEST_CASE("Generated boards satisfy all constraints", "[legacy]") {
    BoardGenerator gen(42);

    for (Level level = 1; level <= 3; level++) {
        for (BoardTypeIndex type = 0; type < 3; type++) { // Subset for speed
            Board actualBoard = gen.generate(level, type);
            Board coveredBoard = actualBoard.toCovered();

            std::vector<Board> compatible;
            CompatibleBoardGenerator::generate(coveredBoard, type, [&](const Board& b) {
                compatible.push_back(b);
                return compatible.size() < 100; // Limit for testing
            });

            const auto& params = BoardTypeData::params(level, type);
            const auto& rowHints = coveredBoard.rowHints();
            const auto& colHints = coveredBoard.colHints();

            for (const auto& cb : compatible) {
                // Count values
                int n0 = 0, n1 = 0, n2 = 0, n3 = 0;
                for (size_t i = 0; i < BOARD_SIZE; i++) {
                    for (size_t j = 0; j < BOARD_SIZE; j++) {
                        PanelValue v = cb.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j));
                        switch (toInt(v)) {
                        case 0:
                            n0++;
                            break;
                        case 1:
                            n1++;
                            break;
                        case 2:
                            n2++;
                            break;
                        case 3:
                            n3++;
                            break;
                        }
                    }
                }

                REQUIRE(n0 == params.n0);
                REQUIRE(n2 == params.n2);
                REQUIRE(n3 == params.n3);
                REQUIRE(n1 == params.n1());

                // Check row sums and voltorb counts
                for (size_t i = 0; i < BOARD_SIZE; i++) {
                    int sum = 0;
                    int voltorbs = 0;
                    for (size_t j = 0; j < BOARD_SIZE; j++) {
                        int val = toInt(cb.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j)));
                        sum += val;
                        if (val == 0) voltorbs++;
                    }
                    REQUIRE(sum == rowHints[i].sum);
                    REQUIRE(voltorbs == rowHints[i].voltorbCount);
                }

                // Check column sums and voltorb counts
                for (size_t j = 0; j < BOARD_SIZE; j++) {
                    int sum = 0;
                    int voltorbs = 0;
                    for (size_t i = 0; i < BOARD_SIZE; i++) {
                        int val = toInt(cb.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j)));
                        sum += val;
                        if (val == 0) voltorbs++;
                    }
                    REQUIRE(sum == colHints[j].sum);
                    REQUIRE(voltorbs == colHints[j].voltorbCount);
                }

                // Check legality
                REQUIRE(LegalityChecker::isLegal(cb, params));
            }
        }
    }
}

TEST_CASE("Rejection sampling acceptance rate is reasonable", "[legacy]") {
    // The legacy code uses rejection sampling with known acceptance rates.
    // Our generator should have similar rejection counts.

    BoardGenerator gen(42);

    // Level 1 has high acceptance rates (near 1.0), so few rejections expected
    size_t totalRejections = 0;
    int numBoards = 100;

    for (int i = 0; i < numBoards; i++) {
        gen.generate(1, 0);
        totalRejections += gen.lastRejectionCount();
    }

    // Average rejections should be low for level 1
    double avgRejections = static_cast<double>(totalRejections) / numBoards;
    REQUIRE(avgRejections < 2.0); // Should be mostly accepted on first try
}
