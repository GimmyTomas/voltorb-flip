#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace legacy {

// Board type parameters (n_0=voltorbs, n_2=twos, n_3=threes, max_total_free, max_row_free)
struct Parameters {
    int8_t n_0;
    int8_t n_2;
    int8_t n_3;
    int8_t max_total_free;
    int8_t max_row_free;
};

// Board state representation
struct Board {
    int8_t level;
    int8_t panel[5][5]; // -1 = unknown, 0 = voltorb, 1/2/3 = value
    int8_t row_sum[5];
    int8_t row_0[5];
    int8_t column_sum[5];
    int8_t column_0[5];
};

// Memoization entry for recursive solver
struct HashItem {
    Board board;
    int suggested_row;
    int suggested_column;
    double win_chance;
};

// Global lookup tables
extern const Parameters board_type[8][10];
extern const long int N_permutations[8][10];
extern const int N_permutations_reduced[8][10];
extern const long int N_permutations_gcd[8];
extern const long int N_accepted[8][10];
extern const long int N_accepted_reduced[8][10];
extern const int N_accepted_gcd[8];

// ============ Board operations ============

// Check if a board configuration is legal for given parameters
bool check_legality_board(const Parameters& params, const Board& board);

// Check if test_board has no conflicting revealed panels with real_board
bool no_conflict(const Board& real_board, const Board& test_board);

// Check if boards are equal (same panel values)
bool are_equal(const Board& a, const Board& b);

// Check if all panels are revealed (none are -1)
bool fully_flipped(const Board& board);

// Check if partial board doesn't exceed row/column sum constraints
bool numbers_dont_exceed(const Board& board);

// Check if board is compatible with a given type
bool compatible_type(const Board& board, uint8_t type);

// Game state: -1 = in progress, 0 = lost (voltorb flipped), 1 = won
int is_won(const Board& board);

// ============ Board generation ============

// Generate all valid voltorb position configurations
void generate_0_positions(const Board& board, std::vector<Board>& positions);

// Generate all complete boards compatible with the given partial board
void generate_compatible_boards(const Board& board,
                                const std::vector<Board>& positions,
                                std::vector<Board> compatible[10]);

// ============ Probability calculation ============

// Panel probability distribution: P(0), P(1), P(2), P(3)
struct PanelProbabilities {
    double p[4]; // p[0]=P(voltorb), p[1]=P(one), p[2]=P(two), p[3]=P(three)
};

// Calculate probabilities for each unknown panel
// Returns a 5x5 grid of probabilities (only meaningful for panels where board.panel[i][j] == -1)
std::vector<std::vector<PanelProbabilities>>
calculatePanelProbabilities(const Board& board,
                            const std::vector<Board> compatible[10],
                            const std::vector<ptrdiff_t> compatible_indices[10]);

// ============ Solver ============

// Result of finding the best panel to flip
struct BestPanelResult {
    int row;
    int col;
    double winProbability;
};

// Find the best panel to flip using recursive minimax with memoization
// This is the main solver entry point
BestPanelResult findBestPanel(const Board& board,
                              const std::vector<Board> compatible[10],
                              std::vector<ptrdiff_t> compatible_indices[10],
                              std::vector<HashItem> hash[25],
                              int n_uncovered);

// ============ Utility functions ============

// Cover all panels (set to -1)
void cover_all_panels(const Board& real_board, Board& covered_board);

// Print board to stdout
void print_board(const Board& board, int row_suggested = 5, int column_suggested = 5);

// Print panel probabilities to stdout
void print_panel_probabilities(const Board& board,
                               const std::vector<Board> compatible[10],
                               const std::vector<ptrdiff_t> compatible_indices[10]);

} // namespace legacy
