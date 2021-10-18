#include "connect4.h"

struct Board board;

int main(int argc, char *argv[])
{
    struct Board *boardptr = &board;
    init_board(boardptr);
    print_board(boardptr);
    int player = 2;
    int move;
    while (!decide_game(boardptr))
    {
        player = 3 - player;
    column:
    {
        if (player == 1)
        {
            printf("1st player move : insert column : ");
        }
        else
        {
            printf("2st player move : insert column : ");
        }

        scanf("%d", &move);
        if(add_piece(boardptr, player, move) == -1)
            goto column;
        else
        {
            goto print;
        }
        
    }

    print:
        print_board(boardptr);
    }
    printf("Player % d is the winner\n", player);
}