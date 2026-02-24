// Trace exactly what happens when filling the target voltorb configuration
#include "legacy_solver.hpp"
#include "voltorb/board_type.hpp"
#include "voltorb/constraints.hpp"
#include "voltorb/generator.hpp"

#include <algorithm>
#include <iostream>
#include <set>
#include <iomanip>

using namespace voltorb;

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

static std::string legacyBoardToString(const legacy::Board& board) {
    std::string s;
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            if (board.panel[i][j] == -1) s += "?";
            else s += std::to_string(board.panel[i][j]);
        }
    }
    return s;
}

static void printLegacyBoard(const legacy::Board& board, const std::string& prefix = "") {
    for (int i = 0; i < 5; i++) {
        std::cout << prefix;
        for (int j = 0; j < 5; j++) {
            if (board.panel[i][j] == -1) std::cout << "? ";
            else std::cout << (int)board.panel[i][j] << " ";
        }
        std::cout << "| sum=" << (int)board.row_sum[i] << " v=" << (int)board.row_0[i] << "\n";
    }
    std::cout << prefix << "─────────\n";
    std::cout << prefix;
    for (int j = 0; j < 5; j++) std::cout << (int)board.column_sum[j] << " ";
    std::cout << "\n" << prefix;
    for (int j = 0; j < 5; j++) std::cout << (int)board.column_0[j] << " ";
    std::cout << "\n";
}

static auto getVoltorbPattern(const std::string& s) {
    std::string pattern;
    for (char c : s) {
        pattern += (c == '0') ? "V" : ".";
    }
    return pattern;
}

int main() {
    BoardGenerator gen(12345);

    for (int boardNum = 0; boardNum < 4; boardNum++) {
        gen.generate(2, boardNum % 10);
    }
    Board actualBoard = gen.generate(2, 4);
    Board covered = actualBoard.toCovered();
    legacy::Board legacyBoard = toLegacyBoard(covered);

    std::string target = "1113011111101210031030101";
    std::string targetPattern = getVoltorbPattern(target);

    std::cout << "# Tracing the Fill Process for Target Voltorb Configuration\n\n";

    // Generate all voltorb positions
    std::vector<legacy::Board> voltorbPositions;
    legacy::generate_0_positions(legacyBoard, voltorbPositions);

    // Find the target voltorb config
    int targetIdx = -1;
    for (size_t k = 0; k < voltorbPositions.size(); k++) {
        std::string actualPattern;
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 5; j++) {
                actualPattern += (voltorbPositions[k].panel[i][j] == 0) ? "V" : ".";
            }
        }
        if (actualPattern == targetPattern) {
            targetIdx = k;
            break;
        }
    }

    std::cout << "Target voltorb config found at index " << targetIdx << "\n\n";
    std::cout << "Starting board:\n```\n";
    printLegacyBoard(voltorbPositions[targetIdx], "  ");
    std::cout << "```\n\n";

    // Now manually trace what generate_compatible_boards does with JUST this config
    std::cout << "## Running generate_compatible_boards with ONLY the target voltorb config\n\n";

    std::vector<legacy::Board> singleConfig;
    singleConfig.push_back(voltorbPositions[targetIdx]);

    std::vector<legacy::Board> compatible[10];
    legacy::generate_compatible_boards(legacyBoard, singleConfig, compatible);

    std::cout << "Results for type 0: " << compatible[0].size() << " boards\n\n";

    if (compatible[0].empty()) {
        std::cout << "**NO BOARDS GENERATED!** The algorithm failed to produce any boards.\n\n";
    } else {
        std::cout << "Boards generated:\n";
        for (const auto& b : compatible[0]) {
            std::string s = legacyBoardToString(b);
            std::cout << "- " << s;
            if (s == target) std::cout << " ← TARGET!";
            std::cout << "\n";
        }
        std::cout << "\n";

        // Check voltorb patterns
        std::cout << "Voltorb patterns of generated boards:\n";
        std::set<std::string> patterns;
        for (const auto& b : compatible[0]) {
            std::string s = legacyBoardToString(b);
            patterns.insert(getVoltorbPattern(s));
        }
        for (const auto& p : patterns) {
            std::cout << "```\n";
            for (int i = 0; i < 5; i++) {
                std::cout << "  ";
                for (int j = 0; j < 5; j++) {
                    std::cout << p[i*5+j] << " ";
                }
                std::cout << "\n";
            }
            std::cout << "```\n";
        }

        if (patterns.find(targetPattern) == patterns.end()) {
            std::cout << "\n**The target voltorb pattern is NOT in the output!**\n";
            std::cout << "The filling process changed the voltorb positions!\n";
        }
    }

    // Let's check why the target config produces 0 boards
    std::cout << "\n## Why Does Target Config Produce 0 Boards?\n\n";

    // Check compatible_type for type 0
    std::cout << "Checking compatible_type for target config and type 0:\n";
    bool isCompat = legacy::compatible_type(voltorbPositions[targetIdx], 0);
    std::cout << "- compatible_type result: " << (isCompat ? "PASS" : "FAIL") << "\n\n";

    if (!isCompat) {
        // Check what specifically fails
        const auto& vp = voltorbPositions[targetIdx];
        std::cout << "Checking individual conditions:\n";

        // Check total voltorbs
        int totalVoltorbs = vp.row_0[0] + vp.row_0[1] + vp.row_0[2] + vp.row_0[3] + vp.row_0[4];
        std::cout << "- Total voltorbs from hints: " << totalVoltorbs;
        std::cout << " (type 0 needs " << legacy::board_type[1][0].n_0 << "): ";
        std::cout << (totalVoltorbs == legacy::board_type[1][0].n_0 ? "OK" : "FAIL") << "\n";

        // Check total sum
        int totalSum = vp.row_sum[0] + vp.row_sum[1] + vp.row_sum[2] + vp.row_sum[3] + vp.row_sum[4];
        int expectedSum = 25 - legacy::board_type[1][0].n_0 +
                          legacy::board_type[1][0].n_2 +
                          2 * legacy::board_type[1][0].n_3;
        std::cout << "- Total sum from hints: " << totalSum;
        std::cout << " (type 0 expects " << expectedSum << "): ";
        std::cout << (totalSum == expectedSum ? "OK" : "FAIL") << "\n";

        // Check legality
        bool legal = legacy::check_legality_board(legacy::board_type[1][0], vp);
        std::cout << "- check_legality_board: " << (legal ? "PASS" : "FAIL") << "\n";
    }

    // Let's trace what happens inside generate_compatible_boards for this config
    std::cout << "\n## Tracing generate_compatible_boards for target config\n\n";

    // Manually do what generate_compatible_boards does
    std::cout << "Step 1: Check compatible_type(real_board, type=0)\n";
    bool realBoardCompat = legacy::compatible_type(legacyBoard, 0);
    std::cout << "- Result: " << (realBoardCompat ? "PASS" : "FAIL") << "\n\n";

    if (realBoardCompat) {
        std::cout << "Step 2: Copy voltorb positions to compatible_boards[0]\n";
        std::cout << "- Copying target config...\n\n";

        std::cout << "Step 3: Merge revealed panels\n";
        legacy::Board merged = voltorbPositions[targetIdx];
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 5; j++) {
                if (legacyBoard.panel[i][j] != -1) {
                    if (merged.panel[i][j] == -1) {
                        merged.panel[i][j] = legacyBoard.panel[i][j];
                    } else if (merged.panel[i][j] != legacyBoard.panel[i][j]) {
                        std::cout << "- CONFLICT at (" << i << "," << j << ")!\n";
                    }
                }
            }
        }
        std::cout << "- No conflicts (board is all unknown)\n\n";

        std::cout << "Step 4: Fill non-voltorbs iteratively\n";
        std::cout << "Starting partial board:\n```\n";
        printLegacyBoard(merged, "  ");
        std::cout << "```\n\n";

        // Check if any row/column can be filled
        std::cout << "Analyzing fillable rows/columns:\n";
        for (int i = 0; i < 5; i++) {
            int unknowns = 0, remaining = merged.row_sum[i];
            for (int j = 0; j < 5; j++) {
                if (merged.panel[i][j] == -1) unknowns++;
                else if (merged.panel[i][j] > 0) remaining -= merged.panel[i][j];
            }
            if (unknowns > 0) {
                std::cout << "- Row " << i << ": " << unknowns << " unknowns, need sum " << remaining << "\n";
            }
        }
        for (int j = 0; j < 5; j++) {
            int unknowns = 0, remaining = merged.column_sum[j];
            for (int i = 0; i < 5; i++) {
                if (merged.panel[i][j] == -1) unknowns++;
                else if (merged.panel[i][j] > 0) remaining -= merged.panel[i][j];
            }
            if (unknowns > 0) {
                std::cout << "- Col " << j << ": " << unknowns << " unknowns, need sum " << remaining << "\n";
            }
        }
    }

    // Now let's run with ALL configs and see which ones produce type 0 boards
    std::cout << "\n## Analysis: Which voltorb configs produce type 0 boards?\n\n";

    std::cout << "### Individual runs (one config at a time):\n\n";
    int totalIndividual = 0;
    for (size_t k = 0; k < voltorbPositions.size(); k++) {
        std::vector<legacy::Board> oneConfig;
        oneConfig.push_back(voltorbPositions[k]);

        std::vector<legacy::Board> compat[10];
        legacy::generate_compatible_boards(legacyBoard, oneConfig, compat);

        std::string inputPattern;
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 5; j++) {
                inputPattern += (voltorbPositions[k].panel[i][j] == 0) ? "V" : ".";
            }
        }

        std::cout << "Config " << std::setw(2) << k << ": " << std::setw(2) << compat[0].size() << " boards";
        if (k == (size_t)targetIdx) std::cout << " [TARGET CONFIG]";

        if (!compat[0].empty()) {
            totalIndividual += compat[0].size();
            std::set<std::string> outputPatterns;
            for (const auto& b : compat[0]) {
                outputPatterns.insert(getVoltorbPattern(legacyBoardToString(b)));
            }
            bool patternPreserved = (outputPatterns.size() == 1 && *outputPatterns.begin() == inputPattern);
            if (!patternPreserved) {
                std::cout << " **PATTERN CHANGED!**";
            }
        }
        std::cout << "\n";
    }
    std::cout << "\nTotal from individual runs: " << totalIndividual << " boards\n\n";

    // Now run with ALL configs together
    std::cout << "### Joint run (all configs together):\n\n";
    std::vector<legacy::Board> allCompat[10];
    legacy::generate_compatible_boards(legacyBoard, voltorbPositions, allCompat);
    std::cout << "Total from joint run: " << allCompat[0].size() << " boards\n\n";

    if (totalIndividual != (int)allCompat[0].size()) {
        std::cout << "**DISCREPANCY!** Individual runs give " << totalIndividual
                  << " boards but joint run gives " << allCompat[0].size() << " boards.\n";
        std::cout << "Difference: " << (int)allCompat[0].size() - totalIndividual << " boards\n\n";

        // Find boards in joint but not in individual
        std::set<std::string> individualBoards;
        for (size_t k = 0; k < voltorbPositions.size(); k++) {
            std::vector<legacy::Board> oneConfig;
            oneConfig.push_back(voltorbPositions[k]);
            std::vector<legacy::Board> compat[10];
            legacy::generate_compatible_boards(legacyBoard, oneConfig, compat);
            for (const auto& b : compat[0]) {
                individualBoards.insert(legacyBoardToString(b));
            }
        }

        std::set<std::string> jointBoards;
        for (const auto& b : allCompat[0]) {
            jointBoards.insert(legacyBoardToString(b));
        }

        std::cout << "Boards in JOINT but not in INDIVIDUAL:\n";
        for (const auto& s : jointBoards) {
            if (individualBoards.find(s) == individualBoards.end()) {
                std::cout << "- " << s << " (voltorb pattern: " << getVoltorbPattern(s) << ")\n";
            }
        }
        std::cout << "\n";

        std::cout << "Boards in INDIVIDUAL but not in JOINT:\n";
        for (const auto& s : individualBoards) {
            if (jointBoards.find(s) == jointBoards.end()) {
                std::cout << "- " << s << " (voltorb pattern: " << getVoltorbPattern(s) << ")\n";
            }
        }
    }

    return 0;
}
