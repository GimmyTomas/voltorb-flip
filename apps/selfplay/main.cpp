#include "voltorb/game.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>

using namespace voltorb;

void printStats(const SelfPlayStats& stats, Level level = 0) {
    std::cout << std::fixed << std::setprecision(2);

    std::cout << "\n=== Self-Play Statistics ===\n";
    std::cout << "Games played: " << stats.gamesPlayed << "\n";
    std::cout << "Games won:    " << stats.gamesWon << " (" << (stats.winRate() * 100) << "%)\n";
    std::cout << "Games lost:   " << stats.gamesLost << "\n";
    std::cout << "Total coins:  " << stats.totalCoins << "\n";

    if (level == 0) {
        // Per-level breakdown
        std::cout << "\nPer-level statistics:\n";
        std::cout << "Level   Wins    Losses  Win%    Exact   Sampled\n";
        std::cout << "------------------------------------------------\n";
        for (Level l = 1; l <= MAX_LEVEL; l++) {
            size_t total = stats.winsPerLevel[l - 1] + stats.lossesPerLevel[l - 1];
            if (total > 0) {
                std::cout << std::setw(5) << static_cast<int>(l) << "   " << std::setw(6)
                          << stats.winsPerLevel[l - 1] << "  " << std::setw(6)
                          << stats.lossesPerLevel[l - 1] << "  " << std::setw(5)
                          << (stats.winRateForLevel(l) * 100) << "%  " << std::setw(6)
                          << stats.exactSolves[l - 1] << "  " << std::setw(6)
                          << stats.sampledSolves[l - 1] << "\n";
            }
        }
    }

    std::cout << "\n";
}

int main(int argc, char* argv[]) {
    std::cout << "=================================\n";
    std::cout << "   Voltorb Flip Self-Play\n";
    std::cout << "=================================\n\n";

    size_t numGames = 100;
    Level specificLevel = 0; // 0 = all levels with progression
    bool verbose = false;
    uint64_t seed = 0;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-n" && i + 1 < argc) {
            numGames = static_cast<size_t>(std::stoi(argv[++i]));
        } else if (arg == "-l" && i + 1 < argc) {
            specificLevel = static_cast<Level>(std::stoi(argv[++i]));
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "-s" && i + 1 < argc) {
            seed = static_cast<uint64_t>(std::stoll(argv[++i]));
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  -n <num>    Number of games to play (default: 100)\n";
            std::cout << "  -l <level>  Play only at specific level (1-8, default: progression)\n";
            std::cout << "  -v          Verbose output\n";
            std::cout << "  -s <seed>   Random seed\n";
            std::cout << "  -h          Show this help\n";
            return 0;
        }
    }

    SelfPlayer player(seed);

    // Configure solver options
    SolverOptions options;
    options.timeout = std::chrono::milliseconds(5000);
    options.maxCompatibleBoards = 100000;
    options.sampleSize = 10000;
    player.setSolverOptions(options);

    std::cout << "Running " << numGames << " games";
    if (specificLevel > 0) {
        std::cout << " at level " << static_cast<int>(specificLevel);
    }
    std::cout << "...\n\n";

    auto startTime = std::chrono::steady_clock::now();

    SelfPlayStats stats;
    if (specificLevel > 0) {
        stats = player.playAtLevel(specificLevel, numGames, verbose);
    } else {
        stats = player.play(numGames, verbose);
    }

    auto endTime = std::chrono::steady_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    printStats(stats, specificLevel);

    std::cout << "Total time: " << duration << "ms";
    if (numGames > 0) {
        std::cout << " (" << (duration / numGames) << "ms per game)";
    }
    std::cout << "\n";

    return 0;
}
