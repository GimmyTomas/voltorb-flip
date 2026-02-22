#pragma once

#include "types.hpp"

#include <array>
#include <cstdint>

namespace voltorb {

// Parameters for a board type
struct BoardTypeParams {
    int8_t n0;           // Number of voltorbs
    int8_t n2;           // Number of 2s
    int8_t n3;           // Number of 3s
    int8_t maxTotalFree; // Max multipliers in free rows/cols total
    int8_t maxRowFree;   // Max multipliers in any single free row/col

    // Derived: number of 1s = 25 - n0 - n2 - n3
    constexpr int8_t n1() const {
        return static_cast<int8_t>(TOTAL_PANELS - n0 - n2 - n3);
    }

    // Total sum of all panel values
    constexpr int totalSum() const {
        return n1() + 2 * n2 + 3 * n3;
    }
};

// Static lookup tables for board type data
class BoardTypeData {
public:
    // Get parameters for a specific level and type
    static const BoardTypeParams& params(Level level, BoardTypeIndex type);

    // Get the number of permutations (25!/(n0!n1!n2!n3!))
    static int64_t permutations(Level level, BoardTypeIndex type);

    // Get the number of accepted (valid) boards for a type
    static int64_t acceptedCount(Level level, BoardTypeIndex type);

    // Get GCD for reduced calculations
    static int64_t permutationsGcd(Level level);
    static int64_t acceptedGcd(Level level);

    // Get reduced permutation count (permutations / gcd)
    static int reducedPermutations(Level level, BoardTypeIndex type);

    // Get reduced accepted count (accepted / gcd)
    static int64_t reducedAccepted(Level level, BoardTypeIndex type);

    // Check if a board type is compatible with given total voltorb count and sum
    static bool isCompatible(Level level, BoardTypeIndex type, int totalVoltorbs, int totalSum);

private:
    BoardTypeData() = delete;

    // Lookup tables
    static const std::array<std::array<BoardTypeParams, NUM_TYPES_PER_LEVEL>, MAX_LEVEL>
        paramsTable_;
    static const std::array<std::array<int64_t, NUM_TYPES_PER_LEVEL>, MAX_LEVEL> permutationsTable_;
    static const std::array<std::array<int64_t, NUM_TYPES_PER_LEVEL>, MAX_LEVEL> acceptedTable_;
    static const std::array<int64_t, MAX_LEVEL> permutationsGcdTable_;
    static const std::array<int64_t, MAX_LEVEL> acceptedGcdTable_;
    static const std::array<std::array<int, NUM_TYPES_PER_LEVEL>, MAX_LEVEL>
        reducedPermutationsTable_;
    static const std::array<std::array<int64_t, NUM_TYPES_PER_LEVEL>, MAX_LEVEL>
        reducedAcceptedTable_;
};

} // namespace voltorb
