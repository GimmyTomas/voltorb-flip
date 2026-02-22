#include <cassert>
//
//  main.cpp
//  voltorb-flip
//
//  Created by Giovanni Maria Tomaselli on 16/04/22.
//

#include <cstdlib>
#include <iostream>
#include <ctime>
#include <list>
#include <fstream>
#include <algorithm>
#include <vector>
#include <string>
#include <random>
#include <iomanip>

int factorial (int n)
{
    if (n < 0) return 0;
    return !n ? 1: n* factorial(n-1);
}

auto getRandomSeed() -> std::seed_seq
{
    std::random_device source;

    unsigned int random_data[10];
    for(auto& elem : random_data) elem = source();
    
    return std::seed_seq(random_data + 0, random_data + 10);
}

int rand_uniform (int a, int b) {
    static auto seed = getRandomSeed();
    static std::mt19937 rng(seed);

    std::uniform_int_distribution<int> dist(a, b);
    return dist(rng);
}

struct parameters
{
    int8_t n_0;
    int8_t n_2;
    int8_t n_3;
    int8_t max_total_free;
    int8_t max_row_free;
};

parameters board_type[8][10] = {
    {{6,3,1,3,3},{6,0,3,2,2},{6,5,0,4,3},{6,2,2,3,3},{6,4,1,4,3},{6,3,1,3,3},{6,0,3,2,2},{6,5,0,4,3},{6,2,2,3,3},{6,4,1,4,3}},
    {{7,1,3,3,2},{7,6,0,4,3},{7,3,2,3,2},{7,0,4,3,2},{7,5,1,4,3},{7,1,3,2,2},{7,6,0,3,3},{7,3,2,2,2},{7,0,4,2,2},{7,5,1,3,3}},
    {{8,2,3,3,2},{8,7,0,4,3},{8,4,2,4,3},{8,1,4,3,2},{8,6,1,3,4},{8,2,3,2,2},{8,7,0,3,3},{8,4,2,3,3},{8,1,4,2,2},{8,6,1,3,3}},
    {{8,3,3,3,4},{8,0,5,3,2},{10,8,0,5,4},{10,5,2,4,3},{10,2,4,4,3},{8,3,3,3,3},{8,0,5,2,2},{10,8,0,4,4},{10,5,2,3,3},{10,2,4,3,3}},
    {{10,7,1,5,4},{10,4,3,4,3},{10,1,5,4,3},{10,9,0,5,4},{10,6,2,5,4},{10,7,1,4,4},{10,4,3,3,3},{10,1,5,3,3},{10,9,0,4,4},{10,6,2,4,4}},
    {{10,3,4,4,3},{10,0,6,4,3},{10,8,1,5,4},{10,5,3,5,4},{10,2,5,4,3},{10,3,4,3,3},{10,0,6,3,3},{10,8,1,4,4},{10,5,3,4,4},{10,2,5,3,3}},
    {{10,7,2,5,4},{10,4,4,5,4},{13,1,6,4,3},{13,9,1,6,5},{10,6,3,5,4},{10,7,2,4,4},{10,4,4,4,4},{13,1,6,3,3},{13,9,1,5,5},{10,6,3,4,4}},
    {{10,0,7,4,3},{10,8,2,6,5},{10,5,4,5,4},{10,2,6,5,4},{10,7,3,6,5},{10,0,7,3,3},{10,8,2,5,5},{10,5,4,4,4},{10,2,6,4,4},{10,7,3,5,5}}
};

long int N_permutations[8][10] = { // = 25!/(n0!n1!n2!n3!)
    {2745758400, 171609900, 2059318800, 4118637600, 10296594000, 2745758400, 171609900, 2059318800, 4118637600, 10296594000},
    {5883768000, 8923714800, 41186376000, 1470942000, 53542288800, 5883768000, 8923714800, 41186376000, 1470942000, 53542288800},
    {66927861000, 21034470600, 200783583000, 33463930500, 147241294200, 66927861000, 21034470600, 200783583000, 33463930500, 147241294200},
    {267711444000, 6692786100, 21034470600, 441723882600, 245402157000, 267711444000, 6692786100, 21034470600, 441723882600, 245402157000},
    {168275764800, 736206471000, 98160862800, 16360143800, 588965176800, 168275764800, 736206471000, 98160862800, 16360143800, 588965176800},
    {736206471000, 16360143800, 147241294200, 1177930353600, 441723882600, 736206471000, 16360143800, 147241294200, 1177930353600, 441723882600},
    {588965176800, 1472412942000, 28830463200, 3432198000, 1374252079200, 588965176800, 1472412942000, 28830463200, 3432198000, 1374252079200},
    {21034470600, 441723882600, 2061378118800, 588965176800, 1177930353600, 21034470600, 441723882600, 2061378118800, 588965176800, 1177930353600}
};

long int N_permutations_gcd[8] = {171609900, 98062800, 956112300, 318704100, 2337163400, 16360143800, 686439600, 21034470600};

int N_permutations_reduced[8][10] = { // = 25!/(n0!n1!n2!n3!), divided by the greatest common divisor per each Level
    {16, 1, 12, 24, 60, 16, 1, 12, 24, 60},             // gcd =    171,609,900
    {60, 91, 420, 15, 546, 60, 91, 420, 15, 546},       // gcd =     98,062,800
    {70, 22, 210, 35, 154, 70, 22, 210, 35, 154},       // gcd =    956,112,300
    {840, 21, 66, 1386, 770, 840, 21, 66, 1386, 770},   // gcd =    318,704,100
    {72, 315, 42, 7, 252, 72, 315, 42, 7, 252},         // gcd =  2,337,163,400
    {45, 1, 9, 72, 27, 45, 1, 9, 72, 27},               // gcd = 16,360,143,800
    {858, 2145, 42, 5, 2002, 858, 2145, 42, 5, 2002},   // gcd =    686,439,600
    {1, 21, 98, 28, 56, 1, 21, 98, 28, 56}              // gcd = 21,034,470,600
};

double acceptance_rate[8][10] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
};

long int N_accepted[8][10] = { // = valid boards
    {1732660000, 81056200, 1407934200, 2598990000, 7039671000, 1732660000, 81056200, 1407934200, 2598990000, 7039671000},
    {3285100800, 5722710400, 17316544000, 821275200, 34336262400, 2684683200, 4495352000, 14348488000, 671170800, 26972112000},
    {34984058000, 12979316000, 145577634000, 17492029000, 81740052800, 32177972000, 11677150400, 128566014000, 16088986000, 81740052800},
    {171421352000, 3498405800, 18355191900, 335965442400, 204113718000, 171421352000, 3217797200, 17976411900, 331634822400, 199722318000},
    {146841535200, 559942404000, 81645487200, 13300405700, 513945373200, 143811295200, 552724704000, 79888927200, 13159573700, 503339533200},
    {559942404000, 13607581200, 119703651300, 1027890746400, 335965442400, 552724704000, 13314821200, 118436163300, 1006679066400, 331634822400},
    {478814605200, 1284863433000, 25901458800, 3265542000, 1117234078800, 473744653200, 1258348833000, 25901458800, 3265542000, 1105404190800},
    {15998354400, 400990788900, 1675851118200, 513945373200, 1069308770400, 15792134400, 394129278900, 1658106286200, 503339533200, 1051011410400}
};

int N_accepted_gcd[8] = {200, 400, 200, 100, 100, 100, 600, 300};

long int N_accepted_reduced[8][10] = { // = valid boards, divided by the greatest common divisor per each Level
    {8663300, 405281, 7039671, 12994950, 35198355, 8663300, 405281, 7039671, 12994950, 35198355},   // gcd = 200
    {8212752, 14306776, 43291360, 2053188, 85840656, 6711708, 11238380, 35871220, 1677927, 67430280},   // gcd = 400
    {174920290, 64896580, 727888170, 87460145, 408700264, 160889860, 58385752, 642830070, 80444930, 408700264}, // gcd = 200
    {1714213520, 34984058, 183551919, 3359654424, 2041137180, 1714213520, 32177972, 179764119, 3316348224, 1997223180}, // gcd = 100
    {1468415352, 5599424040, 816454872, 133004057, 5139453732, 1438112952, 5527247040, 798889272, 131595737, 5033395332},   // gcd = 100
    {5599424040, 136075812, 1197036513, 10278907464, 3359654424, 5527247040, 133148212, 1184361633, 10066790664, 3316348224},   // gcd = 100
    {798024342, 2141439055, 43169098, 5442570, 1862056798, 789574422, 2097248055, 43169098, 5442570, 1842340318},   // gcd = 600
    {53327848, 1336635963, 5586170394, 1713151244, 3564362568, 52640448, 1313764263, 5527020954, 1677798444, 3503371368}    // gcd = 300
};


struct board
{
    int8_t level;
    int8_t panel[5][5]; // -1 = unflipped, 0 = voltorb
    int8_t row_sum[5];
    int8_t row_0[5];
    int8_t column_sum[5];
    int8_t column_0[5];
};

struct hash_item
{
    board board;
    int suggested_row;
    int suggested_column;
    double win_chance;
};

void print_board (board board, int row_suggested = 5, int column_suggested = 5)
{
    std::cout << std::endl;
    std::cout << "\t       Level " << unsigned(board.level) << std::endl;
    for (int i = 0; i < 5; i++)
    {
        std::cout << "\t---------------------" << std::endl;
        std::cout << "\t";
        for (int j = 0; j < 5; j++)
        {
            if (i == row_suggested && j == column_suggested) std::cout << "|###";
            else
            {
                if (board.panel[i][j] == -1) std::cout << "|   ";
                else std::cout << "| " << unsigned(board.panel[i][j]) << " ";
            }
        }
        std::cout << "| " << unsigned(board.row_sum[i]) << "," << unsigned(board.row_0[i]) << std::endl;
    }
    std::cout << "\t---------------------" << std::endl;
    std::cout << "\t";
    for (int j = 0; j < 5; j++)
    {
        if (board.column_sum[j] < 10) std::cout << "  " << unsigned(board.column_sum[j]) << " ";
        else std::cout << " " << unsigned(board.column_sum[j]) << " ";
    }
    std::cout << std::endl;
    std::cout << "\t";
    for (int j = 0; j < 5; j++) std::cout << "  " << unsigned(board.column_0[j]) << " ";
    std::cout << std::endl;
    std::cout << std::endl;
}

void cover_all_panels (board & real_board, board & covered_board)
// sets covered_board equal to real_board but with all panels covered
{
    covered_board = real_board;
    
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++)
            covered_board.panel[i][j] = -1;
}

bool check_legality_board (parameters & parameters, board & board)
{
    int total_free_multipliers = 0;
    int row_free_multipliers = 0;
    
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++)
            if ( (board.row_0[i] == 0 || board.column_0[j] == 0) && board.panel[i][j] > 1)
                    total_free_multipliers++;
    if (total_free_multipliers >= parameters.max_total_free) return false;
        
    for (int i = 0; i < 5; i++)
        if (board.row_0[i] == 0)
        {
            row_free_multipliers = 0;
            for (int j = 0; j < 5; j++)
                if (board.panel[i][j] > 1)
                    row_free_multipliers++;
            if (row_free_multipliers >= parameters.max_row_free) return false;
        }
    
    for (int j = 0; j < 5; j++)
        if (board.column_0[j] == 0)
        {
            row_free_multipliers = 0;
            for (int i = 0; i < 5; i++)
                if (board.panel[i][j] > 1)
                    row_free_multipliers++;
            if (row_free_multipliers >= parameters.max_row_free) return false;
        }
    
    return true;
}

int random_initialize_board (int8_t level, board & board, int type = rand_uniform(0,9), int number_of_tries = 1)
// returns the number of tries (= 1 + number of rejections)
{
    std::cout << "Type = " << type << std::endl;
    int i,j;
    parameters parameters = board_type[level-1][type];
    
    board.level = level;
    
    for (i = 0; i < 5; i++)
        for (j = 0; j < 5; j++)
            board.panel[i][j] = 1;
    
    while (parameters.n_0 > 0)
    {
        i = rand_uniform(0,4);
        j = rand_uniform(0,4);
        if (board.panel[i][j] == 1)
        {
            board.panel[i][j] = 0;
            parameters.n_0--;
        }
    }
    
    while (parameters.n_2 > 0)
    {
        i = rand_uniform(0,4);
        j = rand_uniform(0,4);
        if (board.panel[i][j] == 1)
        {
            board.panel[i][j] = 2;
            parameters.n_2--;
        }
    }
    
    while (parameters.n_3 > 0)
    {
        i = rand_uniform(0,4);
        j = rand_uniform(0,4);
        if (board.panel[i][j] == 1)
        {
            board.panel[i][j] = 3;
            parameters.n_3--;
        }
    }
    
    for (i = 0; i < 5; i++)
    {
        board.row_sum[i] = board.panel[i][0] + board.panel[i][1] + board.panel[i][2] + board.panel[i][3] + board.panel[i][4];
        board.row_0[i] = (board.panel[i][0] == 0) + (board.panel[i][1] == 0) + (board.panel[i][2] == 0) + (board.panel[i][3] == 0) + (board.panel[i][4] == 0);
    }
    
    for (j = 0; j < 5; j++)
    {
        board.column_sum[j] = board.panel[0][j] + board.panel[1][j] + board.panel[2][j] + board.panel[3][j] + board.panel[4][j];
        board.column_0[j] = (board.panel[0][j] == 0) + (board.panel[1][j] == 0) + (board.panel[2][j] == 0) + (board.panel[3][j] == 0) + (board.panel[4][j] == 0);
    }
    
    if (check_legality_board(parameters,board))
        return number_of_tries;
    else
        return random_initialize_board(level,board,type,number_of_tries+1);
}

void manual_initialize_board(board & board)
{
    board.level = 4;
    
    int type = 4;
        
    int8_t panels[5][5] = {
        {0,1,3,0,1},
        {3,0,1,0,1},
        {1,3,0,0,1},
        {0,0,0,0,2},
        {1,1,1,2,3}
    };
    
    std::copy(&panels[0][0], &panels[0][0]+25, &board.panel[0][0]);
    
    int n_0, sum;
    
    for (int i = 0; i < 5; i++)
    {
        n_0 = 0;
        sum = 0;
        for (int j = 0; j < 5; j++)
        {
            if (board.panel[i][j] == 0)
                n_0++;
            sum += board.panel[i][j];
        }
        board.row_sum[i] = sum;
        board.row_0[i] = n_0;
    }
    
    for (int j = 0; j < 5; j++)
    {
        n_0 = 0;
        sum = 0;
        for (int i = 0; i < 5; i++)
        {
            if (board.panel[i][j] == 0)
                n_0++;
            sum += board.panel[i][j];
        }
        board.column_sum[j] = sum;
        board.column_0[j] = n_0;
    }
    
    assert(check_legality_board(board_type[board.level-1][type], board));
}

void test_acceptance_rate()
{
    long int N = 100*1000*1000; // number of accepted boards per each type
    
    int level;
    
    std::cout << "What Level do you want to test the accept/reject rate of? ";
    std::cin >> level;
    
    long int tries[10] = {0,0,0,0,0,0,0,0,0,0};
    
    board test_board;
    
    for (long int n = 0; n <= N; n++)
    {
        for (int type = 0; type < 10; type++)
            tries[type] += random_initialize_board(level,test_board,type);
        if ((n+1)%(N/100) == 0) std::cout << 100*(n+1)/N << "%" << std::endl;
    }
    
    // I do N instead of N+1, and tries-1 instead of tries, to eliminate the bias given by the last try always being successful
    std::cout << "Level " << level << std::endl;
    for (int type = 0; type < 10; type++)
        std::cout << "Type " << type << "\t\t" << N << "\t" << tries[type]-1 << "\t" << (double)N/(tries[type]-1) << "   \t" << sqrt( ((double)N/(tries[type]-1)) * (1-(double)N/(tries[type]-1)) / (tries[type]-1) ) << std::endl;
}

void calculate_number_of_valid_boards()
{
    int level, type_selected;
    
    std::cout << "What Level do you want to calculate the number of valid boards of? ";
    std::cin >> level;
    std::cout << "What type? (enter 10 to do all types from 0 to 9) ";
    std::cin >> type_selected;
    
    int i, j, n, n_0;
    long int total_number, accepted_number;
    
    std::string positions_0;
    std::string positions_2;
    
    board test, test2;
    
    for (int type = ((type_selected == 10) ? 0 : type_selected); type <= ((type_selected == 10) ? 9 : type_selected); type++)
    {
        total_number = 0;
        accepted_number = 0;
        
        positions_0.clear();
        positions_0.resize(25, 0);
        std::fill(positions_0.begin(), positions_0.begin() + board_type[level-1][type].n_0, 1);
        
        do {
            n = 0;
            for (i = 0; i < 5; i++)
                for (j = 0; j < 5; j++)
                {
                    if (positions_0[n] == 1)
                        test.panel[i][j] = 0;
                    else
                        test.panel[i][j] = -1;
                    n++;
                }
            
            for (i = 0; i < 5; i++)
            {
                n_0 = 0;
                for (j = 0; j < 5; j++)
                    if (test.panel[i][j] == 0)
                        n_0++;
                test.row_0[i] = n_0;
            }
            
            for (j = 0; j < 5; j++)
            {
                n_0 = 0;
                for (i = 0; i < 5; i++)
                    if (test.panel[i][j] == 0)
                        n_0++;
                test.column_0[j] = n_0;
            }
            
            positions_2.clear();
            positions_2.resize(25 - board_type[level-1][type].n_0, 0);
            std::fill(positions_2.begin(), positions_2.begin() + board_type[level-1][type].n_2 + board_type[level-1][type].n_3, 1);
            
            do {
                test2 = test;
                n = 0;
                for (i = 0; i < 5; i++)
                    for (j = 0; j < 5; j++)
                        if (test.panel[i][j] == -1)
                        {
                            if (positions_2[n] == 1)
                                test2.panel[i][j] = 2;
                            else
                                test2.panel[i][j] = 1;
                            n++;
                        }
                total_number++;
                if (total_number % 100000000 == 0)
                    std::cout << total_number << std::endl;
                if (check_legality_board(board_type[level-1][type], test2))
                    accepted_number++;
                
            } while(std::prev_permutation(positions_2.begin(), positions_2.end()));
        } while(std::prev_permutation(positions_0.begin(), positions_0.end()));
        
        accepted_number *= factorial(board_type[level-1][type].n_2 + board_type[level-1][type].n_3)/(factorial(board_type[level-1][type].n_2) * factorial(board_type[level-1][type].n_3));
        total_number *= factorial(board_type[level-1][type].n_2 + board_type[level-1][type].n_3)/(factorial(board_type[level-1][type].n_2) * factorial(board_type[level-1][type].n_3));
        
        std::cout << "Type " << type << "\t\t" << accepted_number << "\t" << total_number << "\t" << (double)accepted_number/total_number << std::endl;
    }
}

int is_won (board board)
// -1 if board is not finished, 0 if it is lost, 1 if it is won
{
    int total_flipped = 0;
    
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++)
        {
            if (board.panel[i][j] == 0) return 0;
            else total_flipped += abs(board.panel[i][j]);
        }
    
    int total = board.row_sum[0] + board.row_sum[1] + board.row_sum[2] + board.row_sum[3] + board.row_sum[4] + board.row_0[0] + board.row_0[1] + board.row_0[2] + board.row_0[3] + board.row_0[4];
    
    if (total == total_flipped) return 1;
    else return -1;
}

bool valid_0_positions (board & real_board, std::string positions_0[5], int rows_to_consider)
// checks whether a distribution of the 0's in the first rows_to_consider rows can already be excluded from the information known about the columns
{
    int sum[5] = {0,0,0,0,0};
    for (int j = 0; j < 5; j++)
        for (int i = 0; i < rows_to_consider; i++)
            sum[j] += positions_0[i][j];
    
    if (sum[0] <= real_board.column_0[0] && sum[1] <= real_board.column_0[1] && sum[2] <= real_board.column_0[2] && sum[3] <= real_board.column_0[3] && sum[4] <= real_board.column_0[4])
        return 1;
    else
        return 0;
}

bool no_conflict (board & real_board, board & test_board)
{
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++)
            if ((real_board.panel[i][j] != -1) && (test_board.panel[i][j] != -1) && (real_board.panel[i][j] != test_board.panel[i][j]))
                return 0;
    return 1;
}

void generate_0_positions (board & real_board, std::vector<board> & possible_0_positions)
// fills possible_0_positions
{
    int i,j;
    board test;
    
    std::string positions_0[5];
    for (i = 0; i < 5; i++)
    {
        positions_0[i].resize(5, 0);
        std::fill(positions_0[i].begin(), positions_0[i].begin() + real_board.row_0[i], 1);
    }
    
    possible_0_positions.clear();
    
    do {
        if (valid_0_positions(real_board,positions_0,1))
        {
            do {
                if (valid_0_positions(real_board,positions_0,2))
                {
                    do {
                        if (valid_0_positions(real_board,positions_0,3))
                        {
                            do {
                                if (valid_0_positions(real_board,positions_0,4))
                                {
                                    do {
                                        if (valid_0_positions(real_board,positions_0,5))
                                        {
                                            test = real_board;
                                            for (i = 0; i < 5; i++)
                                                for (j = 0; j < 5; j++)
                                                    test.panel[i][j] = positions_0[i][j]-1;
                                            if (no_conflict (real_board, test))
                                                possible_0_positions.push_back(test);
                                        }
                                    } while(std::prev_permutation(positions_0[4].begin(), positions_0[4].end()));
                                }
                            } while(std::prev_permutation(positions_0[3].begin(), positions_0[3].end()));
                        }
                    } while(std::prev_permutation(positions_0[2].begin(), positions_0[2].end()));
                }
            } while(std::prev_permutation(positions_0[1].begin(), positions_0[1].end()));
        }
    } while(std::prev_permutation(positions_0[0].begin(), positions_0[0].end()));
}

bool compatible_type(board & real_board, uint8_t type)
// checks whether the known info or the panels already flipped/filled in real_board can immediately rule out that real_board is of a given type (because they exceed the availability of 0's, 2's and 3's, or because the sum total doesn't match)
{
    int min_number_of[4] = {0,0,0,0};
    
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++)
            if (real_board.panel[i][j] > 0) // could also be > -1
                min_number_of[real_board.panel[i][j]] += 1;
    if (!(min_number_of[1] <= 25 - board_type[real_board.level-1][type].n_0 - board_type[real_board.level-1][type].n_2 - board_type[real_board.level-1][type].n_3))
        return 0;
    if (!(min_number_of[2] <= board_type[real_board.level-1][type].n_2))
        return 0;
    if (!(min_number_of[3] <= board_type[real_board.level-1][type].n_3))
        return 0;
    
    if (!(check_legality_board(board_type[real_board.level-1][type], real_board)))
        return 0;
    
    int total_bombs = real_board.row_0[0] + real_board.row_0[1] + real_board.row_0[2] + real_board.row_0[3] + real_board.row_0[4];
    if (!(total_bombs == board_type[real_board.level-1][type].n_0))
        return 0;
    
    int total_sums = real_board.row_sum[0] + real_board.row_sum[1] + real_board.row_sum[2] + real_board.row_sum[3] + real_board.row_sum[4];
    if (!(total_sums == (25 - board_type[real_board.level-1][type].n_0 + 1*board_type[real_board.level-1][type].n_2 + 2*board_type[real_board.level-1][type].n_3)))
        return 0;
            
    return 1;
}

bool fully_flipped (board & real_board)
// checks whether all panels are flipped
{
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++)
            if (real_board.panel[i][j] == -1)
                return 0;
    
    return 1;
}

bool are_equal (board & real_board, board & test_board)
{
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++)
            if (real_board.panel[i][j] != test_board.panel[i][j])
                return 0;
    return 1;
}

bool numbers_dont_exceed (board & real_board)
// check whether a partially flipped/filled board (whose -1's stand for non-0 because the 0's have already been positioned) violates some of its own constraints from row_sum and column_sum
{
    int sum, sum_abs;
    
    bool totally_flipped_row;
    
    for (int j = 0; j < 5; j++)
    {
        totally_flipped_row = 1;
        sum = 0;
        sum_abs = 0;
        
        for (int i = 0; i < 5; i++)
        {
            sum += real_board.panel[i][j];
            sum_abs += abs(real_board.panel[i][j]);
            if (real_board.panel[i][j] == -1)
                totally_flipped_row = 0;
        }
        if (sum_abs > real_board.column_sum[j])
            return 0;
        if ((sum != real_board.column_sum[j]) && totally_flipped_row)
            return 0;
    }
    
    for (int i = 0; i < 5; i++)
    {
        totally_flipped_row = 1;
        sum = 0;
        sum_abs = 0;
        
        for (int j = 0; j < 5; j++)
        {
            sum += real_board.panel[i][j];
            sum_abs += abs(real_board.panel[i][j]);
            if (real_board.panel[i][j] == -1)
                totally_flipped_row = 0;
        }
        if (sum_abs > real_board.row_sum[i])
            return 0;
        if ((sum != real_board.row_sum[i]) && totally_flipped_row)
            return 0;
    }
        
    return 1;
}

int combinations (int total, int n_covered)
{
    if (n_covered == 0)
        return 999999;
    
    int result = 0;
        
    for (int n_3 = ((total - 2*n_covered > 0) ? (total - 2*n_covered) : 0); n_3 <= (total - n_covered)/2; n_3++)
        result += factorial(n_covered) / (factorial(2*n_covered - total + n_3) * factorial(total - n_covered - 2*n_3) * factorial(n_3));
        
    return (result == 0) ? 999999 : result;
}

void find_and_fill_row_with_possibilities (std::vector<board>::iterator it, std::vector<board> & compatible_boards_of_type, uint8_t type)
// pushes_back in compatible_boards_of_type the boards obtained from (*it) considering all possibilities coming from flipping one of its rows/columns; chooses with row/column to flip in a clever way; removes (+it) from compatible_boards_of_type
{
    ptrdiff_t pos = it - compatible_boards_of_type.begin();
    
    int i, j, n;
    board test, test2;
        
    int total, n_covered;
    int min_combinations = 999999;
    
    int i_or_j_chosen = 0;
    int i_or_j_other;
    bool row_or_column = 1; // 1 means that i_or_j_chosen should be read as an i (row); 0 means column
    
    std::string positions_2;
    std::string positions_3;
    int n_2, n_3;
    
    for (i = 0; i < 5; i++)
    {
        total = compatible_boards_of_type[pos].row_sum[i];
        n_covered = 5;
        for (j = 0; j < 5; j++)
        {
            total -= ( (compatible_boards_of_type[pos].panel[i][j] == -1) ? 0 : compatible_boards_of_type[pos].panel[i][j] );
            n_covered -= ( (compatible_boards_of_type[pos].panel[i][j] == -1) ? 0 : 1 );
        }
        if (min_combinations > combinations(total, n_covered))
        {
            min_combinations = combinations(total, n_covered);
            i_or_j_chosen = i;
            row_or_column = 1;
        }
    }
    
    for (j = 0; j < 5; j++)
    {
        total = compatible_boards_of_type[pos].column_sum[j];
        n_covered = 5;
        for (i = 0; i < 5; i++)
        {
            total -= ( (compatible_boards_of_type[pos].panel[i][j] == -1) ? 0 : compatible_boards_of_type[pos].panel[i][j] );
            n_covered -= ( (compatible_boards_of_type[pos].panel[i][j] == -1) ? 0 : 1 );
        }
        if (min_combinations > combinations(total, n_covered))
        {
            min_combinations = combinations(total, n_covered);
            i_or_j_chosen = j;
            row_or_column = 0;
        }
    }
    
    //std::cout << "row or column = " << unsigned(row_or_column) << ", i or j chosen = " << i_or_j_chosen << std::endl;
    
    if (row_or_column)
    {
        total = compatible_boards_of_type[pos].row_sum[i_or_j_chosen];
        n_covered = 5;
        for (i_or_j_other = 0; i_or_j_other < 5; i_or_j_other++)
        {
            total -= ( (compatible_boards_of_type[pos].panel[i_or_j_chosen][i_or_j_other] == -1) ? 0 : compatible_boards_of_type[pos].panel[i_or_j_chosen][i_or_j_other] );
            n_covered -= ( (compatible_boards_of_type[pos].panel[i_or_j_chosen][i_or_j_other] == -1) ? 0 : 1 );
        }
        //std::cout << "total = " << total << ", n_covered = " << n_covered << ", combination = " << combinations(total, n_covered) << std::endl;
        
        for (n_3 = ((total - 2*n_covered > 0) ? (total - 2*n_covered) : 0); n_3 <= (total - n_covered)/2; n_3++)
        {
            n_2 = total - n_covered - 2*n_3;
            positions_2.clear();
            positions_2.resize(n_covered, 0);
            std::fill(positions_2.begin(), positions_2.begin() + n_2, 1);
            
            //std::cout << "n_3 = " << n_3 << ", n_2 = " << n_2 << std::endl;
            
            do {
                test = *it;
                
                n = 0;
                for (i_or_j_other = 0; i_or_j_other < 5; i_or_j_other++)
                    if (compatible_boards_of_type[pos].panel[i_or_j_chosen][i_or_j_other] == -1)
                    {
                        if (positions_2[n] == 1)
                            test.panel[i_or_j_chosen][i_or_j_other] = 2;
                        n++;
                    }
                
                if (numbers_dont_exceed(test))
                {
                    positions_3.clear();
                    positions_3.resize(n_covered, 0);
                    std::fill(positions_3.begin(), positions_3.begin() + n_3, 1);
                    
                    do {
                        test2 = test;
                        
                        n = 0;
                        for (i_or_j_other = 0; i_or_j_other < 5; i_or_j_other++)
                            if (test.panel[i_or_j_chosen][i_or_j_other] == -1)
                            {
                                if (positions_3[n] == 1)
                                    test2.panel[i_or_j_chosen][i_or_j_other] = 3;
                                n++;
                            }
                        
                        if (numbers_dont_exceed(test2))
                        {
                            for (i_or_j_other = 0; i_or_j_other < 5; i_or_j_other++)
                                if (test2.panel[i_or_j_chosen][i_or_j_other] == -1)
                                    test2.panel[i_or_j_chosen][i_or_j_other] = 1;
                            
                            if (compatible_type(test2, type))
                                compatible_boards_of_type.push_back(test2);
                        }
                    } while(std::prev_permutation(positions_3.begin(), positions_3.end()));
                }
            } while(std::prev_permutation(positions_2.begin(), positions_2.end()));
        }
    }
    else
    {
        total = compatible_boards_of_type[pos].column_sum[i_or_j_chosen];
        n_covered = 5;
        for (i_or_j_other = 0; i_or_j_other < 5; i_or_j_other++)
        {
            total -= ( (compatible_boards_of_type[pos].panel[i_or_j_other][i_or_j_chosen] == -1) ? 0 : compatible_boards_of_type[pos].panel[i_or_j_other][i_or_j_chosen] );
            n_covered -= ( (compatible_boards_of_type[pos].panel[i_or_j_other][i_or_j_chosen] == -1) ? 0 : 1 );
        }
        //std::cout << "total = " << total << ", n_covered = " << n_covered << ", combination = " << combinations(total, n_covered) << std::endl;
        for (n_3 = ((total - 2*n_covered > 0) ? (total - 2*n_covered) : 0); n_3 <= (total - n_covered)/2; n_3++)
        {
            n_2 = total - n_covered - 2*n_3;
            positions_2.clear();
            positions_2.resize(n_covered, 0);
            std::fill(positions_2.begin(), positions_2.begin() + n_2, 1);
            
            //std::cout << "n_3 = " << n_3 << ", n_2 = " << n_2 << std::endl;
            
            do {
                test = *it;
                
                n = 0;
                for (i_or_j_other = 0; i_or_j_other < 5; i_or_j_other++)
                    if (compatible_boards_of_type[pos].panel[i_or_j_other][i_or_j_chosen] == -1)
                    {
                        if (positions_2[n] == 1)
                            test.panel[i_or_j_other][i_or_j_chosen] = 2;
                        n++;
                    }
                                
                if (numbers_dont_exceed(test))
                {
                    positions_3.clear();
                    positions_3.resize(n_covered, 0);
                    std::fill(positions_3.begin(), positions_3.begin() + n_3, 1);
                    
                    do {
                        test2 = test;
                        
                        n = 0;
                        for (i_or_j_other = 0; i_or_j_other < 5; i_or_j_other++)
                            if (test.panel[i_or_j_other][i_or_j_chosen] == -1)
                            {
                                if (positions_3[n] == 1)
                                    test2.panel[i_or_j_other][i_or_j_chosen] = 3;
                                n++;
                            }
                        
                        if (numbers_dont_exceed(test2))
                        {
                            for (i_or_j_other = 0; i_or_j_other < 5; i_or_j_other++)
                                if (test2.panel[i_or_j_other][i_or_j_chosen] == -1)
                                    test2.panel[i_or_j_other][i_or_j_chosen] = 1;
                            
                            if (compatible_type(test2, type))
                                compatible_boards_of_type.push_back(test2);
                        }
                    } while(std::prev_permutation(positions_3.begin(), positions_3.end()));
                }
            } while(std::prev_permutation(positions_2.begin(), positions_2.end()));
        }
    }
    
    compatible_boards_of_type.erase(compatible_boards_of_type.begin() + pos);
}

void generate_compatible_boards (board & real_board, std::vector<board> & possible_0_positions, std::vector<board> compatible_boards[10])
// fills compatible_boards
{
    std::vector<board>::iterator it;
    int i, j;
    
    auto p = [](board b){ return !fully_flipped(b); };
    
    for (int type = 0; type < 10; type++)
        if (compatible_type(real_board, type))
        {
            compatible_boards[type] = possible_0_positions;
            
            for (it = compatible_boards[type].begin(); it != compatible_boards[type].end(); it++)
                for (i = 0; i < 5; i++)
                    for (j = 0; j < 5; j++)
                    {
                        if (real_board.panel[i][j] != -1)
                        {
                            if ((*it).panel[i][j] == -1)
                                (*it).panel[i][j] = real_board.panel[i][j];
                            else
                                compatible_boards[type].erase(it);
                        }
                        if (real_board.panel[i][j] == 0)
                        {
                            std::cout << "erhmmmm... your board is already lost" << std::endl;
                            return;
                        }
                    }
            
            it = std::find_if(compatible_boards[type].begin(), compatible_boards[type].end(), p);
            while ( it != compatible_boards[type].end())
            {
                //print_board(*it);
                //std::cout << "type = " << type << ", size = " << compatible_boards[type].size() << std::endl;
                find_and_fill_row_with_possibilities(it, compatible_boards[type], type);
                it = std::find_if(compatible_boards[type].begin(), compatible_boards[type].end(), p);
            }
        }
}

bool double_check_everything (board & real_board, uint8_t type)
{
    int sum_in_a_row = 0;
    int n_0_in_a_row = 0;
    int number_of[4] = {0,0,0,0};
    
    for (int i = 0; i < 5; i++)
    {
        n_0_in_a_row = 0;
        sum_in_a_row = 0;
        for (int j = 0; j < 5; j++)
        {
            number_of[real_board.panel[i][j]]++;
            if (real_board.panel[i][j] == 0)
                n_0_in_a_row++;
            sum_in_a_row += real_board.panel[i][j];
        }
        
        if ((sum_in_a_row != real_board.row_sum[i]) || (n_0_in_a_row != real_board.row_0[i]) )
            return 0;
    }
    
    for (int j = 0; j < 5; j++)
    {
        n_0_in_a_row = 0;
        sum_in_a_row = 0;
        for (int i = 0; i < 5; i++)
        {
            if (real_board.panel[i][j] == 0)
                n_0_in_a_row++;
            sum_in_a_row += real_board.panel[i][j];
        }
        
        if ((sum_in_a_row != real_board.column_sum[j]) || (n_0_in_a_row != real_board.column_0[j]) )
            return 0;
    }
    
    if ((number_of[0] != board_type[real_board.level-1][type].n_0) || (number_of[2] != board_type[real_board.level-1][type].n_2) || (number_of[3] != board_type[real_board.level-1][type].n_3))
        return 0;
    
    return check_legality_board(board_type[real_board.level-1][type], real_board) && fully_flipped(real_board) && numbers_dont_exceed(real_board) && compatible_type(real_board, type);
}

void brutal_generate_compatible_boards (board & real_board, std::vector<board> & possible_0_positions, std::vector<board> compatible_boards[10])
// fills with 1's, 2's and 3's with brute force, intentionally redundant in checking correctness
{
    int i, j, n;
    board test, test2, test3;
    
    int total_bombs = real_board.row_0[0] + real_board.row_0[1] + real_board.row_0[2] + real_board.row_0[3] + real_board.row_0[4];
    
    int min_number_of[4] = {0,0,0,0};
    
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++)
            if (real_board.panel[i][j] > 0) // could also be > -1
                min_number_of[real_board.panel[i][j]] += 1;
    
    std::string positions_2;
    std::string positions_3;
    
    for (int type = 0; type < 10; type++)
        if (compatible_type(real_board,type))
            for (std::vector<board>::iterator it = possible_0_positions.begin(); it != possible_0_positions.end(); ++it)
            {
                positions_2.clear();
                positions_2.resize(25 - total_bombs - min_number_of[1] - min_number_of[2] - min_number_of[3], 0);
                std::fill(positions_2.begin(), positions_2.begin() + board_type[real_board.level-1][type].n_2 - min_number_of[2], 1);
                
                //std::cout << "n_2 = " << unsigned(board_type[real_board.level-1][type].n_2) << " - " << min_number_of[2] << std::endl;
                
                do {
                    test = *it;
                    
                    for (i = 0; i < 5; i++)
                        for (j = 0; j < 5; j++)
                            if (real_board.panel[i][j] > 0)
                                test.panel[i][j] = real_board.panel[i][j];
                    
                    n = 0;
                    for (i = 0; i < 5; i++)
                        for (j = 0; j < 5; j++)
                            if ((*it).panel[i][j] == -1)
                            {
                                if (positions_2[n] == 1)
                                    test.panel[i][j] = 2;
                                n++;
                            }
                    //std::cout << "test, type = " << type << std::endl;
                    //print_board(test);
                    
                    if (numbers_dont_exceed(test))
                    {
                        positions_3.clear();
                        positions_3.resize(25 - total_bombs-min_number_of[1] - board_type[test.level-1][type].n_2 - min_number_of[3], 0);
                        std::fill(positions_3.begin(), positions_3.begin() + board_type[test.level-1][type].n_3 - min_number_of[3], 1);
                        
                        do {
                            test2 = test;
                            
                            n = 0;
                            for (i = 0; i < 5; i++)
                                for (j = 0; j < 5; j++)
                                    if (test.panel[i][j] == -1)
                                    {
                                        if (positions_3[n] == 1)
                                            test2.panel[i][j] = 3;
                                        n++;
                                    }
                            //std::cout << "test2, type = " << type << std::endl;
                            //print_board(test2);
                            
                            if (numbers_dont_exceed(test2))
                            {
                                test3 = test2;
                                
                                for (i = 0; i < 5; i++)
                                    for (j = 0; j < 5; j++)
                                        if (test2.panel[i][j] == -1)
                                            test3.panel[i][j] = 1;
                                if (double_check_everything(test3, type))
                                    compatible_boards[type].push_back(test3);
                            }
                        } while(std::prev_permutation(positions_3.begin(), positions_3.end()));
                    }
                } while(std::prev_permutation(positions_2.begin(), positions_2.end()));
            }
}

double best_panel_recursive_part (board & real_board, int & suggested_row, int & suggested_column, std::vector<board> compatible_boards[10], std::vector<ptrdiff_t> compatible_boards_without_conflict[10], std::vector<hash_item> hash[25], int n_uncovered)
{
    if (is_won(real_board) == 1)
        return 1.0;
    
    for (std::vector<hash_item>::iterator iter = hash[n_uncovered].begin(); iter != hash[n_uncovered].end(); ++iter)
        if (are_equal((*iter).board, real_board))
        {
            //std::cout << n_uncovered << std::endl;
            suggested_row = (*iter).suggested_row;
            suggested_column = (*iter).suggested_column;
            return (*iter).win_chance;
        }
        
    int i, j;
    std::vector<ptrdiff_t>::iterator it;
    
    double P_of_b = 0;
    for (int type = 0; type < 10; type++)
        P_of_b += (double)compatible_boards_without_conflict[type].size() / N_accepted[real_board.level-1][type];
    
    double win_chance = 0, win_chance_from_panel, tmp;
    
    board next_board;
    std::vector<ptrdiff_t> next_compatible_boards_without_conflict[10];
    hash_item next_result;
    
    bool found_free_panel;
    
    for (i = 0; i < 5; i++) // checks if there is an unflipped panel guaranteed to not be 0, avoiding to compute the max
        for (j = 0; j < 5; j++)
            if (real_board.panel[i][j] == -1)
            {
                found_free_panel = 1;

                for (int type = 0; type < 10; type++)
                {
                    if (!compatible_boards_without_conflict[type].empty())
                    {
                        for (it = compatible_boards_without_conflict[type].begin(); it != compatible_boards_without_conflict[type].end(); ++it)
                            if (compatible_boards[type][*it].panel[i][j] == 0) break;
                        if (it != compatible_boards_without_conflict[type].end())
                        {
                            found_free_panel = 0;
                            break;
                        }
                    }
                }
                
                if (found_free_panel)
                {
                    suggested_row = i;
                    suggested_column = j;
                    
                    //std::cout << "Found free panel: i = " << i << ", j = " << j << std::endl;
                    
                    win_chance = 0;
                    
                    for (int value = 1; value < 4; value++)
                    {
                        tmp = 0;

                        for (int type = 0; type < 10; type++)
                            next_compatible_boards_without_conflict[type].clear();
                        
                        next_board = real_board;
                        next_board.panel[i][j] = value;
                                                                                                    
                        for (int type = 0; type < 10; type++)
                        {
                            for (it = compatible_boards_without_conflict[type].begin(); it != compatible_boards_without_conflict[type].end(); ++it)
                                if (no_conflict(compatible_boards[type][*it], next_board))
                                    next_compatible_boards_without_conflict[type].push_back(*it);
                            tmp += (double)next_compatible_boards_without_conflict[type].size() / N_accepted[real_board.level-1][type];
                        }
                        
                        if (tmp > 0)
                        {
                            next_result.board = next_board;
                            next_result.win_chance = best_panel_recursive_part(next_board, next_result.suggested_row, next_result.suggested_column, compatible_boards, next_compatible_boards_without_conflict, hash, n_uncovered + 1);
                            tmp *= next_result.win_chance / P_of_b;
                            hash[n_uncovered + 1].push_back(next_result);
                        }
                        
                        win_chance += tmp;
                    }
                    
                    return win_chance;
                }
            }
    
    //std::cout << "Didn't find a free panel." << std::endl;
        
    for (i = 0; i < 5; i++)
        for (j = 0; j < 5; j++)
            if (real_board.panel[i][j] == -1)
            {
                win_chance_from_panel = 0;
                
                for (int value = 1; value < 4; value++)
                {
                    //if (n_uncovered == 5)
                        //std::cout << "Analyzing i = " << i << ", j = " << j << ", v = " << value << std::endl;
                    
                    for (int type = 0; type < 10; type++)
                        next_compatible_boards_without_conflict[type].clear();
                    
                    next_board = real_board;
                    next_board.panel[i][j] = value;
                    
                    tmp = 0;
                                    
                    for (int type = 0; type < 10; type++)
                    {
                        for (it = compatible_boards_without_conflict[type].begin(); it != compatible_boards_without_conflict[type].end(); ++it)
                            if (no_conflict(compatible_boards[type][*it], next_board))
                                next_compatible_boards_without_conflict[type].push_back(*it);
                        tmp += (double)next_compatible_boards_without_conflict[type].size() / N_accepted[real_board.level-1][type];
                    }
                    
                    if (tmp > 0)
                    {
                        //std::cout << "Didn't find a free panel. Looking at i = " << i << ", j = " << j << ", value = " << value << std::endl;
                        next_result.board = next_board;
                        next_result.win_chance = best_panel_recursive_part(next_board, next_result.suggested_row, next_result.suggested_column, compatible_boards, next_compatible_boards_without_conflict, hash, n_uncovered + 1);
                        tmp *= next_result.win_chance / P_of_b;
                        hash[n_uncovered + 1].push_back(next_result);
                    }
                    
                    win_chance_from_panel += tmp;
                }
                                
                if (win_chance_from_panel > win_chance)
                {
                    suggested_row = i;
                    suggested_column = j;
                    win_chance = win_chance_from_panel;
                }
            }
    
    return win_chance;
}

void print_panel_probabilities (board & real_board, std::vector<board> compatible_boards[10], std::vector<ptrdiff_t> compatible_boards_without_conflict[10])
{
    board next_board;
    std::vector<ptrdiff_t> next_compatible_boards_without_conflict[10];
    std::vector<ptrdiff_t>::iterator it;
    
    double P_of_b = 0;
    for (int type = 0; type < 10; type++)
        P_of_b += (double)compatible_boards_without_conflict[type].size() / N_accepted[real_board.level-1][type];
    double P_of_b_p_v;
    
    std::cout << std::fixed;
    std::cout << std::setprecision(4);
    
    std::cout << "            \tP(0) \tP(1) \tP(2) \tP(3) \t\tP(mult) " << std::endl;
    double P_1, P_2, P_3;
    
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++)
            if (real_board.panel[i][j] == -1)
            {
                std::cout << "i = " << i << ", j = " << j;
                for (int value = 0; value < 4; value++)
                {
                    P_of_b_p_v = 0;
                    for (int type = 0; type < 10; type++)
                        next_compatible_boards_without_conflict[type].clear();
                    
                    next_board = real_board;
                    next_board.panel[i][j] = value;
                    
                    for (int type = 0; type < 10; type++)
                    {
                        for (it = compatible_boards_without_conflict[type].begin(); it != compatible_boards_without_conflict[type].end(); ++it)
                            if (no_conflict(compatible_boards[type][*it], next_board))
                                next_compatible_boards_without_conflict[type].push_back(*it);
                        P_of_b_p_v += (double) ( (double) next_compatible_boards_without_conflict[type].size() / N_accepted[real_board.level-1][type] ) / P_of_b;
                    }
                    if (value == 1) P_1 = P_of_b_p_v;
                    if (value == 2) P_2 = P_of_b_p_v;
                    if (value == 3) P_3 = P_of_b_p_v;
                    std::cout << "\t" << P_of_b_p_v;
                }
                std::cout << "\t\t" << P_2 + P_3;
                std::cout << std::endl;
            }
}

int main (int argc, const char * argv[]) {
    
    board real_board, covered_board;
        
    random_initialize_board(8,real_board);
        
    std::vector<board> possible_0_positions;
    std::vector<board> compatible_boards[10];
    std::vector<ptrdiff_t> compatible_boards_without_conflict[10];
    std::vector<hash_item> hash[25];
    int suggested_row, suggested_column;
    int chosen_row, chosen_column;
    
    print_board(real_board);
    
    cover_all_panels(real_board, covered_board);
    
    int n_uncovered = 0;
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++)
            if (covered_board.panel[i][j] > -1)
                n_uncovered++;
        
    while (is_won(covered_board) == -1)
    {
        print_board(covered_board);
        
        generate_0_positions(covered_board,possible_0_positions);
        
        generate_compatible_boards(covered_board, possible_0_positions, compatible_boards);
        //brutal_generate_compatible_boards(covered_board, possible_0_positions, compatible_boards);
        for (int type = 0; type < 10; type++)
        {
            std::cout << "Type " << type << " " << compatible_boards[type].size() << std::endl;
            for (int n = 0; n < compatible_boards[type].size(); n++)
                compatible_boards_without_conflict[type].push_back(n);
        }
        
        print_panel_probabilities(covered_board, compatible_boards, compatible_boards_without_conflict);
        
        double win_chance = best_panel_recursive_part(covered_board, suggested_row, suggested_column, compatible_boards, compatible_boards_without_conflict, hash, n_uncovered);
        
        std::cout << "i = " << suggested_row << ", j = " << suggested_column << ", p = " << win_chance << std::endl;
        
        std::cout << std::endl;
        
        std::cout << "Row to flip: ";
        std::cin >> chosen_row;
        std::cout << "Column to flip: ";
        std::cin >> chosen_column;
        std::cout << std::endl;
        
        covered_board.panel[chosen_row][chosen_column] = real_board.panel[chosen_row][chosen_column];
        
        possible_0_positions.clear();
        for (int type = 0; type < 10; type++)
        {
            compatible_boards[type].clear();
            compatible_boards_without_conflict[type].clear();
        }
    }
    
    print_board(covered_board);
    
    //test_acceptance_rate();
    //calculate_number_of_valid_boards();
    
    return 0;
}

