#include "voltorb/transposition.hpp"

#include <algorithm>

namespace voltorb {

TranspositionTable::TranspositionTable(size_t size) {
    size_t actualSize = nextPowerOf2(size);
    table_.resize(actualSize);
    mask_ = actualSize - 1;
}

const TranspositionEntry* TranspositionTable::probe(uint64_t hash) const {
    size_t index = hash & mask_;
    const TranspositionEntry& entry = table_[index];

    if (entry.isEmpty()) {
        misses_++;
        return nullptr;
    }

    // Verify full hash to handle collisions
    if (entry.hash != hash) {
        misses_++;
        return nullptr;
    }

    hits_++;
    return &entry;
}

void TranspositionTable::store(uint64_t hash, double winProb, Position bestPanel,
                               uint8_t depth, TTEntryFlag flag) {
    size_t index = hash & mask_;
    TranspositionEntry& entry = table_[index];

    // Depth-preferring replacement: don't evict deeper entries with shallower ones
    if (!entry.isEmpty() && entry.hash != hash) {
        if (depth < entry.depth) {
            return;  // Don't evict a deeper entry with a shallower one
        }
        collisions_++;
    }

    entry.hash = hash;
    entry.winProbability = winProb;
    entry.bestPanel = bestPanel;
    entry.depth = depth;
    entry.flag = flag;

    stores_++;
}

void TranspositionTable::clear() {
    for (auto& entry : table_) {
        entry = TranspositionEntry{};
    }
    resetStats();
}

double TranspositionTable::getFillRate() const {
    if (table_.empty()) return 0.0;

    size_t nonEmpty = 0;
    for (const auto& entry : table_) {
        if (!entry.isEmpty()) {
            nonEmpty++;
        }
    }
    return static_cast<double>(nonEmpty) / static_cast<double>(table_.size());
}

void TranspositionTable::resetStats() {
    hits_ = 0;
    misses_ = 0;
    stores_ = 0;
    collisions_ = 0;
}

size_t TranspositionTable::nextPowerOf2(size_t n) {
    if (n == 0) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return n + 1;
}

} // namespace voltorb
