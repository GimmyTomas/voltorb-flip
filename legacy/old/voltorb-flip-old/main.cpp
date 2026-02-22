//
//  main.cpp
//  voltorb-flip
//
//  Created by Giovanni Maria Tomaselli on 15/06/21.
//

#include <cstdlib>
#include <iostream>
#include <ctime>
#include <list>
#include <fstream>
#include <algorithm>
#include <vector>

struct parameters
{
	int8_t n_bombs;
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

struct board
{
	int8_t level;
	int8_t panel[5][5]; // -1 = unflipped, 0 = voltorb
	int8_t row_sum[5];
	int8_t row_bombs[5];
	int8_t column_sum[5];
	int8_t column_bombs[5];
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
		std::cout << "| " << unsigned(board.row_sum[i]) << "," << unsigned(board.row_bombs[i]) << std::endl;
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
	for (int j = 0; j < 5; j++) std::cout << "  " << unsigned(board.column_bombs[j]) << " ";
	std::cout << std::endl;
	std::cout << std::endl;
}

bool check_legality_board (parameters & parameters, board & board)
{
	int total_free_multipliers = 0;
	int row_free_multipliers = 0;
		
	for (int i = 0; i < 5; i++)
		if (board.row_bombs[i] == 0)
			for (int j = 0; j < 5; j++)
				if (board.panel[i][j] > 1)
					row_free_multipliers++;
	
	if (row_free_multipliers >= parameters.max_row_free) return false;
	
	row_free_multipliers = 0;
	
	for (int j = 0; j < 5; j++)
		if (board.column_bombs[j] == 0)
			for (int i = 0; i < 5; i++)
				if (board.panel[i][j] > 1)
				{
					row_free_multipliers++;
					total_free_multipliers++;
				}
	
	if (row_free_multipliers >= parameters.max_row_free || total_free_multipliers >= parameters.max_total_free) return false;
	
	return true;
}

void random_initialize_board (int8_t level, board & board)
{
	int i,j;
	parameters parameters = board_type[level-1][rand()%10];
	
	board.level = level;
	
	for (i = 0; i < 5; i++)
		for (j = 0; j < 5; j++)
			board.panel[i][j] = 1;
	
	while (parameters.n_bombs > 0)
	{
		i = rand()%5;
		j = rand()%5;
		if (board.panel[i][j] == 1)
		{
			board.panel[i][j] = 0;
			parameters.n_bombs--;
		}
	}
	
	while (parameters.n_2 > 0)
	{
		i = rand()%5;
		j = rand()%5;
		if (board.panel[i][j] == 1)
		{
			board.panel[i][j] = 2;
			parameters.n_2--;
		}
	}
	
	while (parameters.n_3 > 0)
	{
		i = rand()%5;
		j = rand()%5;
		if (board.panel[i][j] == 1)
		{
			board.panel[i][j] = 3;
			parameters.n_3--;
		}
	}
	
	for (i = 0; i < 5; i++)
	{
		board.row_sum[i] = board.panel[i][0] + board.panel[i][1] + board.panel[i][2] + board.panel[i][3] + board.panel[i][4];
		board.row_bombs[i] = (board.panel[i][0] == 0) + (board.panel[i][1] == 0) + (board.panel[i][2] == 0) + (board.panel[i][3] == 0) + (board.panel[i][4] == 0);
	}
	
	for (j = 0; j < 5; j++)
	{
		board.column_sum[j] = board.panel[0][j] + board.panel[1][j] + board.panel[2][j] + board.panel[3][j] + board.panel[4][j];
		board.column_bombs[j] = (board.panel[0][j] == 0) + (board.panel[1][j] == 0) + (board.panel[2][j] == 0) + (board.panel[3][j] == 0) + (board.panel[4][j] == 0);
	}
	
	if (!check_legality_board(parameters,board)) random_initialize_board(level,board);
}

int is_winning (board board)
// -1 if board is not finished, 0 if it is lost, 1 if it is won
{
	int total_flipped = 0;
	
	for (int i = 0; i < 5; i++)
		for (int j = 0; j < 5; j++)
		{
			if (board.panel[i][j] == 0) return 0;
			else total_flipped += abs(board.panel[i][j]);
		}
	
	int total = board.row_sum[0] + board.row_sum[1] + board.row_sum[2] + board.row_sum[3] + board.row_sum[4] + board.row_bombs[0] + board.row_bombs[1] + board.row_bombs[2] + board.row_bombs[3] + board.row_bombs[4];
	
	if (total == total_flipped) return 1;
	else return -1;
}

void generate_compatible_boards (board & real_board, std::vector<board> & compatible_boards, bool possible_panel_values[][5][4])
// fills compatible_boards
{
	int i,j;
	int total_bombs = real_board.row_bombs[0] + real_board.row_bombs[1] + real_board.row_bombs[2] + real_board.row_bombs[3] + real_board.row_bombs[4];
	int total_sums = real_board.row_sum[0] + real_board.row_sum[1] + real_board.row_sum[2] + real_board.row_sum[3] + real_board.row_sum[4];
	int min_number_of[4] = {0,0,0,0};
	int max_number_of[4] = {0,0,0,0};
	// min/max_number_of[i] contain the min/max number of occurrences of i in the board (we don't actually need the info for bombs); let's fill them:
	
	for (i = 0; i < 5; i++)
		for (j = 0; j < 5; j++)
		{
			if (real_board.panel[i][j] > 0) // could also be > -1
				min_number_of[real_board.panel[i][j]] += 1;
			for (int value = 1; value < 4; value ++)
				if (possible_panel_values[i][j][value])
					max_number_of[value] += 1;
		}
	
	bool bombs_check, total_sums_check, number_of_1_check, number_of_2_check, number_of_3_check;
	int to_be_placed[4];
	for (int k = 0; k < 10; k++)
	{
		bombs_check = (total_bombs == board_type[real_board.level-1][k].n_bombs);
		total_sums_check = (total_sums == (25 - board_type[real_board.level-1][k].n_bombs + 1*board_type[real_board.level-1][k].n_2 + 2*board_type[real_board.level-1][k].n_3));
		number_of_1_check = (min_number_of[1] <= 25 - board_type[real_board.level-1][k].n_bombs - board_type[real_board.level-1][k].n_2 - board_type[real_board.level-1][k].n_3) && (max_number_of[1] >= 25 - board_type[real_board.level-1][k].n_bombs - board_type[real_board.level-1][k].n_2 - board_type[real_board.level-1][k].n_3);
		number_of_2_check = (min_number_of[2] <= board_type[real_board.level-1][k].n_2) && (max_number_of[2] >= board_type[real_board.level-1][k].n_2);
		number_of_3_check = (min_number_of[3] <= board_type[real_board.level-1][k].n_3) && (max_number_of[3] >= board_type[real_board.level-1][k].n_3);
		if (bombs_check && total_sums_check && number_of_1_check && number_of_2_check && number_of_3_check) // checks that the board type k is compatible with the known info
		{
			to_be_placed[0] = board_type[real_board.level-1][k].n_bombs;
			to_be_placed[1] = 25 - board_type[real_board.level-1][k].n_bombs - board_type[real_board.level-1][k].n_2 - board_type[real_board.level-1][k].n_3;
			to_be_placed[2] = board_type[real_board.level-1][k].n_2;
			to_be_placed[3] = board_type[real_board.level-1][k].n_3;
			
			for (i = 0; i < 5; i++)
				for (j = 0; j < 5; j++)
					if (real_board.panel[i][j] != -1)
						to_be_placed[real_board.panel[i][j]] -= 1;
		}
	}
}

double best_panel (board & real_board, int & suggested_row, int & suggested_column, std::vector<board> & compatible_boards)
// returns -1 if there are panels to flip that are guaranteed not to be voltorb
// otherwise, returns the probability to win the board
{
	int i,j;
	
	if (compatible_boards.empty())
	{
		// check for unflipped free panels
		for (i = 0; i < 5; i++)
			for (j = 0; j < 5; j++)
				if (real_board.panel[i][j] == -1 && (real_board.row_sum[i] == 0 || real_board.column_sum[j] == 0) )
				{
					suggested_row = i;
					suggested_column = j;
					return -1;
				}
		
		// do something about panels that are safe, or are certainly voltorb: choose them and return -1...
		
		bool possible_panel_values[5][5][4];
		generate_compatible_boards(real_board,compatible_boards,possible_panel_values);
	}
	
	board next_board; //the board with 1 more flipped panel
	std::vector<board> next_compatible_boards;
	int next_row, next_column;
	std::vector<board>::iterator it;
	
	double score, max_score = 0;
	long int number_next_boards;
		
	for (i = 0; i < 5; i++)
		for (j = 0; j < 5; j++)
			if (real_board.panel[i][j] == -1)
			{
				score = 0;
				for (int8_t panel_value = 0; panel_value < 4; panel_value++)
				{
					it = std::copy_if(compatible_boards.begin(), compatible_boards.end(), next_compatible_boards.begin(), [&i,&j,&panel_value](board & board){return board.panel[i][j] == panel_value;});
					next_compatible_boards.resize(std::distance(next_compatible_boards.begin(),it));
					number_next_boards = next_compatible_boards.size();
					if (number_next_boards != 0)
					{
						next_board = real_board;
						next_board.panel[i][j] = panel_value;
						if (is_winning(next_board) != -1) score += number_next_boards * is_winning(next_board);
						else score += number_next_boards * best_panel(next_board, next_row, next_column, next_compatible_boards);
					}
				}
				score /= compatible_boards.size();
				if (score > max_score)
				{
					max_score = score;
					suggested_row = i;
					suggested_column = j;
				}
			}
	
	return max_score;
}

int main (int argc, const char * argv[]) {
	
	srand(static_cast<unsigned int>(time(nullptr)));
	
	board real_board;
	
	random_initialize_board(1,real_board);
	
	std::vector<board> compatible_boards;
	int suggested_row, suggested_column;
	
	print_board(real_board);
	
	best_panel(real_board, suggested_row, suggested_column, compatible_boards);
	
	return 0;
}
