#ifndef CONNECT4_H_GUARD
#define CONNECT4_H_GUARD

#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

#define MAX_ROW 6
#define MAX_COL 7

typedef struct Board{
    int grid[MAX_ROW][MAX_COL];
    int depth[MAX_COL];
}Board;

void init_board(struct Board* board);
void print_board(struct Board* board);
int add_piece(struct Board* board,int player, int column);

bool check_depth(struct Board* board,int column);
bool check_inside(struct Board* board, int row, int column);
bool check_player(struct Board* board, int player, int row, int column);
bool check_filled(struct Board* board);

int count_left_pieces(struct Board* board, int player, int row, int column);
int count_right_pieces(struct Board* board, int player, int row, int column);
int count_upper_pieces(struct Board* board, int player, int row, int column);
int count_below_pieces(struct Board* board, int player, int row, int column);
int count_diagonal_up_left(struct Board* board, int player, int row, int column);
int count_diagonal_up_right(struct Board* board, int player, int row, int column);
int count_diagonal_down_left(struct Board* board, int player, int row, int column);
int count_diagonal_down_right(struct Board* board, int player, int row, int column);

int decide_game(struct Board* board);
int decide_winner(struct Board* board, int player);

#endif