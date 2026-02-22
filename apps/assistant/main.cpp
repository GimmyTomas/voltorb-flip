#include "voltorb/board.hpp"
#include "voltorb/game.hpp"
#include "voltorb/probability.hpp"
#include "voltorb/solver.hpp"
#include "voltorb/constraints.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace voltorb;

void printUsage() {
    std::cout << "\nVoltorb Flip Assistant - Commands:\n"
              << "  new <level>     - Start a new game at given level (1-8)\n"
              << "  hint <r> <sum> <voltorbs>   - Set row hint (0-indexed)\n"
              << "  hintc <c> <sum> <voltorbs>  - Set column hint (0-indexed)\n"
              << "  flip <r> <c> <value>        - Reveal a panel (value 0-3)\n"
              << "  undo            - Undo last flip/hint command\n"
              << "  solve           - Get best panel to flip\n"
              << "  probs           - Show probabilities for all panels\n"
              << "  safe            - Show all guaranteed safe panels\n"
              << "  board           - Display current board\n"
              << "  help            - Show this help\n"
              << "  quit            - Exit\n\n";
}

void printBoard(const Board& board, std::optional<Position> highlight = std::nullopt) {
    std::cout << board.toString(highlight) << std::endl;
}

void printProbabilities(const Board& board) {
    std::vector<Board> compatibleBoards;
    CompatibleBoardGenerator::generateAllTypes(board, [&](const Board& b) {
        compatibleBoards.push_back(b);
        return compatibleBoards.size() < 100000;
    });

    if (compatibleBoards.empty()) {
        std::cout << "No compatible boards found. Check your inputs.\n";
        return;
    }

    auto probs = ProbabilityCalculator::calculate(board, compatibleBoards);

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "\nPanel Probabilities:\n";
    std::cout << "Pos     P(0)    P(1)    P(2)    P(3)    P(mult)\n";
    std::cout << "------------------------------------------------\n";

    for (const auto& p : probs.panels) {
        std::cout << "(" << static_cast<int>(p.pos.row) << "," << static_cast<int>(p.pos.col)
                  << ")   " << std::setw(6) << p.pVoltorb << "  " << std::setw(6) << p.pOne << "  "
                  << std::setw(6) << p.pTwo << "  " << std::setw(6) << p.pThree << "  "
                  << std::setw(6) << p.pMultiplier() << "\n";
    }

    std::cout << "\nCompatible boards: " << probs.totalCompatible << "\n";
    std::cout << "Type probabilities: ";
    for (size_t i = 0; i < NUM_TYPES_PER_LEVEL; i++) {
        if (probs.typeProbs[i] > 0.001) {
            std::cout << "T" << i << ":" << std::setprecision(2) << probs.typeProbs[i] << " ";
        }
    }
    std::cout << "\n\n";
}

int main() {
    std::cout << "=================================\n";
    std::cout << "   Voltorb Flip Solver v1.0\n";
    std::cout << "=================================\n";

    printUsage();

    Board board;
    Solver solver;
    solver.clearCache();

    // History stack for undo
    std::vector<Board> history;

    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) {
            break;
        }

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "quit" || cmd == "exit" || cmd == "q") {
            break;
        } else if (cmd == "help" || cmd == "h" || cmd == "?") {
            printUsage();
        } else if (cmd == "new") {
            int level;
            if (iss >> level && level >= 1 && level <= 8) {
                history.clear();  // Clear history on new game
                board = Board(static_cast<Level>(level));
                std::cout << "Started new game at level " << level << "\n";
                std::cout << "Enter row hints with: hint <row> <sum> <voltorbs>\n";
                std::cout << "Enter column hints with: hintc <col> <sum> <voltorbs>\n";
            } else {
                std::cout << "Usage: new <level> (1-8)\n";
            }
        } else if (cmd == "hint") {
            int row, sum, voltorbs;
            if (iss >> row >> sum >> voltorbs && row >= 0 && row < 5) {
                history.push_back(board);  // Save state before modification
                board.setRowHint(static_cast<uint8_t>(row),
                                 {static_cast<uint8_t>(sum), static_cast<uint8_t>(voltorbs)});
                std::cout << "Set row " << row << " hint: sum=" << sum
                          << ", voltorbs=" << voltorbs << "\n";
            } else {
                std::cout << "Usage: hint <row> <sum> <voltorbs>\n";
            }
        } else if (cmd == "hintc") {
            int col, sum, voltorbs;
            if (iss >> col >> sum >> voltorbs && col >= 0 && col < 5) {
                history.push_back(board);  // Save state before modification
                board.setColHint(static_cast<uint8_t>(col),
                                 {static_cast<uint8_t>(sum), static_cast<uint8_t>(voltorbs)});
                std::cout << "Set column " << col << " hint: sum=" << sum
                          << ", voltorbs=" << voltorbs << "\n";
            } else {
                std::cout << "Usage: hintc <col> <sum> <voltorbs>\n";
            }
        } else if (cmd == "flip") {
            int row, col, value;
            if (iss >> row >> col >> value && row >= 0 && row < 5 && col >= 0 && col < 5 &&
                value >= 0 && value <= 3) {
                history.push_back(board);  // Save state before modification
                board.set(static_cast<uint8_t>(row), static_cast<uint8_t>(col),
                          static_cast<PanelValue>(value));
                std::cout << "Revealed (" << row << "," << col << ") = " << value << "\n";

                if (value == 0) {
                    std::cout << "*** VOLTORB! Game over. ***\n";
                }
            } else {
                std::cout << "Usage: flip <row> <col> <value> (value: 0-3)\n";
            }
        } else if (cmd == "undo") {
            if (history.empty()) {
                std::cout << "Nothing to undo.\n";
            } else {
                board = history.back();
                history.pop_back();
                std::cout << "Undone. Current state:\n";
                printBoard(board);
            }
        } else if (cmd == "solve") {
            auto result = solver.solve(board);

            std::cout << "\nSuggested panel: (" << static_cast<int>(result.suggestedPosition.row)
                      << "," << static_cast<int>(result.suggestedPosition.col) << ")\n";
            std::cout << "Win probability: " << std::fixed << std::setprecision(2)
                      << (result.winProbability * 100.0) << "%\n";
            std::cout << "Method: " << (result.isExact ? "Exact" : "Monte Carlo (approximate)")
                      << "\n";
            std::cout << "Boards evaluated: " << result.boardsEvaluated << "\n";
            std::cout << "Compute time: " << result.computeTime.count() << "ms\n";

            if (result.reason) {
                std::cout << "Reason: " << *result.reason << "\n";
            }

            printBoard(board, result.suggestedPosition);
        } else if (cmd == "probs") {
            printProbabilities(board);
        } else if (cmd == "safe") {
            auto safePanels = solver.findSafePanels(board);

            if (safePanels.empty()) {
                std::cout << "No guaranteed safe panels found.\n";
            } else {
                std::cout << "Safe panels: ";
                for (const auto& pos : safePanels) {
                    std::cout << "(" << static_cast<int>(pos.row) << ","
                              << static_cast<int>(pos.col) << ") ";
                }
                std::cout << "\n";
            }
        } else if (cmd == "board") {
            printBoard(board);
        } else if (!cmd.empty()) {
            std::cout << "Unknown command: " << cmd << "\n";
            std::cout << "Type 'help' for available commands.\n";
        }
    }

    std::cout << "Goodbye!\n";
    return 0;
}
