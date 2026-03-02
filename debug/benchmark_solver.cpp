// Benchmark for the Voltorb Flip solver
// Generates boards at various levels, partially reveals safe panels,
// and measures per-depth timing to identify performance bottlenecks.

#include "voltorb/board.hpp"
#include "voltorb/constraints.hpp"
#include "voltorb/generator.hpp"
#include "voltorb/probability.hpp"
#include "voltorb/solver.hpp"

#include <chrono>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace voltorb;

struct BenchmarkCase {
    std::string name;
    Board board;
    size_t compatibleCount;
};

// Generate a board and partially reveal safe panels to create a mid-game state
static BenchmarkCase makeCase(const std::string& name, Level level, uint64_t seed,
                               int revealCount) {
    BoardGenerator gen(seed);
    Board fullBoard = gen.generate(level);

    // Create a covered (unknown) version with hints
    Board board = fullBoard.toCovered();

    // Reveal some panels that are NOT voltorbs (simulating safe plays)
    int revealed = 0;
    for (uint8_t r = 0; r < BOARD_SIZE && revealed < revealCount; r++) {
        for (uint8_t c = 0; c < BOARD_SIZE && revealed < revealCount; c++) {
            PanelValue trueVal = fullBoard.get(r, c);
            if (trueVal == PanelValue::One) {
                board.set(r, c, trueVal);
                revealed++;
            }
        }
    }

    // Count compatible boards
    size_t count = 0;
    CompatibleBoardGenerator::generateAllTypes(board, [&](const Board&) {
        count++;
        return count <= 500000;
    });

    return {name, board, std::min(count, size_t(500000))};
}

static void runBenchmark(const BenchmarkCase& bc, int timeoutMs) {
    std::cout << "\n========================================\n";
    std::cout << "Case: " << bc.name << "\n";
    std::cout << "Level: " << static_cast<int>(bc.board.level())
              << "  Unknown panels: " << bc.board.countUnknown()
              << "  Compatible boards: " << bc.compatibleCount << "\n";
    std::cout << bc.board.toString() << "\n";
    std::cout << "----------------------------------------\n";
    std::cout << std::setw(6) << "Depth"
              << std::setw(14) << "Time(ms)"
              << std::setw(14) << "Cumul(ms)"
              << std::setw(14) << "Nodes"
              << std::setw(12) << "WinProb"
              << std::setw(8) << "Exact"
              << std::setw(12) << "Panel"
              << "\n";
    std::cout << "----------------------------------------\n";

    auto wallStart = std::chrono::steady_clock::now();
    auto prevTime = wallStart;

    SolverOptions opts;
    opts.timeout = std::chrono::milliseconds(timeoutMs);
    opts.maxCompatibleBoards = 500000;

    Solver solver(opts);

    SolverResult result = solver.solve(bc.board,
        [&](const SolverProgress& progress) {
            auto now = std::chrono::steady_clock::now();
            auto depthMs = std::chrono::duration_cast<std::chrono::microseconds>(
                now - prevTime).count() / 1000.0;
            auto cumulMs = std::chrono::duration_cast<std::chrono::microseconds>(
                now - wallStart).count() / 1000.0;
            prevTime = now;

            std::cout << std::setw(6) << progress.depth
                      << std::setw(14) << std::fixed << std::setprecision(1) << depthMs
                      << std::setw(14) << std::fixed << std::setprecision(1) << cumulMs
                      << std::setw(14) << progress.nodesSearched
                      << std::setw(12) << std::fixed << std::setprecision(4)
                      << progress.winProbability
                      << std::setw(8) << (progress.isExact ? "yes" : "no")
                      << std::setw(5) << "R" << static_cast<int>(progress.bestPanel.row)
                      << "C" << static_cast<int>(progress.bestPanel.col)
                      << "\n";
        }
    );

    auto wallEnd = std::chrono::steady_clock::now();
    double totalMs = std::chrono::duration_cast<std::chrono::microseconds>(
        wallEnd - wallStart).count() / 1000.0;

    std::cout << "----------------------------------------\n";
    std::cout << "Total time:    " << std::fixed << std::setprecision(1) << totalMs << " ms\n";
    std::cout << "Final depth:   " << result.searchDepth << "\n";
    std::cout << "Win prob:      " << std::fixed << std::setprecision(6) << result.winProbability << "\n";
    std::cout << "Exact:         " << (result.isExact ? "yes" : "no") << "\n";
    std::cout << "Nodes:         " << solver.getNodesEvaluated() << "\n";
    std::cout << "Cache hits:    " << solver.getCacheHits() << "\n";
    std::cout << "Cache misses:  " << solver.getCacheMisses() << "\n";
    if (result.reason) {
        std::cout << "Reason:        " << *result.reason << "\n";
    }
    std::cout << "Suggested:     R" << static_cast<int>(result.suggestedPosition.row)
              << "C" << static_cast<int>(result.suggestedPosition.col) << "\n";
}

int main(int argc, char* argv[]) {
    int timeoutMs = 10000;
    if (argc > 1) {
        timeoutMs = std::atoi(argv[1]);
    }

    std::cout << "Voltorb Flip Solver Benchmark\n";
    std::cout << "Timeout per case: " << timeoutMs << " ms\n";

    std::vector<BenchmarkCase> cases;

    // Level 6 cases — typical hard game
    cases.push_back(makeCase("L6-seed42-reveal3", 6, 42, 3));
    cases.push_back(makeCase("L6-seed42-reveal6", 6, 42, 6));
    cases.push_back(makeCase("L6-seed99-reveal3", 6, 99, 3));
    cases.push_back(makeCase("L6-seed99-reveal6", 6, 99, 6));

    // Level 8 cases — hardest level
    cases.push_back(makeCase("L8-seed42-reveal3", 8, 42, 3));
    cases.push_back(makeCase("L8-seed42-reveal6", 8, 42, 6));
    cases.push_back(makeCase("L8-seed77-reveal3", 8, 77, 3));

    // A nearly-solved board (few unknowns remaining)
    cases.push_back(makeCase("L6-seed42-reveal10", 6, 42, 10));

    for (const auto& bc : cases) {
        runBenchmark(bc, timeoutMs);
    }

    std::cout << "\n========================================\n";
    std::cout << "Benchmark complete.\n";

    return 0;
}
