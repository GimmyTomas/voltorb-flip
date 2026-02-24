//
//  legacy_solver.cpp
//  Extracted from main.cpp for library use
//
//  Created by Giovanni Maria Tomaselli on 16/04/22.
//  Refactored into library form.
//

#include "legacy_solver.hpp"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>

namespace legacy {

// ============ Global lookup tables ============

const Parameters board_type[8][10] = {
    {{6, 3, 1, 3, 3},
     {6, 0, 3, 2, 2},
     {6, 5, 0, 4, 3},
     {6, 2, 2, 3, 3},
     {6, 4, 1, 4, 3},
     {6, 3, 1, 3, 3},
     {6, 0, 3, 2, 2},
     {6, 5, 0, 4, 3},
     {6, 2, 2, 3, 3},
     {6, 4, 1, 4, 3}},
    {{7, 1, 3, 3, 2},
     {7, 6, 0, 4, 3},
     {7, 3, 2, 3, 2},
     {7, 0, 4, 3, 2},
     {7, 5, 1, 4, 3},
     {7, 1, 3, 2, 2},
     {7, 6, 0, 3, 3},
     {7, 3, 2, 2, 2},
     {7, 0, 4, 2, 2},
     {7, 5, 1, 3, 3}},
    {{8, 2, 3, 3, 2},
     {8, 7, 0, 4, 3},
     {8, 4, 2, 4, 3},
     {8, 1, 4, 3, 2},
     {8, 6, 1, 3, 4},
     {8, 2, 3, 2, 2},
     {8, 7, 0, 3, 3},
     {8, 4, 2, 3, 3},
     {8, 1, 4, 2, 2},
     {8, 6, 1, 3, 3}},
    {{8, 3, 3, 3, 4},
     {8, 0, 5, 3, 2},
     {10, 8, 0, 5, 4},
     {10, 5, 2, 4, 3},
     {10, 2, 4, 4, 3},
     {8, 3, 3, 3, 3},
     {8, 0, 5, 2, 2},
     {10, 8, 0, 4, 4},
     {10, 5, 2, 3, 3},
     {10, 2, 4, 3, 3}},
    {{10, 7, 1, 5, 4},
     {10, 4, 3, 4, 3},
     {10, 1, 5, 4, 3},
     {10, 9, 0, 5, 4},
     {10, 6, 2, 5, 4},
     {10, 7, 1, 4, 4},
     {10, 4, 3, 3, 3},
     {10, 1, 5, 3, 3},
     {10, 9, 0, 4, 4},
     {10, 6, 2, 4, 4}},
    {{10, 3, 4, 4, 3},
     {10, 0, 6, 4, 3},
     {10, 8, 1, 5, 4},
     {10, 5, 3, 5, 4},
     {10, 2, 5, 4, 3},
     {10, 3, 4, 3, 3},
     {10, 0, 6, 3, 3},
     {10, 8, 1, 4, 4},
     {10, 5, 3, 4, 4},
     {10, 2, 5, 3, 3}},
    {{10, 7, 2, 5, 4},
     {10, 4, 4, 5, 4},
     {13, 1, 6, 4, 3},
     {13, 9, 1, 6, 5},
     {10, 6, 3, 5, 4},
     {10, 7, 2, 4, 4},
     {10, 4, 4, 4, 4},
     {13, 1, 6, 3, 3},
     {13, 9, 1, 5, 5},
     {10, 6, 3, 4, 4}},
    {{10, 0, 7, 4, 3},
     {10, 8, 2, 6, 5},
     {10, 5, 4, 5, 4},
     {10, 2, 6, 5, 4},
     {10, 7, 3, 6, 5},
     {10, 0, 7, 3, 3},
     {10, 8, 2, 5, 5},
     {10, 5, 4, 4, 4},
     {10, 2, 6, 4, 4},
     {10, 7, 3, 5, 5}}};

const long int N_permutations[8][10] = {
    // = 25!/(n0!n1!n2!n3!)
    {2745758400, 171609900, 2059318800, 4118637600, 10296594000, 2745758400, 171609900,
     2059318800, 4118637600, 10296594000},
    {5883768000, 8923714800, 41186376000, 1470942000, 53542288800, 5883768000, 8923714800,
     41186376000, 1470942000, 53542288800},
    {66927861000, 21034470600, 200783583000, 33463930500, 147241294200, 66927861000, 21034470600,
     200783583000, 33463930500, 147241294200},
    {267711444000, 6692786100, 21034470600, 441723882600, 245402157000, 267711444000, 6692786100,
     21034470600, 441723882600, 245402157000},
    {168275764800, 736206471000, 98160862800, 16360143800, 588965176800, 168275764800,
     736206471000, 98160862800, 16360143800, 588965176800},
    {736206471000, 16360143800, 147241294200, 1177930353600, 441723882600, 736206471000,
     16360143800, 147241294200, 1177930353600, 441723882600},
    {588965176800, 1472412942000, 28830463200, 3432198000, 1374252079200, 588965176800,
     1472412942000, 28830463200, 3432198000, 1374252079200},
    {21034470600, 441723882600, 2061378118800, 588965176800, 1177930353600, 21034470600,
     441723882600, 2061378118800, 588965176800, 1177930353600}};

const long int N_permutations_gcd[8] = {171609900,  98062800,    956112300,  318704100,
                                        2337163400, 16360143800, 686439600,  21034470600};

const int N_permutations_reduced[8][10] = {
    // = 25!/(n0!n1!n2!n3!), divided by the greatest common divisor per each Level
    {16, 1, 12, 24, 60, 16, 1, 12, 24, 60},           // gcd =    171,609,900
    {60, 91, 420, 15, 546, 60, 91, 420, 15, 546},     // gcd =     98,062,800
    {70, 22, 210, 35, 154, 70, 22, 210, 35, 154},     // gcd =    956,112,300
    {840, 21, 66, 1386, 770, 840, 21, 66, 1386, 770}, // gcd =    318,704,100
    {72, 315, 42, 7, 252, 72, 315, 42, 7, 252},       // gcd =  2,337,163,400
    {45, 1, 9, 72, 27, 45, 1, 9, 72, 27},             // gcd = 16,360,143,800
    {858, 2145, 42, 5, 2002, 858, 2145, 42, 5, 2002}, // gcd =    686,439,600
    {1, 21, 98, 28, 56, 1, 21, 98, 28, 56}            // gcd = 21,034,470,600
};

const long int N_accepted[8][10] = {
    // = valid boards
    {1732660000, 81056200, 1407934200, 2598990000, 7039671000, 1732660000, 81056200, 1407934200,
     2598990000, 7039671000},
    {3285100800, 5722710400, 17316544000, 821275200, 34336262400, 2684683200, 4495352000,
     14348488000, 671170800, 26972112000},
    {34984058000, 12979316000, 145577634000, 17492029000, 81740052800, 32177972000, 11677150400,
     128566014000, 16088986000, 81740052800},
    {171421352000, 3498405800, 18355191900, 335965442400, 204113718000, 171421352000, 3217797200,
     17976411900, 331634822400, 199722318000},
    {146841535200, 559942404000, 81645487200, 13300405700, 513945373200, 143811295200,
     552724704000, 79888927200, 13159573700, 503339533200},
    {559942404000, 13607581200, 119703651300, 1027890746400, 335965442400, 552724704000,
     13314821200, 118436163300, 1006679066400, 331634822400},
    {478814605200, 1284863433000, 25901458800, 3265542000, 1117234078800, 473744653200,
     1258348833000, 25901458800, 3265542000, 1105404190800},
    {15998354400, 400990788900, 1675851118200, 513945373200, 1069308770400, 15792134400,
     394129278900, 1658106286200, 503339533200, 1051011410400}};

const int N_accepted_gcd[8] = {200, 400, 200, 100, 100, 100, 600, 300};

const long int N_accepted_reduced[8][10] = {
    // = valid boards, divided by the greatest common divisor per each Level
    {8663300, 405281, 7039671, 12994950, 35198355, 8663300, 405281, 7039671, 12994950,
     35198355}, // gcd = 200
    {8212752, 14306776, 43291360, 2053188, 85840656, 6711708, 11238380, 35871220, 1677927,
     67430280}, // gcd = 400
    {174920290, 64896580, 727888170, 87460145, 408700264, 160889860, 58385752, 642830070, 80444930,
     408700264}, // gcd = 200
    {1714213520, 34984058, 183551919, 3359654424, 2041137180, 1714213520, 32177972, 179764119,
     3316348224, 1997223180}, // gcd = 100
    {1468415352, 5599424040, 816454872, 133004057, 5139453732, 1438112952, 5527247040, 798889272,
     131595737, 5033395332}, // gcd = 100
    {5599424040, 136075812, 1197036513, 10278907464, 3359654424, 5527247040, 133148212, 1184361633,
     10066790664, 3316348224}, // gcd = 100
    {798024342, 2141439055, 43169098, 5442570, 1862056798, 789574422, 2097248055, 43169098,
     5442570, 1842340318}, // gcd = 600
    {53327848, 1336635963, 5586170394, 1713151244, 3564362568, 52640448, 1313764263, 5527020954,
     1677798444, 3503371368} // gcd = 300
};

// ============ Board operations ============

bool check_legality_board(const Parameters& params, const Board& board) {
    int total_free_multipliers = 0;
    int row_free_multipliers = 0;

    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++)
            if ((board.row_0[i] == 0 || board.column_0[j] == 0) && board.panel[i][j] > 1)
                total_free_multipliers++;
    if (total_free_multipliers >= params.max_total_free)
        return false;

    for (int i = 0; i < 5; i++)
        if (board.row_0[i] == 0) {
            row_free_multipliers = 0;
            for (int j = 0; j < 5; j++)
                if (board.panel[i][j] > 1)
                    row_free_multipliers++;
            if (row_free_multipliers >= params.max_row_free)
                return false;
        }

    for (int j = 0; j < 5; j++)
        if (board.column_0[j] == 0) {
            row_free_multipliers = 0;
            for (int i = 0; i < 5; i++)
                if (board.panel[i][j] > 1)
                    row_free_multipliers++;
            if (row_free_multipliers >= params.max_row_free)
                return false;
        }

    return true;
}

bool no_conflict(const Board& real_board, const Board& test_board) {
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++)
            if ((real_board.panel[i][j] != -1) && (test_board.panel[i][j] != -1) &&
                (real_board.panel[i][j] != test_board.panel[i][j]))
                return false;
    return true;
}

bool are_equal(const Board& a, const Board& b) {
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++)
            if (a.panel[i][j] != b.panel[i][j])
                return false;
    return true;
}

bool fully_flipped(const Board& board) {
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++)
            if (board.panel[i][j] == -1)
                return false;
    return true;
}

bool numbers_dont_exceed(const Board& board) {
    int sum, sum_abs;
    bool totally_flipped_row;

    for (int j = 0; j < 5; j++) {
        totally_flipped_row = true;
        sum = 0;
        sum_abs = 0;

        for (int i = 0; i < 5; i++) {
            sum += board.panel[i][j];
            sum_abs += abs(board.panel[i][j]);
            if (board.panel[i][j] == -1)
                totally_flipped_row = false;
        }
        if (sum_abs > board.column_sum[j])
            return false;
        if ((sum != board.column_sum[j]) && totally_flipped_row)
            return false;
    }

    for (int i = 0; i < 5; i++) {
        totally_flipped_row = true;
        sum = 0;
        sum_abs = 0;

        for (int j = 0; j < 5; j++) {
            sum += board.panel[i][j];
            sum_abs += abs(board.panel[i][j]);
            if (board.panel[i][j] == -1)
                totally_flipped_row = false;
        }
        if (sum_abs > board.row_sum[i])
            return false;
        if ((sum != board.row_sum[i]) && totally_flipped_row)
            return false;
    }

    return true;
}

bool compatible_type(const Board& board, uint8_t type) {
    int min_number_of[4] = {0, 0, 0, 0};

    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++)
            if (board.panel[i][j] > 0) // could also be > -1
                min_number_of[board.panel[i][j]] += 1;
    if (!(min_number_of[1] <= 25 - board_type[board.level - 1][type].n_0 -
                                 board_type[board.level - 1][type].n_2 -
                                 board_type[board.level - 1][type].n_3))
        return false;
    if (!(min_number_of[2] <= board_type[board.level - 1][type].n_2))
        return false;
    if (!(min_number_of[3] <= board_type[board.level - 1][type].n_3))
        return false;

    if (!(check_legality_board(board_type[board.level - 1][type], board)))
        return false;

    int total_bombs =
        board.row_0[0] + board.row_0[1] + board.row_0[2] + board.row_0[3] + board.row_0[4];
    if (!(total_bombs == board_type[board.level - 1][type].n_0))
        return false;

    int total_sums =
        board.row_sum[0] + board.row_sum[1] + board.row_sum[2] + board.row_sum[3] + board.row_sum[4];
    if (!(total_sums == (25 - board_type[board.level - 1][type].n_0 +
                         1 * board_type[board.level - 1][type].n_2 +
                         2 * board_type[board.level - 1][type].n_3)))
        return false;

    return true;
}

int is_won(const Board& board) {
    int total_flipped = 0;

    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++) {
            if (board.panel[i][j] == 0)
                return 0;
            else
                total_flipped += abs(board.panel[i][j]);
        }

    int total = board.row_sum[0] + board.row_sum[1] + board.row_sum[2] + board.row_sum[3] +
                board.row_sum[4] + board.row_0[0] + board.row_0[1] + board.row_0[2] +
                board.row_0[3] + board.row_0[4];

    if (total == total_flipped)
        return 1;
    else
        return -1;
}

// ============ Board generation - helper functions ============

static bool valid_0_positions(const Board& real_board, std::string positions_0[5],
                              int rows_to_consider) {
    int sum[5] = {0, 0, 0, 0, 0};
    for (int j = 0; j < 5; j++)
        for (int i = 0; i < rows_to_consider; i++)
            sum[j] += positions_0[i][j];

    if (sum[0] <= real_board.column_0[0] && sum[1] <= real_board.column_0[1] &&
        sum[2] <= real_board.column_0[2] && sum[3] <= real_board.column_0[3] &&
        sum[4] <= real_board.column_0[4])
        return true;
    else
        return false;
}

void generate_0_positions(const Board& real_board, std::vector<Board>& possible_0_positions) {
    int i, j;
    Board test;

    std::string positions_0[5];
    for (i = 0; i < 5; i++) {
        positions_0[i].resize(5, 0);
        std::fill(positions_0[i].begin(), positions_0[i].begin() + real_board.row_0[i], 1);
    }

    possible_0_positions.clear();

    do {
        if (valid_0_positions(real_board, positions_0, 1)) {
            do {
                if (valid_0_positions(real_board, positions_0, 2)) {
                    do {
                        if (valid_0_positions(real_board, positions_0, 3)) {
                            do {
                                if (valid_0_positions(real_board, positions_0, 4)) {
                                    do {
                                        if (valid_0_positions(real_board, positions_0, 5)) {
                                            test = real_board;
                                            for (i = 0; i < 5; i++)
                                                for (j = 0; j < 5; j++)
                                                    test.panel[i][j] = positions_0[i][j] - 1;
                                            if (no_conflict(real_board, test))
                                                possible_0_positions.push_back(test);
                                        }
                                    } while (std::prev_permutation(positions_0[4].begin(),
                                                                   positions_0[4].end()));
                                }
                            } while (
                                std::prev_permutation(positions_0[3].begin(), positions_0[3].end()));
                        }
                    } while (std::prev_permutation(positions_0[2].begin(), positions_0[2].end()));
                }
            } while (std::prev_permutation(positions_0[1].begin(), positions_0[1].end()));
        }
    } while (std::prev_permutation(positions_0[0].begin(), positions_0[0].end()));
}

// ============ Compatible board generation ============

// Recursive backtracking to fill non-voltorb cells exhaustively
static void fill_non_voltorbs_recursive(Board& current, int pos, int remaining2s, int remaining3s,
                                        std::vector<Board>& result, uint8_t type) {
    // Find next unknown cell (where panel[i][j] == -1)
    while (pos < 25) {
        int i = pos / 5;
        int j = pos % 5;
        if (current.panel[i][j] == -1) {
            break;
        }
        pos++;
    }

    // If no more unknown cells, validate and add to result
    if (pos >= 25) {
        if (numbers_dont_exceed(current) && compatible_type(current, type)) {
            result.push_back(current);
        }
        return;
    }

    int i = pos / 5;
    int j = pos % 5;

    // Try placing 1
    current.panel[i][j] = 1;
    if (numbers_dont_exceed(current)) {
        fill_non_voltorbs_recursive(current, pos + 1, remaining2s, remaining3s, result, type);
    }

    // Try placing 2 (if we have remaining 2s to place)
    if (remaining2s > 0) {
        current.panel[i][j] = 2;
        if (numbers_dont_exceed(current)) {
            fill_non_voltorbs_recursive(current, pos + 1, remaining2s - 1, remaining3s, result, type);
        }
    }

    // Try placing 3 (if we have remaining 3s to place)
    if (remaining3s > 0) {
        current.panel[i][j] = 3;
        if (numbers_dont_exceed(current)) {
            fill_non_voltorbs_recursive(current, pos + 1, remaining2s, remaining3s - 1, result, type);
        }
    }

    // Backtrack: reset cell to unknown
    current.panel[i][j] = -1;
}

void generate_compatible_boards(const Board& real_board,
                                const std::vector<Board>& possible_0_positions,
                                std::vector<Board> compatible_boards[10]) {
    for (int type = 0; type < 10; type++) {
        compatible_boards[type].clear();
        if (!compatible_type(real_board, static_cast<uint8_t>(type))) {
            continue;
        }

        const Parameters& params = board_type[real_board.level - 1][type];

        for (const Board& voltorb_board : possible_0_positions) {
            // Create a board with voltorbs placed and revealed panels merged
            Board board = voltorb_board;
            bool conflict = false;

            // Merge revealed panels from real_board
            for (int i = 0; i < 5; i++) {
                for (int j = 0; j < 5; j++) {
                    if (real_board.panel[i][j] != -1) {
                        if (board.panel[i][j] == -1) {
                            board.panel[i][j] = real_board.panel[i][j];
                        } else if (board.panel[i][j] != real_board.panel[i][j]) {
                            // Conflict between voltorb position and revealed panel
                            conflict = true;
                            break;
                        }
                    }
                }
                if (conflict) break;
            }

            if (conflict) {
                continue;
            }

            // Count how many 2s and 3s are already placed (from revealed panels)
            int placed_2s = 0;
            int placed_3s = 0;
            for (int i = 0; i < 5; i++) {
                for (int j = 0; j < 5; j++) {
                    if (board.panel[i][j] == 2) placed_2s++;
                    else if (board.panel[i][j] == 3) placed_3s++;
                }
            }

            int remaining2s = params.n_2 - placed_2s;
            int remaining3s = params.n_3 - placed_3s;

            // Skip if we've already exceeded the count
            if (remaining2s < 0 || remaining3s < 0) {
                continue;
            }

            // Use recursive backtracking to fill all non-voltorb cells
            fill_non_voltorbs_recursive(board, 0, remaining2s, remaining3s,
                                        compatible_boards[type], static_cast<uint8_t>(type));
        }
    }
}

// ============ Probability calculation ============

std::vector<std::vector<PanelProbabilities>>
calculatePanelProbabilities(const Board& board, const std::vector<Board> compatible[10],
                            const std::vector<ptrdiff_t> compatible_indices[10]) {
    std::vector<std::vector<PanelProbabilities>> result(
        5, std::vector<PanelProbabilities>(5, PanelProbabilities{{0.0, 0.0, 0.0, 0.0}}));

    Board next_board;
    std::vector<ptrdiff_t> next_compatible_indices[10];

    double P_of_b = 0;
    for (int type = 0; type < 10; type++)
        P_of_b += static_cast<double>(compatible_indices[type].size()) /
                  static_cast<double>(N_accepted[board.level - 1][type]);

    if (P_of_b == 0)
        return result;

    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            if (board.panel[i][j] == -1) {
                for (int value = 0; value < 4; value++) {
                    double P_of_b_p_v = 0;
                    for (int type = 0; type < 10; type++)
                        next_compatible_indices[type].clear();

                    next_board = board;
                    next_board.panel[i][j] = static_cast<int8_t>(value);

                    for (int type = 0; type < 10; type++) {
                        for (auto idx : compatible_indices[type])
                            if (no_conflict(compatible[type][static_cast<size_t>(idx)], next_board))
                                next_compatible_indices[type].push_back(idx);
                        P_of_b_p_v +=
                            (static_cast<double>(next_compatible_indices[type].size()) /
                             static_cast<double>(N_accepted[board.level - 1][type])) /
                            P_of_b;
                    }
                    result[static_cast<size_t>(i)][static_cast<size_t>(j)].p[value] = P_of_b_p_v;
                }
            }
        }
    }

    return result;
}

void print_panel_probabilities(const Board& board, const std::vector<Board> compatible[10],
                               const std::vector<ptrdiff_t> compatible_indices[10]) {
    auto probs = calculatePanelProbabilities(board, compatible, compatible_indices);

    std::cout << std::fixed;
    std::cout << std::setprecision(4);

    std::cout << "            \tP(0) \tP(1) \tP(2) \tP(3) \t\tP(mult) " << std::endl;

    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++)
            if (board.panel[i][j] == -1) {
                std::cout << "i = " << i << ", j = " << j;
                for (int v = 0; v < 4; v++)
                    std::cout << "\t" << probs[static_cast<size_t>(i)][static_cast<size_t>(j)].p[v];
                std::cout << "\t\t"
                          << probs[static_cast<size_t>(i)][static_cast<size_t>(j)].p[2] +
                                 probs[static_cast<size_t>(i)][static_cast<size_t>(j)].p[3];
                std::cout << std::endl;
            }
}

// ============ Recursive solver ============

static double best_panel_recursive_impl(const Board& board, int& suggested_row,
                                        int& suggested_column, const std::vector<Board> compatible[10],
                                        std::vector<ptrdiff_t> compatible_indices[10],
                                        std::vector<HashItem> hash[25], int n_uncovered) {
    if (is_won(board) == 1)
        return 1.0;

    for (auto& item : hash[n_uncovered])
        if (are_equal(item.board, board)) {
            suggested_row = item.suggested_row;
            suggested_column = item.suggested_column;
            return item.win_chance;
        }

    double P_of_b = 0;
    for (int type = 0; type < 10; type++)
        P_of_b += static_cast<double>(compatible_indices[type].size()) /
                  static_cast<double>(N_accepted[board.level - 1][type]);

    double win_chance = 0, win_chance_from_panel, tmp;

    Board next_board;
    std::vector<ptrdiff_t> next_compatible_indices[10];
    HashItem next_result;

    bool found_free_panel;

    // Check for guaranteed safe panels first
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++)
            if (board.panel[i][j] == -1) {
                found_free_panel = true;

                for (int type = 0; type < 10; type++) {
                    if (!compatible_indices[type].empty()) {
                        bool found_voltorb = false;
                        for (auto idx : compatible_indices[type])
                            if (compatible[type][static_cast<size_t>(idx)].panel[i][j] == 0) {
                                found_voltorb = true;
                                break;
                            }
                        if (found_voltorb) {
                            found_free_panel = false;
                            break;
                        }
                    }
                }

                if (found_free_panel) {
                    suggested_row = i;
                    suggested_column = j;

                    win_chance = 0;

                    for (int value = 1; value < 4; value++) {
                        tmp = 0;

                        for (int type = 0; type < 10; type++)
                            next_compatible_indices[type].clear();

                        next_board = board;
                        next_board.panel[i][j] = static_cast<int8_t>(value);

                        for (int type = 0; type < 10; type++) {
                            for (auto idx : compatible_indices[type])
                                if (no_conflict(compatible[type][static_cast<size_t>(idx)], next_board))
                                    next_compatible_indices[type].push_back(idx);
                            tmp += static_cast<double>(next_compatible_indices[type].size()) /
                                   static_cast<double>(N_accepted[board.level - 1][type]);
                        }

                        if (tmp > 0) {
                            next_result.board = next_board;
                            next_result.win_chance = best_panel_recursive_impl(
                                next_board, next_result.suggested_row, next_result.suggested_column,
                                compatible, next_compatible_indices, hash, n_uncovered + 1);
                            tmp *= next_result.win_chance / P_of_b;
                            hash[n_uncovered + 1].push_back(next_result);
                        }

                        win_chance += tmp;
                    }

                    return win_chance;
                }
            }

    // No guaranteed safe panel, evaluate all
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++)
            if (board.panel[i][j] == -1) {
                win_chance_from_panel = 0;

                for (int value = 1; value < 4; value++) {
                    for (int type = 0; type < 10; type++)
                        next_compatible_indices[type].clear();

                    next_board = board;
                    next_board.panel[i][j] = static_cast<int8_t>(value);

                    tmp = 0;

                    for (int type = 0; type < 10; type++) {
                        for (auto idx : compatible_indices[type])
                            if (no_conflict(compatible[type][static_cast<size_t>(idx)], next_board))
                                next_compatible_indices[type].push_back(idx);
                        tmp += static_cast<double>(next_compatible_indices[type].size()) /
                               static_cast<double>(N_accepted[board.level - 1][type]);
                    }

                    if (tmp > 0) {
                        next_result.board = next_board;
                        next_result.win_chance = best_panel_recursive_impl(
                            next_board, next_result.suggested_row, next_result.suggested_column,
                            compatible, next_compatible_indices, hash, n_uncovered + 1);
                        tmp *= next_result.win_chance / P_of_b;
                        hash[n_uncovered + 1].push_back(next_result);
                    }

                    win_chance_from_panel += tmp;
                }

                if (win_chance_from_panel > win_chance) {
                    suggested_row = i;
                    suggested_column = j;
                    win_chance = win_chance_from_panel;
                }
            }

    return win_chance;
}

BestPanelResult findBestPanel(const Board& board, const std::vector<Board> compatible[10],
                              std::vector<ptrdiff_t> compatible_indices[10],
                              std::vector<HashItem> hash[25], int n_uncovered) {
    BestPanelResult result;
    result.row = 0;
    result.col = 0;
    result.winProbability =
        best_panel_recursive_impl(board, result.row, result.col, compatible, compatible_indices,
                                  hash, n_uncovered);
    return result;
}

// ============ Utility functions ============

void cover_all_panels(const Board& real_board, Board& covered_board) {
    covered_board = real_board;

    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++)
            covered_board.panel[i][j] = -1;
}

void print_board(const Board& board, int row_suggested, int column_suggested) {
    std::cout << std::endl;
    std::cout << "\t       Level " << unsigned(board.level) << std::endl;
    for (int i = 0; i < 5; i++) {
        std::cout << "\t---------------------" << std::endl;
        std::cout << "\t";
        for (int j = 0; j < 5; j++) {
            if (i == row_suggested && j == column_suggested)
                std::cout << "|###";
            else {
                if (board.panel[i][j] == -1)
                    std::cout << "|   ";
                else
                    std::cout << "| " << unsigned(board.panel[i][j]) << " ";
            }
        }
        std::cout << "| " << unsigned(board.row_sum[i]) << "," << unsigned(board.row_0[i])
                  << std::endl;
    }
    std::cout << "\t---------------------" << std::endl;
    std::cout << "\t";
    for (int j = 0; j < 5; j++) {
        if (board.column_sum[j] < 10)
            std::cout << "  " << unsigned(board.column_sum[j]) << " ";
        else
            std::cout << " " << unsigned(board.column_sum[j]) << " ";
    }
    std::cout << std::endl;
    std::cout << "\t";
    for (int j = 0; j < 5; j++)
        std::cout << "  " << unsigned(board.column_0[j]) << " ";
    std::cout << std::endl;
    std::cout << std::endl;
}

} // namespace legacy
