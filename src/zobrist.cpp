#include "voltorb/zobrist.hpp"

namespace voltorb {

// Static member definitions
std::array<std::array<uint64_t, 5>, TOTAL_PANELS> ZobristHasher::zobristTable_;
bool ZobristHasher::initialized_ = false;

void ZobristHasher::initialize(uint64_t seed) {
    // Use Mersenne Twister for high-quality random numbers
    std::mt19937_64 rng(seed);

    // Fill the Zobrist table with random values
    for (size_t pos = 0; pos < TOTAL_PANELS; pos++) {
        for (size_t val = 0; val < 5; val++) {
            zobristTable_[pos][val] = rng();
        }
    }

    initialized_ = true;
}

uint64_t ZobristHasher::hashPanels(const PanelGrid& panels) {
    if (!initialized_) {
        initialize();
    }

    uint64_t hash = 0;

    for (size_t row = 0; row < BOARD_SIZE; row++) {
        for (size_t col = 0; col < BOARD_SIZE; col++) {
            size_t posIndex = row * BOARD_SIZE + col;
            size_t valIndex = valueToIndex(panels[row][col]);
            hash ^= zobristTable_[posIndex][valIndex];
        }
    }

    return hash;
}

uint64_t ZobristHasher::updateHash(uint64_t currentHash, Position pos,
                                   PanelValue oldValue, PanelValue newValue) {
    if (!initialized_) {
        initialize();
    }

    size_t posIndex = pos.toIndex();
    size_t oldValIndex = valueToIndex(oldValue);
    size_t newValIndex = valueToIndex(newValue);

    // XOR out the old value, XOR in the new value
    // a ^ a = 0, so XORing twice removes the old contribution
    return currentHash ^ zobristTable_[posIndex][oldValIndex] ^ zobristTable_[posIndex][newValIndex];
}

uint64_t ZobristHasher::getZobristValue(Position pos, PanelValue value) {
    if (!initialized_) {
        initialize();
    }
    return zobristTable_[pos.toIndex()][valueToIndex(value)];
}

uint64_t ZobristHasher::getZobristValue(size_t posIndex, PanelValue value) {
    if (!initialized_) {
        initialize();
    }
    return zobristTable_[posIndex][valueToIndex(value)];
}

} // namespace voltorb
