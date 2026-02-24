// Investigate Level 2, board 4, type 0 discrepancy
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

static void printBoardGrid(const std::string& s) {
    for (int i = 0; i < 5; i++) {
        std::cout << "  ";
        for (int j = 0; j < 5; j++) {
            std::cout << s[i * 5 + j] << " ";
        }
        std::cout << "\n";
    }
}

int main() {
    // In Catch2 with SECTIONs, each section re-runs with fresh generator
    // So Level 2 section starts fresh at seed 12345
    BoardGenerator gen(12345);

    // Skip Level 2 boards 0-3 (generator starts fresh for this section)
    for (int boardNum = 0; boardNum < 4; boardNum++) {
        BoardTypeIndex type = static_cast<BoardTypeIndex>(boardNum % 10);
        gen.generate(2, type);
    }

    // Generate Level 2, board 4 (generated with type 4)
    Board actualBoard = gen.generate(2, 4);
    Board covered = actualBoard.toCovered();

    // Convert to legacy format
    legacy::Board legacyBoard = toLegacyBoard(covered);

    // Run legacy solver
    std::vector<legacy::Board> voltorbPositions;
    std::vector<legacy::Board> legacyCompatible[10];
    legacy::generate_0_positions(legacyBoard, voltorbPositions);
    legacy::generate_compatible_boards(legacyBoard, voltorbPositions, legacyCompatible);

    // Run new solver for type 0
    std::set<std::string> newBoardsSet;
    std::vector<std::string> newBoardsList;
    CompatibleBoardGenerator::generate(covered, 0, [&](const Board& b) {
        std::string s = boardToString(b);
        if (newBoardsSet.find(s) == newBoardsSet.end()) {
            newBoardsSet.insert(s);
            newBoardsList.push_back(s);
        }
        return true;
    });

    std::set<std::string> legacyBoardsSet;
    std::vector<std::string> legacyBoardsList;
    for (const auto& b : legacyCompatible[0]) {
        std::string s = legacyBoardToString(b);
        if (legacyBoardsSet.find(s) == legacyBoardsSet.end()) {
            legacyBoardsSet.insert(s);
            legacyBoardsList.push_back(s);
        }
    }

    // Sort for easier comparison
    std::sort(newBoardsList.begin(), newBoardsList.end());
    std::sort(legacyBoardsList.begin(), legacyBoardsList.end());

    // Output
    std::cout << "# Level 2, Board 4, Type 0 Analysis\n\n";

    std::cout << "## Board Hints\n\n";
    std::cout << "```\n";
    std::cout << "     Col:  0  1  2  3  4\n";
    std::cout << "         +---------------+\n";
    for (int i = 0; i < 5; i++) {
        std::cout << "  Row " << i << ": |  ?  ?  ?  ?  ? | sum="
                  << (int)covered.rowHint(i).sum
                  << ", voltorbs=" << (int)covered.rowHint(i).voltorbCount << "\n";
    }
    std::cout << "         +---------------+\n";
    std::cout << "    sum:   ";
    for (int j = 0; j < 5; j++) {
        std::cout << std::setw(2) << (int)covered.colHint(j).sum << " ";
    }
    std::cout << "\n";
    std::cout << "      v:   ";
    for (int j = 0; j < 5; j++) {
        std::cout << std::setw(2) << (int)covered.colHint(j).voltorbCount << " ";
    }
    std::cout << "\n```\n\n";

    // Type 0 params
    const auto& params = BoardTypeData::params(2, 0);
    std::cout << "## Type 0 Parameters (Level 2)\n\n";
    std::cout << "- n0 (voltorbs) = " << (int)params.n0 << "\n";
    std::cout << "- n1 (ones) = " << (int)params.n1() << "\n";
    std::cout << "- n2 (twos) = " << (int)params.n2 << "\n";
    std::cout << "- n3 (threes) = " << (int)params.n3 << "\n";
    std::cout << "- maxTotalFree = " << (int)params.maxTotalFree << "\n";
    std::cout << "- maxRowFree = " << (int)params.maxRowFree << "\n";
    std::cout << "- totalSum = " << params.totalSum() << "\n\n";

    // Check compatibility
    int totalVoltorbs = 0, totalSum = 0;
    for (int i = 0; i < 5; i++) {
        totalVoltorbs += covered.rowHint(i).voltorbCount;
        totalSum += covered.rowHint(i).sum;
    }
    std::cout << "Board totals: voltorbs=" << totalVoltorbs << ", sum=" << totalSum << "\n";
    std::cout << "Type 0 expects: voltorbs=" << (int)params.n0 << ", sum=" << params.totalSum() << "\n";
    std::cout << "Compatible: " << (totalVoltorbs == params.n0 && totalSum == params.totalSum() ? "YES" : "NO") << "\n\n";

    std::cout << "## Results\n\n";
    std::cout << "- **New solver**: " << newBoardsList.size() << " compatible boards\n";
    std::cout << "- **Legacy solver**: " << legacyBoardsList.size() << " compatible boards\n\n";

    // Find differences
    std::vector<std::string> inNewOnly, inLegacyOnly;
    for (const auto& s : newBoardsList) {
        if (legacyBoardsSet.find(s) == legacyBoardsSet.end()) {
            inNewOnly.push_back(s);
        }
    }
    for (const auto& s : legacyBoardsList) {
        if (newBoardsSet.find(s) == newBoardsSet.end()) {
            inLegacyOnly.push_back(s);
        }
    }

    std::cout << "## Boards Found ONLY by New Solver (" << inNewOnly.size() << ")\n\n";
    for (const auto& s : inNewOnly) {
        std::cout << "### Board: " << s << "\n```\n";
        printBoardGrid(s);
        std::cout << "```\n";

        // Verify
        int n0=0, n1=0, n2=0, n3=0;
        for (char c : s) {
            if (c == '0') n0++;
            else if (c == '1') n1++;
            else if (c == '2') n2++;
            else if (c == '3') n3++;
        }
        std::cout << "Counts: n0=" << n0 << ", n1=" << n1 << ", n2=" << n2 << ", n3=" << n3 << "\n";

        // Verify row/col sums
        std::cout << "Row sums: ";
        for (int i = 0; i < 5; i++) {
            int sum = 0;
            for (int j = 0; j < 5; j++) sum += s[i*5+j] - '0';
            std::cout << sum << " ";
        }
        std::cout << "(expected: ";
        for (int i = 0; i < 5; i++) std::cout << (int)covered.rowHint(i).sum << " ";
        std::cout << ")\n";

        std::cout << "Col sums: ";
        for (int j = 0; j < 5; j++) {
            int sum = 0;
            for (int i = 0; i < 5; i++) sum += s[i*5+j] - '0';
            std::cout << sum << " ";
        }
        std::cout << "(expected: ";
        for (int j = 0; j < 5; j++) std::cout << (int)covered.colHint(j).sum << " ";
        std::cout << ")\n";

        // Check legality with both
        Board testBoard(2);
        for (int i = 0; i < 5; i++) {
            testBoard.setRowHint(i, covered.rowHint(i));
            testBoard.setColHint(i, covered.colHint(i));
        }
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 5; j++) {
                testBoard.set(i, j, static_cast<PanelValue>(s[i*5+j] - '0'));
            }
        }
        legacy::Board legTest = toLegacyBoard(testBoard);

        std::cout << "New isLegal: " << (LegalityChecker::isLegal(testBoard, params) ? "PASS" : "FAIL") << "\n";
        std::cout << "Legacy compatible_type: " << (legacy::compatible_type(legTest, 0) ? "PASS" : "FAIL") << "\n\n";
    }

    if (!inLegacyOnly.empty()) {
        std::cout << "## Boards Found ONLY by Legacy Solver (" << inLegacyOnly.size() << ")\n\n";
        for (const auto& s : inLegacyOnly) {
            std::cout << "### Board: " << s << "\n```\n";
            printBoardGrid(s);
            std::cout << "```\n\n";
        }
    }

    std::cout << "## All Compatible Boards (sorted)\n\n";
    std::cout << "### New Solver (" << newBoardsList.size() << " boards)\n\n";
    std::cout << "```\n";
    int idx = 1;
    for (const auto& s : newBoardsList) {
        std::cout << std::setw(2) << idx++ << ". " << s;
        if (legacyBoardsSet.find(s) == legacyBoardsSet.end()) {
            std::cout << "  <-- MISSING FROM LEGACY";
        }
        std::cout << "\n";
    }
    std::cout << "```\n\n";

    std::cout << "### Legacy Solver (" << legacyBoardsList.size() << " boards)\n\n";
    std::cout << "```\n";
    idx = 1;
    for (const auto& s : legacyBoardsList) {
        std::cout << std::setw(2) << idx++ << ". " << s;
        if (newBoardsSet.find(s) == newBoardsSet.end()) {
            std::cout << "  <-- MISSING FROM NEW";
        }
        std::cout << "\n";
    }
    std::cout << "```\n";

    return 0;
}
