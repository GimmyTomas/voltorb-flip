#include "voltorb/sampler.hpp"

#include "voltorb/board_type.hpp"
#include "voltorb/constraints.hpp"

#include <algorithm>
#include <numeric>

namespace voltorb {

MonteCarloSampler::MonteCarloSampler(uint64_t seed) : rng_(seed) {
    if (seed == 0) {
        std::random_device rd;
        rng_.seed(rd());
    }
}

BoardTypeIndex MonteCarloSampler::sampleType(Level level) {
    // Weight by acceptance count (proportional to P(type))
    std::array<double, NUM_TYPES_PER_LEVEL> weights;
    double totalWeight = 0.0;

    for (BoardTypeIndex type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
        weights[type] = static_cast<double>(BoardTypeData::acceptedCount(level, type));
        totalWeight += weights[type];
    }

    std::uniform_real_distribution<double> dist(0.0, totalWeight);
    double r = dist(rng_);

    double cumulative = 0.0;
    for (BoardTypeIndex type = 0; type < NUM_TYPES_PER_LEVEL; type++) {
        cumulative += weights[type];
        if (r <= cumulative) {
            return type;
        }
    }

    return NUM_TYPES_PER_LEVEL - 1;
}

Board MonteCarloSampler::generateRandomOfType(Level level, BoardTypeIndex type) {
    const auto& params = BoardTypeData::params(level, type);

    Board board(level);

    // Create array of all positions
    std::array<size_t, TOTAL_PANELS> positions;
    std::iota(positions.begin(), positions.end(), 0);

    // Shuffle positions
    std::shuffle(positions.begin(), positions.end(), rng_);

    // Initialize all panels to 1
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            board.set(static_cast<uint8_t>(i), static_cast<uint8_t>(j), PanelValue::One);
        }
    }

    // Place voltorbs
    size_t idx = 0;
    for (int i = 0; i < params.n0; i++) {
        Position pos = Position::fromIndex(positions[idx++]);
        board.set(pos, PanelValue::Voltorb);
    }

    // Place 2s
    for (int i = 0; i < params.n2; i++) {
        Position pos = Position::fromIndex(positions[idx++]);
        board.set(pos, PanelValue::Two);
    }

    // Place 3s
    for (int i = 0; i < params.n3; i++) {
        Position pos = Position::fromIndex(positions[idx++]);
        board.set(pos, PanelValue::Three);
    }

    // Calculate hints
    board.recalculateHints();

    return board;
}

bool MonteCarloSampler::isCompatible(const Board& generated, const Board& constraints) const {
    // Check hints match
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        if (generated.rowHint(static_cast<uint8_t>(i)) !=
            constraints.rowHint(static_cast<uint8_t>(i))) {
            return false;
        }
        if (generated.colHint(static_cast<uint8_t>(i)) !=
            constraints.colHint(static_cast<uint8_t>(i))) {
            return false;
        }
    }

    // Check revealed panels match
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            PanelValue constraint =
                constraints.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j));
            if (isKnown(constraint)) {
                PanelValue actual =
                    generated.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j));
                if (actual != constraint) {
                    return false;
                }
            }
        }
    }

    return true;
}

std::optional<Board> MonteCarloSampler::sampleOne(const Board& board, int maxAttempts) {
    Level level = board.level();

    for (int attempt = 0; attempt < maxAttempts; attempt++) {
        BoardTypeIndex type = sampleType(level);
        Board generated = generateRandomOfType(level, type);

        // Check legality
        const auto& params = BoardTypeData::params(level, type);
        if (!LegalityChecker::isLegal(generated, params)) {
            continue;
        }

        // Check compatibility with constraints
        if (isCompatible(generated, board)) {
            return generated;
        }
    }

    return std::nullopt;
}

std::vector<Board> MonteCarloSampler::sample(const Board& board, size_t numSamples) {
    std::vector<Board> result;
    result.reserve(numSamples);

    // Increase max attempts for harder boards
    int attemptsPerSample = 1000;

    for (size_t i = 0; i < numSamples * 10 && result.size() < numSamples; i++) {
        auto sampled = sampleOne(board, attemptsPerSample);
        if (sampled) {
            result.push_back(*sampled);
        }
    }

    return result;
}

MonteCarloSampler::SampledProbabilities
MonteCarloSampler::sampleProbabilities(const Board& board, size_t numSamples) {
    SampledProbabilities result;

    // Find unknown panels
    for (size_t i = 0; i < BOARD_SIZE; i++) {
        for (size_t j = 0; j < BOARD_SIZE; j++) {
            if (board.get(static_cast<uint8_t>(i), static_cast<uint8_t>(j)) == PanelValue::Unknown) {
                result.unknownPanels.push_back(
                    {static_cast<uint8_t>(i), static_cast<uint8_t>(j)});
            }
        }
    }

    result.probs.resize(result.unknownPanels.size(), {0.0, 0.0, 0.0, 0.0});

    // Sample boards
    auto samples = sample(board, numSamples);
    result.samplesUsed = samples.size();

    if (samples.empty()) {
        return result;
    }

    // Count occurrences
    for (const Board& s : samples) {
        for (size_t idx = 0; idx < result.unknownPanels.size(); idx++) {
            Position pos = result.unknownPanels[idx];
            int value = toInt(s.get(pos));
            if (value >= 0 && value <= 3) {
                result.probs[idx][static_cast<size_t>(value)] += 1.0;
            }
        }
    }

    // Normalize to probabilities
    double n = static_cast<double>(samples.size());
    for (auto& probs : result.probs) {
        for (double& p : probs) {
            p /= n;
        }
    }

    return result;
}

bool MonteCarloSampler::isProbablySafe(const Board& board, Position pos, size_t numSamples,
                                       double threshold) {
    auto samples = sample(board, numSamples);

    if (samples.empty()) {
        return false;
    }

    size_t voltorbCount = 0;
    for (const Board& s : samples) {
        if (s.get(pos) == PanelValue::Voltorb) {
            voltorbCount++;
        }
    }

    double pVoltorb = static_cast<double>(voltorbCount) / static_cast<double>(samples.size());
    return pVoltorb < threshold;
}

} // namespace voltorb
