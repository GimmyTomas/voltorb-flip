// Trace the legacy algorithm step by step to see where boards get lost
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
            s += std::to_string(board.panel[i][j]);
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
        std::cout << "\n";
    }
}

// Count unknown cells in a row
static int countUnknownsInRow(const legacy::Board& b, int row) {
    int count = 0;
    for (int j = 0; j < 5; j++) {
        if (b.panel[row][j] == -1) count++;
    }
    return count;
}

// Count unknown cells in a column
static int countUnknownsInCol(const legacy::Board& b, int col) {
    int count = 0;
    for (int i = 0; i < 5; i++) {
        if (b.panel[i][col] == -1) count++;
    }
    return count;
}

// Calculate remaining sum needed for a row
static int remainingSumForRow(const legacy::Board& b, int row) {
    int filled = 0;
    for (int j = 0; j < 5; j++) {
        if (b.panel[row][j] > 0) filled += b.panel[row][j];
    }
    return b.row_sum[row] - filled;
}

// Calculate remaining sum needed for a column
static int remainingSumForCol(const legacy::Board& b, int col) {
    int filled = 0;
    for (int i = 0; i < 5; i++) {
        if (b.panel[i][col] > 0) filled += b.panel[i][col];
    }
    return b.column_sum[col] - filled;
}

int main() {
    BoardGenerator gen(12345);

    // Generate Level 2, board 4
    for (int boardNum = 0; boardNum < 4; boardNum++) {
        gen.generate(2, boardNum % 10);
    }
    Board actualBoard = gen.generate(2, 4);
    Board covered = actualBoard.toCovered();
    legacy::Board legacyBoard = toLegacyBoard(covered);

    // One of the missing boards
    std::string target = "1113011111101210031030101";

    std::cout << "# Tracing Legacy Algorithm\n\n";
    std::cout << "Target board (MISSING from legacy): " << target << "\n";
    std::cout << "```\n";
    std::cout << "  1 1 1 3 0\n";
    std::cout << "  1 1 1 1 1\n";
    std::cout << "  1 0 1 2 1\n";
    std::cout << "  0 0 3 1 0\n";
    std::cout << "  3 0 1 0 1\n";
    std::cout << "```\n\n";

    // Step 1: Generate voltorb positions
    std::cout << "## Step 1: Generate Voltorb Positions\n\n";
    std::vector<legacy::Board> voltorbPositions;
    legacy::generate_0_positions(legacyBoard, voltorbPositions);
    std::cout << "Found " << voltorbPositions.size() << " valid voltorb configurations.\n\n";

    // Find the voltorb configuration that matches the target board
    std::string targetVoltorbs;
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            targetVoltorbs += (target[i*5+j] == '0') ? "1" : "0";
        }
    }
    std::cout << "Target voltorb pattern (1=voltorb): " << targetVoltorbs << "\n";

    int matchingVoltorbConfig = -1;
    for (size_t k = 0; k < voltorbPositions.size(); k++) {
        std::string s;
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 5; j++) {
                s += (voltorbPositions[k].panel[i][j] == 0) ? "1" : "0";
            }
        }
        if (s == targetVoltorbs) {
            matchingVoltorbConfig = k;
            break;
        }
    }

    if (matchingVoltorbConfig >= 0) {
        std::cout << "Found matching voltorb config at index " << matchingVoltorbConfig << "\n";
        std::cout << "```\n";
        printLegacyBoard(voltorbPositions[matchingVoltorbConfig], "  ");
        std::cout << "```\n";
        std::cout << "(0 = voltorb, -1 = unknown)\n\n";
    } else {
        std::cout << "ERROR: No matching voltorb configuration found!\n";
        return 1;
    }

    // Step 2: Trace generation for type 0
    std::cout << "## Step 2: Fill Non-Voltorbs for Type 0\n\n";

    // Start with just the matching voltorb config
    std::vector<legacy::Board> workQueue;
    workQueue.push_back(voltorbPositions[matchingVoltorbConfig]);

    int iteration = 0;
    std::set<std::string> generatedBoards;

    while (!workQueue.empty()) {
        // Find first partial board (not fully flipped)
        int partialIdx = -1;
        for (size_t i = 0; i < workQueue.size(); i++) {
            bool hasUnknown = false;
            for (int r = 0; r < 5 && !hasUnknown; r++) {
                for (int c = 0; c < 5 && !hasUnknown; c++) {
                    if (workQueue[i].panel[r][c] == -1) hasUnknown = true;
                }
            }
            if (hasUnknown) {
                partialIdx = i;
                break;
            }
        }

        if (partialIdx == -1) break; // All boards are complete

        iteration++;
        std::cout << "### Iteration " << iteration << "\n\n";
        std::cout << "Queue size: " << workQueue.size() << "\n";
        std::cout << "Processing board at index " << partialIdx << ":\n";
        std::cout << "```\n";
        printLegacyBoard(workQueue[partialIdx], "  ");
        std::cout << "```\n";

        // Find the row/column with fewest combinations
        int bestRow = -1, bestCol = -1;
        int minCombinations = 999999;

        for (int i = 0; i < 5; i++) {
            int unknowns = countUnknownsInRow(workQueue[partialIdx], i);
            if (unknowns > 0) {
                int remaining = remainingSumForRow(workQueue[partialIdx], i);
                // Simple estimate of combinations
                int combos = 1;
                if (unknowns > 0) {
                    for (int n3 = std::max(0, remaining - 2*unknowns); n3 <= (remaining - unknowns) / 2; n3++) {
                        combos++;
                    }
                }
                if (combos < minCombinations) {
                    minCombinations = combos;
                    bestRow = i;
                    bestCol = -1;
                }
            }
        }

        for (int j = 0; j < 5; j++) {
            int unknowns = countUnknownsInCol(workQueue[partialIdx], j);
            if (unknowns > 0) {
                int remaining = remainingSumForCol(workQueue[partialIdx], j);
                int combos = 1;
                if (unknowns > 0) {
                    for (int n3 = std::max(0, remaining - 2*unknowns); n3 <= (remaining - unknowns) / 2; n3++) {
                        combos++;
                    }
                }
                if (combos < minCombinations) {
                    minCombinations = combos;
                    bestRow = -1;
                    bestCol = j;
                }
            }
        }

        if (bestRow >= 0) {
            std::cout << "Choosing to fill **Row " << bestRow << "** (fewest combinations: " << minCombinations << ")\n";
            int unknowns = countUnknownsInRow(workQueue[partialIdx], bestRow);
            int remaining = remainingSumForRow(workQueue[partialIdx], bestRow);
            std::cout << "- Unknown cells: " << unknowns << "\n";
            std::cout << "- Remaining sum needed: " << remaining << "\n\n";
        } else if (bestCol >= 0) {
            std::cout << "Choosing to fill **Column " << bestCol << "** (fewest combinations: " << minCombinations << ")\n";
            int unknowns = countUnknownsInCol(workQueue[partialIdx], bestCol);
            int remaining = remainingSumForCol(workQueue[partialIdx], bestCol);
            std::cout << "- Unknown cells: " << unknowns << "\n";
            std::cout << "- Remaining sum needed: " << remaining << "\n\n";
        }

        // Now simulate what find_and_fill_row_with_possibilities does
        // This is complex, so let's just run the actual legacy code and capture results
        std::vector<legacy::Board> newBoards;

        // Run the legacy algorithm from this point
        std::vector<legacy::Board> compatible[10];
        compatible[0] = workQueue;

        // Process one step
        auto p = [](const legacy::Board& b) {
            for (int i = 0; i < 5; i++)
                for (int j = 0; j < 5; j++)
                    if (b.panel[i][j] == -1) return true;
            return false;
        };

        size_t beforeSize = compatible[0].size();
        auto it = std::find_if(compatible[0].begin(), compatible[0].end(), p);

        // We need to extract just the filling part... this is getting complex.
        // Let's just run the full legacy algorithm and see what we get.
        break;
    }

    // Run full legacy algorithm
    std::cout << "## Full Legacy Algorithm Results\n\n";
    std::vector<legacy::Board> compatible[10];
    legacy::generate_compatible_boards(legacyBoard, voltorbPositions, compatible);

    std::cout << "Legacy found " << compatible[0].size() << " boards for type 0\n\n";

    // Check if target is in the results
    bool found = false;
    for (const auto& b : compatible[0]) {
        if (legacyBoardToString(b) == target) {
            found = true;
            break;
        }
    }
    std::cout << "Target board " << (found ? "FOUND" : "**NOT FOUND**") << " in legacy results.\n\n";

    // Let's look for similar boards that ARE found
    std::cout << "## Similar Boards That ARE Found\n\n";
    std::cout << "Looking for boards that differ only in rows 3-4 from target...\n\n";

    for (const auto& b : compatible[0]) {
        std::string s = legacyBoardToString(b);
        // Check if first 15 chars match (rows 0-2)
        if (s.substr(0, 15) == target.substr(0, 15)) {
            std::cout << "Board: " << s << "\n";
            std::cout << "```\n";
            for (int i = 0; i < 5; i++) {
                std::cout << "  ";
                for (int j = 0; j < 5; j++) {
                    std::cout << s[i*5+j] << " ";
                }
                if (i >= 3) std::cout << " ← differs here";
                std::cout << "\n";
            }
            std::cout << "```\n\n";
        }
    }

    // Check what voltorb configurations led to the found boards vs missing boards
    std::cout << "## Voltorb Configuration Analysis\n\n";

    auto getVoltorbPattern = [](const std::string& s) {
        std::string pattern;
        for (char c : s) {
            pattern += (c == '0') ? "V" : ".";
        }
        return pattern;
    };

    std::string targetPattern = getVoltorbPattern(target);
    std::cout << "Target voltorb pattern:\n```\n";
    for (int i = 0; i < 5; i++) {
        std::cout << "  ";
        for (int j = 0; j < 5; j++) {
            std::cout << targetPattern[i*5+j] << " ";
        }
        std::cout << "\n";
    }
    std::cout << "```\n\n";

    // Find all boards in legacy results with the SAME voltorb pattern as target
    std::cout << "Boards with SAME voltorb pattern as target:\n";
    int samePatternCount = 0;
    for (const auto& b : compatible[0]) {
        std::string s = legacyBoardToString(b);
        if (getVoltorbPattern(s) == targetPattern) {
            samePatternCount++;
            std::cout << "- " << s << "\n";
        }
    }
    if (samePatternCount == 0) {
        std::cout << "**NONE FOUND!**\n";
    }
    std::cout << "\n";

    // List all unique voltorb patterns in the legacy results
    std::set<std::string> uniquePatterns;
    for (const auto& b : compatible[0]) {
        std::string s = legacyBoardToString(b);
        uniquePatterns.insert(getVoltorbPattern(s));
    }
    std::cout << "All unique voltorb patterns in legacy results (" << uniquePatterns.size() << " patterns):\n";
    int patternIdx = 0;
    for (const auto& p : uniquePatterns) {
        patternIdx++;
        std::cout << "\nPattern " << patternIdx << ":\n```\n";
        for (int i = 0; i < 5; i++) {
            std::cout << "  ";
            for (int j = 0; j < 5; j++) {
                std::cout << p[i*5+j] << " ";
            }
            std::cout << "\n";
        }
        std::cout << "```\n";

        // Count boards with this pattern
        int count = 0;
        for (const auto& b : compatible[0]) {
            if (getVoltorbPattern(legacyBoardToString(b)) == p) count++;
        }
        std::cout << "(" << count << " boards with this pattern)\n";
    }

    // Check if target voltorb pattern was in the generated voltorb positions
    std::cout << "\n## Was Target Voltorb Pattern Generated?\n\n";
    std::string targetVoltorbStr;
    for (char c : target) {
        targetVoltorbStr += (c == '0') ? "0" : "-1";
        targetVoltorbStr += " ";
    }

    bool patternGenerated = false;
    for (const auto& vp : voltorbPositions) {
        std::string vpPattern = getVoltorbPattern(legacyBoardToString(vp).substr(0, 25));
        // Need to convert: in voltorbPositions, 0 means voltorb, -1 means unknown
        std::string actualPattern;
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 5; j++) {
                actualPattern += (vp.panel[i][j] == 0) ? "V" : ".";
            }
        }
        if (actualPattern == targetPattern) {
            patternGenerated = true;
            std::cout << "YES! Target voltorb pattern was generated.\n\n";
            break;
        }
    }
    if (!patternGenerated) {
        std::cout << "NO! Target voltorb pattern was NOT generated.\n";
        std::cout << "This explains why the board is missing!\n\n";
    }

    return 0;
}
