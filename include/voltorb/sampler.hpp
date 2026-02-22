#pragma once

#include "board.hpp"

#include <random>
#include <vector>

namespace voltorb {

// Monte Carlo sampler for approximating compatible board distribution
// Used as fallback when exhaustive enumeration is too slow
class MonteCarloSampler {
public:
    explicit MonteCarloSampler(uint64_t seed = 0);

    // Sample N compatible boards weighted by type probability
    std::vector<Board> sample(const Board& board, size_t numSamples);

    // Sample and estimate panel probabilities directly
    struct SampledProbabilities {
        std::vector<Position> unknownPanels;
        std::vector<std::array<double, 4>> probs; // [voltorb, 1, 2, 3] per panel
        size_t samplesUsed;
    };

    SampledProbabilities sampleProbabilities(const Board& board, size_t numSamples);

    // Estimate whether panel is safe (p_voltorb < threshold)
    bool isProbablySafe(const Board& board, Position pos, size_t numSamples,
                        double threshold = 0.001);

private:
    std::mt19937_64 rng_;

    // Sample a single compatible board
    std::optional<Board> sampleOne(const Board& board, int maxAttempts = 100);

    // Generate random board of a specific type
    Board generateRandomOfType(Level level, BoardTypeIndex type);

    // Check if generated board is compatible with constraints
    bool isCompatible(const Board& generated, const Board& constraints) const;

    // Sample type weighted by acceptance probability
    BoardTypeIndex sampleType(Level level);
};

} // namespace voltorb
