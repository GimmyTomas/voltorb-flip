// Debug script to investigate discrepancy between legacy and new solver
#include "legacy_solver.hpp"
#include "voltorb/board_type.hpp"
#include "voltorb/constraints.hpp"
#include "voltorb/generator.hpp"

#include <algorithm>
#include <iostream>
#include <set>

using namespace voltorb;

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

// Convert new Board to a canonical string for set comparison
static std::string boardToString(const Board& board) {
    std::string s;
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            s += std::to_string(toInt(board.get(i, j)));
        }
    }
    return s;
}

static std::string legacyBoardToString(const legacy::Board& board) {
    std::string s;
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            s += std::to_string(board.panel[i][j]);
        }
    }
    return s;
}

int main() {
    BoardGenerator gen(12345);

    // Find the first discrepancy
    for (Level level = 1; level <= 8; level++) {
        for (int boardNum = 0; boardNum < 10; boardNum++) {
            BoardTypeIndex genType = static_cast<BoardTypeIndex>(boardNum % 10);
            Board actualBoard = gen.generate(level, genType);
            Board covered = actualBoard.toCovered();

            // Convert to legacy format
            legacy::Board legacyBoard = toLegacyBoard(covered);

            // Run legacy solver
            std::vector<legacy::Board> voltorbPositions;
            std::vector<legacy::Board> legacyCompatible[10];
            legacy::generate_0_positions(legacyBoard, voltorbPositions);
            legacy::generate_compatible_boards(legacyBoard, voltorbPositions, legacyCompatible);

            // Run new solver
            auto newCounts = CompatibleBoardGenerator::countPerType(covered);

            // Check for discrepancy
            for (BoardTypeIndex t = 0; t < 10; t++) {
                if (newCounts[t] != legacyCompatible[t].size()) {
                    std::cout << "DISCREPANCY FOUND!\n";
                    std::cout << "Level " << (int)level << ", board " << boardNum
                              << ", generated as type " << (int)genType
                              << ", checking type " << (int)t << "\n";
                    std::cout << "Legacy count: " << legacyCompatible[t].size() << "\n";
                    std::cout << "New count: " << newCounts[t] << "\n\n";

                    std::cout << "Board hints:\n";
                    for (int i = 0; i < 5; i++) {
                        std::cout << "Row " << i << ": sum=" << (int)covered.rowHint(i).sum
                                  << " voltorbs=" << (int)covered.rowHint(i).voltorbCount << "\n";
                    }
                    for (int j = 0; j < 5; j++) {
                        std::cout << "Col " << j << ": sum=" << (int)covered.colHint(j).sum
                                  << " voltorbs=" << (int)covered.colHint(j).voltorbCount << "\n";
                    }

                    // Collect boards from both
                    std::set<std::string> newBoards;
                    CompatibleBoardGenerator::generate(covered, t, [&](const Board& b) {
                        newBoards.insert(boardToString(b));
                        return true;
                    });

                    std::set<std::string> legacyBoards;
                    for (const auto& b : legacyCompatible[t]) {
                        legacyBoards.insert(legacyBoardToString(b));
                    }

                    std::cout << "\nBoards in NEW but not in LEGACY:\n";
                    int count = 0;
                    for (const auto& s : newBoards) {
                        if (legacyBoards.find(s) == legacyBoards.end()) {
                            std::cout << "  " << s << "\n";

                            // Verify this board
                            Board testBoard(level);
                            for (int i = 0; i < 5; i++) {
                                testBoard.setRowHint(i, covered.rowHint(i));
                                testBoard.setColHint(i, covered.colHint(i));
                            }
                            for (int i = 0; i < 5; i++) {
                                for (int j = 0; j < 5; j++) {
                                    int val = s[i * 5 + j] - '0';
                                    testBoard.set(i, j, static_cast<PanelValue>(val));
                                }
                            }

                            const auto& params = BoardTypeData::params(level, t);
                            bool newLegal = LegalityChecker::isLegal(testBoard, params);
                            legacy::Board legacyTest = toLegacyBoard(testBoard);
                            bool legacyLegal = legacy::compatible_type(legacyTest, t);

                            std::cout << "    New isLegal: " << (newLegal ? "PASS" : "FAIL")
                                      << ", Legacy compatible_type: " << (legacyLegal ? "PASS" : "FAIL")
                                      << "\n";

                            count++;
                            if (count >= 5) break;
                        }
                    }

                    std::cout << "\nBoards in LEGACY but not in NEW:\n";
                    count = 0;
                    for (const auto& s : legacyBoards) {
                        if (newBoards.find(s) == newBoards.end()) {
                            std::cout << "  " << s << "\n";
                            count++;
                            if (count >= 5) break;
                        }
                    }

                    return 0;
                }
            }
        }
    }

    std::cout << "No discrepancies found!\n";
    return 0;

    // Old code below for reference
    Board covered(1);
    std::cout << "Testing Level 2, board 4, type 0\n";
    std::cout << "Actual board hints:\n";
    for (int i = 0; i < 5; i++) {
        std::cout << "Row " << i << ": sum=" << (int)covered.rowHint(i).sum
                  << " voltorbs=" << (int)covered.rowHint(i).voltorbCount << "\n";
    }
    for (int j = 0; j < 5; j++) {
        std::cout << "Col " << j << ": sum=" << (int)covered.colHint(j).sum
                  << " voltorbs=" << (int)covered.colHint(j).voltorbCount << "\n";
    }

    // Get compatible boards from new solver for type 0
    std::set<std::string> newBoards;
    CompatibleBoardGenerator::generate(covered, 0, [&](const Board& b) {
        newBoards.insert(boardToString(b));
        return true;
    });

    // Get compatible boards from legacy solver for type 0
    legacy::Board legacyBoard = toLegacyBoard(covered);
    std::vector<legacy::Board> voltorbPositions;
    std::vector<legacy::Board> legacyCompatible[10];

    legacy::generate_0_positions(legacyBoard, voltorbPositions);
    legacy::generate_compatible_boards(legacyBoard, voltorbPositions, legacyCompatible);

    std::set<std::string> legacyBoards;
    for (const auto& b : legacyCompatible[0]) {
        legacyBoards.insert(legacyBoardToString(b));
    }

    std::cout << "\nNew solver found: " << newBoards.size() << " boards for type 0\n";
    std::cout << "Legacy solver found: " << legacyBoards.size() << " boards for type 0\n";

    // Find boards in new but not in legacy
    std::cout << "\nBoards in NEW but not in LEGACY:\n";
    int count = 0;
    for (const auto& s : newBoards) {
        if (legacyBoards.find(s) == legacyBoards.end()) {
            std::cout << "  " << s << "\n";
            count++;
            if (count > 10) {
                std::cout << "  ... (showing first 10)\n";
                break;
            }
        }
    }

    // Find boards in legacy but not in new
    std::cout << "\nBoards in LEGACY but not in NEW:\n";
    count = 0;
    for (const auto& s : legacyBoards) {
        if (newBoards.find(s) == newBoards.end()) {
            std::cout << "  " << s << "\n";
            count++;
            if (count > 10) {
                std::cout << "  ... (showing first 10)\n";
                break;
            }
        }
    }

    // Verify one of the "extra" boards from new solver is valid
    if (!newBoards.empty()) {
        for (const auto& s : newBoards) {
            if (legacyBoards.find(s) == legacyBoards.end()) {
                std::cout << "\nVerifying extra board: " << s << "\n";

                // Reconstruct the board
                Board testBoard(2);
                for (int i = 0; i < 5; i++) {
                    testBoard.setRowHint(i, covered.rowHint(i));
                    testBoard.setColHint(i, covered.colHint(i));
                }
                for (int i = 0; i < 5; i++) {
                    for (int j = 0; j < 5; j++) {
                        int val = s[i * 5 + j] - '0';
                        testBoard.set(i, j, static_cast<PanelValue>(val));
                    }
                }

                // Check constraints
                const auto& params = BoardTypeData::params(2, 0);
                std::cout << "Type 0 params: n0=" << (int)params.n0 << " n2=" << (int)params.n2
                          << " n3=" << (int)params.n3 << "\n";

                // Count values
                int n0 = 0, n1 = 0, n2 = 0, n3 = 0;
                for (int i = 0; i < 5; i++) {
                    for (int j = 0; j < 5; j++) {
                        int val = s[i * 5 + j] - '0';
                        if (val == 0) n0++;
                        else if (val == 1) n1++;
                        else if (val == 2) n2++;
                        else if (val == 3) n3++;
                    }
                }
                std::cout << "Board counts: n0=" << n0 << " n1=" << n1
                          << " n2=" << n2 << " n3=" << n3 << "\n";

                // Check row/col sums
                bool sumsMatch = true;
                for (int i = 0; i < 5; i++) {
                    int rowSum = 0, rowV = 0;
                    for (int j = 0; j < 5; j++) {
                        int val = s[i * 5 + j] - '0';
                        rowSum += val;
                        if (val == 0) rowV++;
                    }
                    if (rowSum != covered.rowHint(i).sum || rowV != covered.rowHint(i).voltorbCount) {
                        std::cout << "Row " << i << " mismatch: sum=" << rowSum << " (expected "
                                  << (int)covered.rowHint(i).sum << "), voltorbs=" << rowV
                                  << " (expected " << (int)covered.rowHint(i).voltorbCount << ")\n";
                        sumsMatch = false;
                    }
                }
                for (int j = 0; j < 5; j++) {
                    int colSum = 0, colV = 0;
                    for (int i = 0; i < 5; i++) {
                        int val = s[i * 5 + j] - '0';
                        colSum += val;
                        if (val == 0) colV++;
                    }
                    if (colSum != covered.colHint(j).sum || colV != covered.colHint(j).voltorbCount) {
                        std::cout << "Col " << j << " mismatch: sum=" << colSum << " (expected "
                                  << (int)covered.colHint(j).sum << "), voltorbs=" << colV
                                  << " (expected " << (int)covered.colHint(j).voltorbCount << ")\n";
                        sumsMatch = false;
                    }
                }

                if (sumsMatch) {
                    std::cout << "Row/col sums and voltorb counts: MATCH\n";
                }

                std::cout << "Legality check: "
                          << (LegalityChecker::isLegal(testBoard, params) ? "PASS" : "FAIL") << "\n";

                // Also check with legacy
                legacy::Board legacyTest = toLegacyBoard(testBoard);
                std::cout << "Legacy compatible_type: "
                          << (legacy::compatible_type(legacyTest, 0) ? "PASS" : "FAIL") << "\n";

                break;
            }
        }
    }

    return 0;
}
