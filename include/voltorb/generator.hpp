#pragma once

#include "board.hpp"
#include "board_type.hpp"

#include <random>

namespace voltorb {

// Generates random valid boards using rejection sampling
class BoardGenerator {
public:
    explicit BoardGenerator(uint64_t seed = 0);

    // Generate a random valid board for a level (random type)
    Board generate(Level level);

    // Generate a random valid board for a specific level and type
    Board generate(Level level, BoardTypeIndex type);

    // Get the number of rejection attempts from last generation
    size_t lastRejectionCount() const { return lastRejectionCount_; }

    // Reseed the generator
    void seed(uint64_t seed);

private:
    std::mt19937_64 rng_;
    size_t lastRejectionCount_ = 0;

    // Check if board passes legality constraints
    bool isLegal(const Board& board, const BoardTypeParams& params) const;
};

} // namespace voltorb
