#pragma once

#include "types.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace voltorb {

/**
 * Entry flags for transposition table entries.
 */
enum class TTEntryFlag : uint8_t {
    Empty = 0,   // Slot is unused
    Exact = 1,   // Value is exact (fully searched at this depth)
    LowerBound = 2,  // Value is a lower bound (beta cutoff)
    UpperBound = 3   // Value is an upper bound (alpha cutoff)
};

/**
 * A single entry in the transposition table.
 */
struct TranspositionEntry {
    uint64_t hash = 0;           // Full hash for verification (handles collisions)
    double winProbability = 0.0; // Computed win probability
    Position bestPanel{0, 0};    // Best panel found at this position
    uint8_t depth = 0;           // Search depth at which this was computed
    TTEntryFlag flag = TTEntryFlag::Empty;

    bool isEmpty() const { return flag == TTEntryFlag::Empty; }
};

/**
 * High-performance transposition table for memoizing search results.
 *
 * Features:
 * - Fixed-size power-of-2 table for fast modulo (bitwise AND)
 * - Full hash verification to handle collisions
 * - Depth tracking to ensure results are valid for current search depth
 * - Support for exact values and bounds
 *
 * Replacement policy: Always replace (simpler and often effective)
 */
class TranspositionTable {
public:
    /**
     * Create a transposition table with the given size.
     * Size will be rounded up to the nearest power of 2.
     *
     * @param size Number of entries (default 1M entries, ~32MB)
     */
    explicit TranspositionTable(size_t size = 1 << 20);

    /**
     * Probe the table for an entry matching the hash.
     *
     * @param hash The Zobrist hash of the board state
     * @return The entry if found, or nullptr if not present
     */
    const TranspositionEntry* probe(uint64_t hash) const;

    /**
     * Store a result in the table.
     *
     * @param hash The Zobrist hash of the board state
     * @param winProb The computed win probability
     * @param bestPanel The best panel found
     * @param depth The search depth at which this was computed
     * @param flag Whether this is an exact value or a bound
     */
    void store(uint64_t hash, double winProb, Position bestPanel,
               uint8_t depth, TTEntryFlag flag = TTEntryFlag::Exact);

    /**
     * Clear all entries from the table.
     */
    void clear();

    /**
     * Get statistics about table usage.
     */
    size_t getHits() const { return hits_; }
    size_t getMisses() const { return misses_; }
    size_t getStores() const { return stores_; }
    size_t getCollisions() const { return collisions_; }
    size_t getSize() const { return table_.size(); }

    /**
     * Get the fill rate (fraction of non-empty slots).
     */
    double getFillRate() const;

    /**
     * Reset statistics counters.
     */
    void resetStats();

private:
    std::vector<TranspositionEntry> table_;
    size_t mask_;  // For fast modulo: index = hash & mask_

    // Statistics
    mutable size_t hits_ = 0;
    mutable size_t misses_ = 0;
    size_t stores_ = 0;
    size_t collisions_ = 0;

    // Round up to nearest power of 2
    static size_t nextPowerOf2(size_t n);
};

} // namespace voltorb
