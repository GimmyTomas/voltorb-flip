//
//  main.cpp
//  voltorb-flip
//
//  Created by Giovanni Maria Tomaselli on 16/04/22.
//

#include "legacy_solver.hpp"

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <random>

using namespace legacy;

static auto getRandomSeed() -> std::seed_seq {
    std::random_device source;

    unsigned int random_data[10];
    for (auto& elem : random_data)
        elem = source();

    return std::seed_seq(random_data + 0, random_data + 10);
}

static int rand_uniform(int a, int b) {
    static auto seed = getRandomSeed();
    static std::mt19937 rng(seed);

    std::uniform_int_distribution<int> dist(a, b);
    return dist(rng);
}

static int factorial(int n) {
    if (n < 0)
        return 0;
    return !n ? 1 : n * factorial(n - 1);
}

static int random_initialize_board(int8_t level, Board& board, int type = rand_uniform(0, 9),
                                   int number_of_tries = 1) {
    std::cout << "Type = " << type << std::endl;
    int i, j;
    Parameters params = board_type[level - 1][type];

    board.level = level;

    for (i = 0; i < 5; i++)
        for (j = 0; j < 5; j++)
            board.panel[i][j] = 1;

    int n_0_remaining = params.n_0;
    while (n_0_remaining > 0) {
        i = rand_uniform(0, 4);
        j = rand_uniform(0, 4);
        if (board.panel[i][j] == 1) {
            board.panel[i][j] = 0;
            n_0_remaining--;
        }
    }

    int n_2_remaining = params.n_2;
    while (n_2_remaining > 0) {
        i = rand_uniform(0, 4);
        j = rand_uniform(0, 4);
        if (board.panel[i][j] == 1) {
            board.panel[i][j] = 2;
            n_2_remaining--;
        }
    }

    int n_3_remaining = params.n_3;
    while (n_3_remaining > 0) {
        i = rand_uniform(0, 4);
        j = rand_uniform(0, 4);
        if (board.panel[i][j] == 1) {
            board.panel[i][j] = 3;
            n_3_remaining--;
        }
    }

    for (i = 0; i < 5; i++) {
        board.row_sum[i] = static_cast<int8_t>(board.panel[i][0] + board.panel[i][1] +
                                               board.panel[i][2] + board.panel[i][3] +
                                               board.panel[i][4]);
        board.row_0[i] =
            static_cast<int8_t>((board.panel[i][0] == 0) + (board.panel[i][1] == 0) +
                                (board.panel[i][2] == 0) + (board.panel[i][3] == 0) +
                                (board.panel[i][4] == 0));
    }

    for (j = 0; j < 5; j++) {
        board.column_sum[j] = static_cast<int8_t>(board.panel[0][j] + board.panel[1][j] +
                                                  board.panel[2][j] + board.panel[3][j] +
                                                  board.panel[4][j]);
        board.column_0[j] =
            static_cast<int8_t>((board.panel[0][j] == 0) + (board.panel[1][j] == 0) +
                                (board.panel[2][j] == 0) + (board.panel[3][j] == 0) +
                                (board.panel[4][j] == 0));
    }

    if (check_legality_board(params, board))
        return number_of_tries;
    else
        return random_initialize_board(level, board, type, number_of_tries + 1);
}

int main(int argc, const char* argv[]) {
    (void)argc;
    (void)argv;

    Board real_board, covered_board;

    random_initialize_board(8, real_board);

    std::vector<Board> possible_0_positions;
    std::vector<Board> compatible_boards[10];
    std::vector<ptrdiff_t> compatible_boards_without_conflict[10];
    std::vector<HashItem> hash[25];
    int chosen_row, chosen_column;

    print_board(real_board);

    cover_all_panels(real_board, covered_board);

    int n_uncovered = 0;
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++)
            if (covered_board.panel[i][j] > -1)
                n_uncovered++;

    while (is_won(covered_board) == -1) {
        print_board(covered_board);

        generate_0_positions(covered_board, possible_0_positions);

        generate_compatible_boards(covered_board, possible_0_positions, compatible_boards);
        for (int type = 0; type < 10; type++) {
            std::cout << "Type " << type << " " << compatible_boards[type].size() << std::endl;
            for (size_t n = 0; n < compatible_boards[type].size(); n++)
                compatible_boards_without_conflict[type].push_back(static_cast<ptrdiff_t>(n));
        }

        print_panel_probabilities(covered_board, compatible_boards,
                                  compatible_boards_without_conflict);

        auto result = findBestPanel(covered_board, compatible_boards,
                                    compatible_boards_without_conflict, hash, n_uncovered);

        std::cout << "i = " << result.row << ", j = " << result.col
                  << ", p = " << result.winProbability << std::endl;

        std::cout << std::endl;

        std::cout << "Row to flip: ";
        std::cin >> chosen_row;
        std::cout << "Column to flip: ";
        std::cin >> chosen_column;
        std::cout << std::endl;

        covered_board.panel[chosen_row][chosen_column] = real_board.panel[chosen_row][chosen_column];

        possible_0_positions.clear();
        for (int type = 0; type < 10; type++) {
            compatible_boards[type].clear();
            compatible_boards_without_conflict[type].clear();
        }
    }

    print_board(covered_board);

    return 0;
}
