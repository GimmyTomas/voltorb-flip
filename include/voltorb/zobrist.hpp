#pragma once

#include "types.hpp"

#include <array>
#include <cstdint>
#include <random>

namespace voltorb {

/**
 * Zobrist hashing for board states.
 *
 * Zobrist hashing enables O(1) incremental hash updates when panels are revealed.
 * Instead of recomputing the entire hash, we XOR out the old value and XOR in the new.
 *
 * Each (position, value) pair has a pre-computed random 64-bit number.
 * The hash of a board state is the XOR of all relevant random numbers.
 *
 * For Voltorb Flip:
 * - 25 positions (5x5 board)
 * - 5 values per position: Unknown(-1), Voltorb(0), One(1), Two(2), Three(3)
 * - Plus row/column hints (optional, for full state hashing)
 */
class ZobristHasher {
public:
    /**
     * Initialize the Zobrist tables with deterministic random values.
     * Uses a fixed seed for reproducibility across runs.
     */
    static void initialize(uint64_t seed = 0xDEADBEEFCAFEBABEULL);

    /**
     * Check if the hasher has been initialized.
     */
    static bool isInitialized() { return initialized_; }

    /**
     * Compute the full Zobrist hash for a board's panel values only.
     * This is used for memoization during recursive search.
     * Note: Does NOT include hints - those are fixed during a game.
     */
    static uint64_t hashPanels(const PanelGrid& panels);

    /**
     * Incrementally update a hash when a panel is revealed.
     *
     * @param currentHash The current hash value
     * @param pos The position being revealed
     * @param oldValue The old value at this position (typically Unknown)
     * @param newValue The new value being revealed
     * @return The updated hash value
     */
    static uint64_t updateHash(uint64_t currentHash, Position pos,
                               PanelValue oldValue, PanelValue newValue);

    /**
     * Get the random value for a specific position and value.
     * Useful for debugging or manual hash computation.
     */
    static uint64_t getZobristValue(Position pos, PanelValue value);

    /**
     * Get the random value for a specific position index and value.
     */
    static uint64_t getZobristValue(size_t posIndex, PanelValue value);

private:
    // Convert PanelValue to index (0-4)
    static constexpr size_t valueToIndex(PanelValue v) {
        // Unknown=-1 -> 0, Voltorb=0 -> 1, One=1 -> 2, Two=2 -> 3, Three=3 -> 4
        return static_cast<size_t>(static_cast<int8_t>(v) + 2);
    }

    // Random values for each (position, value) combination
    // zobristTable_[position][value_index]
    // position: 0-24 (row*5 + col)
    // value_index: 0-4 (Unknown, Voltorb, One, Two, Three)
    static std::array<std::array<uint64_t, 5>, TOTAL_PANELS> zobristTable_;
    static bool initialized_;
};

} // namespace voltorb
