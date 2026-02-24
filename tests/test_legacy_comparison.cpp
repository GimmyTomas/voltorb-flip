#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "legacy_solver.hpp"
#include "voltorb/board_type.hpp"
#include "voltorb/constraints.hpp"
#include "voltorb/generator.hpp"
#include "voltorb/probability.hpp"
#include "voltorb/solver.hpp"

using namespace voltorb;
using Catch::Matchers::WithinAbs;

// ============ Conversion helpers ============

// Convert new Board to legacy Board
static legacy::Board toLegacyBoard(const Board& board) {
    legacy::Board result;
    result.level = static_cast<int8_t>(board.level());

    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            PanelValue v = board.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j));
            result.panel[i][j] = static_cast<int8_t>(toInt(v));
        }
        result.row_sum[i] = static_cast<int8_t>(board.rowHint(static_cast<uint8_t>(i)).sum);
        result.row_0[i] = static_cast<int8_t>(board.rowHint(static_cast<uint8_t>(i)).voltorbCount);
    }

    for (int j = 0; j < 5; j++) {
        result.column_sum[j] = static_cast<int8_t>(board.colHint(static_cast<uint8_t>(j)).sum);
        result.column_0[j] = static_cast<int8_t>(board.colHint(static_cast<uint8_t>(j)).voltorbCount);
    }

    return result;
}

// Generate compatible boards using the legacy solver
static void runLegacySolver(const legacy::Board& board, std::vector<legacy::Board>& positions,
                            std::vector<legacy::Board> compatible[10],
                            std::vector<ptrdiff_t> compatibleIndices[10]) {
    legacy::generate_0_positions(board, positions);
    legacy::generate_compatible_boards(board, positions, compatible);

    for (int type = 0; type < 10; type++) {
        compatibleIndices[type].clear();
        for (size_t n = 0; n < compatible[type].size(); n++) {
            compatibleIndices[type].push_back(static_cast<ptrdiff_t>(n));
        }
    }
}

// Generate compatible boards using the new solver
static std::vector<Board> runNewSolver(const Board& board) {
    std::vector<Board> result;

    // Generate for all compatible types
    CompatibleBoardGenerator::generateAllTypes(board, [&](const Board& b) {
        result.push_back(b);
        return true; // Continue generating
    });

    return result;
}

// ============ Legacy behavior tests ============

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
                    if (pos.isVoltorb[i][j])
                        count++;
                }
                REQUIRE(count == rowHints[i].voltorbCount);
            }

            // Check column counts
            for (size_t j = 0; j < BOARD_SIZE; j++) {
                int count = 0;
                for (size_t i = 0; i < BOARD_SIZE; i++) {
                    if (pos.isVoltorb[i][j])
                        count++;
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
                        if (val == 0)
                            voltorbs++;
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
                        if (val == 0)
                            voltorbs++;
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

// ============ Probability comparison tests ============

TEST_CASE("Panel probabilities match legacy solver", "[legacy][probability]") {
    BoardGenerator gen(42);

    // Test on multiple levels (level 1-2 for speed)
    for (Level level = 1; level <= 2; level++) {
        SECTION("Level " + std::to_string(level)) {
            // Generate a board
            Board newBoard = gen.generate(level, 0);
            Board covered = newBoard.toCovered();

            // Convert to legacy format
            legacy::Board legacyBoard = toLegacyBoard(covered);

            // Run legacy solver
            std::vector<legacy::Board> voltorbPositions;
            std::vector<legacy::Board> legacyCompatible[10];
            std::vector<ptrdiff_t> legacyIndices[10];
            runLegacySolver(legacyBoard, voltorbPositions, legacyCompatible, legacyIndices);

            auto legacyProbs =
                legacy::calculatePanelProbabilities(legacyBoard, legacyCompatible, legacyIndices);

            // Run new solver
            auto newCompatible = runNewSolver(covered);
            auto newProbs = ProbabilityCalculator::calculate(covered, newCompatible);

            // Compare each unknown panel - should match to machine precision
            for (int i = 0; i < 5; i++) {
                for (int j = 0; j < 5; j++) {
                    if (covered.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j)) ==
                        PanelValue::Unknown) {
                        // Find the corresponding panel in newProbs
                        const PanelProbabilities* newPanelProb = nullptr;
                        for (const auto& pp : newProbs.panels) {
                            if (pp.pos.row == i && pp.pos.col == j) {
                                newPanelProb = &pp;
                                break;
                            }
                        }
                        REQUIRE(newPanelProb != nullptr);

                        INFO("Panel (" << i << ", " << j << ")");
                        CHECK_THAT(newPanelProb->pVoltorb,
                                   WithinAbs(legacyProbs[static_cast<size_t>(i)]
                                                        [static_cast<size_t>(j)]
                                                            .p[0],
                                             1e-10));
                        CHECK_THAT(
                            newPanelProb->pOne,
                            WithinAbs(
                                legacyProbs[static_cast<size_t>(i)][static_cast<size_t>(j)].p[1],
                                1e-10));
                        CHECK_THAT(
                            newPanelProb->pTwo,
                            WithinAbs(
                                legacyProbs[static_cast<size_t>(i)][static_cast<size_t>(j)].p[2],
                                1e-10));
                        CHECK_THAT(
                            newPanelProb->pThree,
                            WithinAbs(
                                legacyProbs[static_cast<size_t>(i)][static_cast<size_t>(j)].p[3],
                                1e-10));
                    }
                }
            }
        }
    }
}

TEST_CASE("Panel probabilities match legacy after partial reveal", "[legacy][probability]") {
    BoardGenerator gen(123);

    // Generate a board and reveal a few panels
    Board newBoard = gen.generate(1, 0);
    Board covered = newBoard.toCovered();

    // Reveal 3 panels (non-voltorb ones)
    int revealed = 0;
    for (int i = 0; i < 5 && revealed < 3; i++) {
        for (int j = 0; j < 5 && revealed < 3; j++) {
            PanelValue actual = newBoard.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j));
            if (actual != PanelValue::Voltorb) {
                covered.set(static_cast<uint8_t>(i), static_cast<uint8_t>(j), actual);
                revealed++;
            }
        }
    }

    // Convert to legacy format
    legacy::Board legacyBoard = toLegacyBoard(covered);

    // Run legacy solver
    std::vector<legacy::Board> voltorbPositions;
    std::vector<legacy::Board> legacyCompatible[10];
    std::vector<ptrdiff_t> legacyIndices[10];
    runLegacySolver(legacyBoard, voltorbPositions, legacyCompatible, legacyIndices);

    auto legacyProbs =
        legacy::calculatePanelProbabilities(legacyBoard, legacyCompatible, legacyIndices);

    // Run new solver
    auto newCompatible = runNewSolver(covered);
    auto newProbs = ProbabilityCalculator::calculate(covered, newCompatible);

    // Compare each unknown panel - should match to machine precision
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            if (covered.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j)) ==
                PanelValue::Unknown) {
                // Find the corresponding panel in newProbs
                const PanelProbabilities* newPanelProb = nullptr;
                for (const auto& pp : newProbs.panels) {
                    if (pp.pos.row == i && pp.pos.col == j) {
                        newPanelProb = &pp;
                        break;
                    }
                }
                REQUIRE(newPanelProb != nullptr);

                INFO("Panel (" << i << ", " << j << ")");
                CHECK_THAT(
                    newPanelProb->pVoltorb,
                    WithinAbs(legacyProbs[static_cast<size_t>(i)][static_cast<size_t>(j)].p[0],
                              1e-10));
                CHECK_THAT(
                    newPanelProb->pOne,
                    WithinAbs(legacyProbs[static_cast<size_t>(i)][static_cast<size_t>(j)].p[1],
                              1e-10));
                CHECK_THAT(
                    newPanelProb->pTwo,
                    WithinAbs(legacyProbs[static_cast<size_t>(i)][static_cast<size_t>(j)].p[2],
                              1e-10));
                CHECK_THAT(
                    newPanelProb->pThree,
                    WithinAbs(legacyProbs[static_cast<size_t>(i)][static_cast<size_t>(j)].p[3],
                              1e-10));
            }
        }
    }
}

TEST_CASE("Suggested panel matches legacy (modulo ties)", "[legacy][suggestion]") {
    BoardGenerator gen(456);

    // Test on a fully covered board - level 1 for speed
    Board newBoard = gen.generate(1, 0);
    Board covered = newBoard.toCovered();

    // Convert to legacy format
    legacy::Board legacyBoard = toLegacyBoard(covered);

    // Run legacy solver
    std::vector<legacy::Board> voltorbPositions;
    std::vector<legacy::Board> legacyCompatible[10];
    std::vector<ptrdiff_t> legacyIndices[10];
    std::vector<legacy::HashItem> legacyHash[25];
    runLegacySolver(legacyBoard, voltorbPositions, legacyCompatible, legacyIndices);

    auto legacyBest =
        legacy::findBestPanel(legacyBoard, legacyCompatible, legacyIndices, legacyHash, 0);

    // Run new solver
    Solver solver;
    auto newResult = solver.solve(covered);

    // Either same panel, or same win probability (tie)
    bool samePanel = (static_cast<int>(newResult.suggestedPosition.row) == legacyBest.row &&
                      static_cast<int>(newResult.suggestedPosition.col) == legacyBest.col);
    bool sameProbability = std::abs(newResult.winProbability - legacyBest.winProbability) < 1e-6;

    INFO("Legacy suggested: (" << legacyBest.row << ", " << legacyBest.col
                               << ") with p=" << legacyBest.winProbability);
    INFO("New suggested: (" << static_cast<int>(newResult.suggestedPosition.row) << ", "
                            << static_cast<int>(newResult.suggestedPosition.col)
                            << ") with p=" << newResult.winProbability);

    // If not same panel, at least the win probabilities should be very close
    // (different panels with equal probability is OK - just a tie-breaking difference)
    if (!samePanel) {
        CHECK_THAT(newResult.winProbability, WithinAbs(legacyBest.winProbability, 1e-4));
    }

    REQUIRE((samePanel || sameProbability));
}

TEST_CASE("Both solvers find compatible boards", "[legacy][compatibility]") {
    BoardGenerator gen(789);

    for (Level level = 1; level <= 2; level++) {
        SECTION("Level " + std::to_string(level)) {
            Board newBoard = gen.generate(level, 0);
            Board covered = newBoard.toCovered();

            // Convert to legacy format
            legacy::Board legacyBoard = toLegacyBoard(covered);

            // Run legacy solver
            std::vector<legacy::Board> voltorbPositions;
            std::vector<legacy::Board> legacyCompatible[10];
            std::vector<ptrdiff_t> legacyIndices[10];
            runLegacySolver(legacyBoard, voltorbPositions, legacyCompatible, legacyIndices);

            // Count total legacy compatible boards
            size_t legacyTotal = 0;
            for (int type = 0; type < 10; type++) {
                legacyTotal += legacyCompatible[type].size();
            }

            // Run new solver
            std::vector<Board> newCompatible;
            CompatibleBoardGenerator::generateAllTypes(covered, [&](const Board& b) {
                newCompatible.push_back(b);
                return true;
            });

            // Both solvers should find at least some compatible boards
            REQUIRE(legacyTotal > 0);
            REQUIRE(newCompatible.size() > 0);

            // The counts should be in the same ballpark (within 2x of each other)
            // Note: The solvers may have subtle differences in their generation algorithms
            double ratio =
                static_cast<double>(newCompatible.size()) / static_cast<double>(legacyTotal);
            INFO("Legacy total: " << legacyTotal);
            INFO("New total: " << newCompatible.size());
            INFO("Ratio (new/legacy): " << ratio);
            CHECK(ratio >= 0.5);
            CHECK(ratio <= 2.0);
        }
    }
}

TEST_CASE("Probabilities sum to 1.0 for unknown panels", "[legacy][probability]") {
    BoardGenerator gen(999);

    Board newBoard = gen.generate(1, 0);
    Board covered = newBoard.toCovered();

    // Run new solver
    auto newCompatible = runNewSolver(covered);
    auto newProbs = ProbabilityCalculator::calculate(covered, newCompatible);

    // Check each panel's probabilities sum to 1.0
    for (const auto& pp : newProbs.panels) {
        double sum = pp.pVoltorb + pp.pOne + pp.pTwo + pp.pThree;
        INFO("Panel (" << static_cast<int>(pp.pos.row) << ", " << static_cast<int>(pp.pos.col)
                       << ")");
        CHECK_THAT(sum, WithinAbs(1.0, 1e-6));
    }
}

// ============ Exact compatible board count comparison ============

TEST_CASE("Compatible board counts match exactly - fully covered boards", "[legacy][exact]") {
    BoardGenerator gen(12345);

    // Test across all levels, multiple boards per level
    for (Level level = 1; level <= MAX_LEVEL; level++) {
        SECTION("Level " + std::to_string(level)) {
            // Test 10 random boards per level
            for (int boardNum = 0; boardNum < 10; boardNum++) {
                BoardTypeIndex type = static_cast<BoardTypeIndex>(boardNum % NUM_TYPES_PER_LEVEL);
                Board newBoard = gen.generate(level, type);
                Board covered = newBoard.toCovered();

                // Convert to legacy format
                legacy::Board legacyBoard = toLegacyBoard(covered);

                // Run legacy solver
                std::vector<legacy::Board> voltorbPositions;
                std::vector<legacy::Board> legacyCompatible[10];
                std::vector<ptrdiff_t> legacyIndices[10];
                runLegacySolver(legacyBoard, voltorbPositions, legacyCompatible, legacyIndices);

                // Run new solver and count per type
                auto newCounts = CompatibleBoardGenerator::countPerType(covered);

                // Compare counts per type - must be EXACTLY equal
                for (BoardTypeIndex t = 0; t < NUM_TYPES_PER_LEVEL; t++) {
                    INFO("Level " << static_cast<int>(level) << ", board " << boardNum
                                  << ", type " << static_cast<int>(t));
                    INFO("Legacy count: " << legacyCompatible[t].size());
                    INFO("New count: " << newCounts[t]);
                    REQUIRE(newCounts[t] == legacyCompatible[t].size());
                }
            }
        }
    }
}

TEST_CASE("Compatible board counts match exactly - partially revealed boards",
          "[legacy][exact][partial]") {
    BoardGenerator gen(67890);

    // Test across multiple levels
    for (Level level = 1; level <= MAX_LEVEL; level++) {
        SECTION("Level " + std::to_string(level)) {
            // Test 5 random boards per level
            for (int boardNum = 0; boardNum < 5; boardNum++) {
                BoardTypeIndex type = static_cast<BoardTypeIndex>(boardNum % NUM_TYPES_PER_LEVEL);
                Board actualBoard = gen.generate(level, type);
                Board covered = actualBoard.toCovered();

                // Reveal varying numbers of panels (1-10 non-voltorb panels)
                int revealCount = 1 + (boardNum * 2);
                int revealed = 0;

                for (int i = 0; i < 5 && revealed < revealCount; i++) {
                    for (int j = 0; j < 5 && revealed < revealCount; j++) {
                        PanelValue actual =
                            actualBoard.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j));
                        if (actual != PanelValue::Voltorb) {
                            covered.set(static_cast<uint8_t>(i), static_cast<uint8_t>(j), actual);
                            revealed++;
                        }
                    }
                }

                // Convert to legacy format
                legacy::Board legacyBoard = toLegacyBoard(covered);

                // Run legacy solver
                std::vector<legacy::Board> voltorbPositions;
                std::vector<legacy::Board> legacyCompatible[10];
                std::vector<ptrdiff_t> legacyIndices[10];
                runLegacySolver(legacyBoard, voltorbPositions, legacyCompatible, legacyIndices);

                // Run new solver and count per type
                auto newCounts = CompatibleBoardGenerator::countPerType(covered);

                // Compare counts per type - must be EXACTLY equal
                for (BoardTypeIndex t = 0; t < NUM_TYPES_PER_LEVEL; t++) {
                    INFO("Level " << static_cast<int>(level) << ", board " << boardNum
                                  << ", revealed " << revealed << ", type " << static_cast<int>(t));
                    INFO("Legacy count: " << legacyCompatible[t].size());
                    INFO("New count: " << newCounts[t]);
                    REQUIRE(newCounts[t] == legacyCompatible[t].size());
                }
            }
        }
    }
}

TEST_CASE("Compatible board counts match exactly - with revealed voltorbs",
          "[legacy][exact][voltorbs]") {
    BoardGenerator gen(11111);

    // Test revealing voltorbs as well as non-voltorbs
    for (Level level = 1; level <= 4; level++) { // Lower levels for speed
        SECTION("Level " + std::to_string(level)) {
            for (int boardNum = 0; boardNum < 3; boardNum++) {
                BoardTypeIndex type = static_cast<BoardTypeIndex>(boardNum % NUM_TYPES_PER_LEVEL);
                Board actualBoard = gen.generate(level, type);
                Board covered = actualBoard.toCovered();

                // Reveal a mix of panels including some voltorbs
                int revealCount = 3 + boardNum;
                int revealed = 0;

                for (int i = 0; i < 5 && revealed < revealCount; i++) {
                    for (int j = 0; j < 5 && revealed < revealCount; j++) {
                        PanelValue actual =
                            actualBoard.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j));
                        // Reveal every other panel we visit
                        if ((i + j) % 2 == 0) {
                            covered.set(static_cast<uint8_t>(i), static_cast<uint8_t>(j), actual);
                            revealed++;
                        }
                    }
                }

                // Convert to legacy format
                legacy::Board legacyBoard = toLegacyBoard(covered);

                // Run legacy solver
                std::vector<legacy::Board> voltorbPositions;
                std::vector<legacy::Board> legacyCompatible[10];
                std::vector<ptrdiff_t> legacyIndices[10];
                runLegacySolver(legacyBoard, voltorbPositions, legacyCompatible, legacyIndices);

                // Run new solver and count per type
                auto newCounts = CompatibleBoardGenerator::countPerType(covered);

                // Compare counts per type - must be EXACTLY equal
                for (BoardTypeIndex t = 0; t < NUM_TYPES_PER_LEVEL; t++) {
                    INFO("Level " << static_cast<int>(level) << ", board " << boardNum
                                  << ", revealed " << revealed << ", type " << static_cast<int>(t));
                    INFO("Legacy count: " << legacyCompatible[t].size());
                    INFO("New count: " << newCounts[t]);
                    REQUIRE(newCounts[t] == legacyCompatible[t].size());
                }
            }
        }
    }
}

TEST_CASE("Total compatible board count matches exactly", "[legacy][exact][total]") {
    BoardGenerator gen(99999);

    // Quick test: 50 random boards across all levels
    for (int testNum = 0; testNum < 50; testNum++) {
        Level level = static_cast<Level>(1 + (testNum % MAX_LEVEL));
        BoardTypeIndex type = static_cast<BoardTypeIndex>(testNum % NUM_TYPES_PER_LEVEL);
        Board actualBoard = gen.generate(level, type);
        Board covered = actualBoard.toCovered();

        // Randomly reveal 0-5 panels
        int revealCount = testNum % 6;
        int revealed = 0;
        for (int i = 0; i < 5 && revealed < revealCount; i++) {
            for (int j = 0; j < 5 && revealed < revealCount; j++) {
                PanelValue actual =
                    actualBoard.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j));
                if (actual != PanelValue::Voltorb) {
                    covered.set(static_cast<uint8_t>(i), static_cast<uint8_t>(j), actual);
                    revealed++;
                }
            }
        }

        // Convert to legacy format
        legacy::Board legacyBoard = toLegacyBoard(covered);

        // Run legacy solver
        std::vector<legacy::Board> voltorbPositions;
        std::vector<legacy::Board> legacyCompatible[10];
        std::vector<ptrdiff_t> legacyIndices[10];
        runLegacySolver(legacyBoard, voltorbPositions, legacyCompatible, legacyIndices);

        // Count legacy total
        size_t legacyTotal = 0;
        for (int t = 0; t < 10; t++) {
            legacyTotal += legacyCompatible[t].size();
        }

        // Run new solver
        auto newCounts = CompatibleBoardGenerator::countPerType(covered);
        size_t newTotal = 0;
        for (size_t c : newCounts) {
            newTotal += c;
        }

        INFO("Test " << testNum << ": Level " << static_cast<int>(level) << ", revealed "
                     << revealed);
        INFO("Legacy total: " << legacyTotal);
        INFO("New total: " << newTotal);
        REQUIRE(newTotal == legacyTotal);
    }
}
