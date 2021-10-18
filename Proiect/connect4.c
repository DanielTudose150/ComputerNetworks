#include "connect4.h"


void init_board(struct Board* board)
{
    for(int i=0;i<MAX_ROW;++i)
    {
        for(int j=0;j<MAX_COL;++j)
            (*board).grid[i][j] = 0;
        (*board).depth[i]=0;
    }
}

void print_board(struct Board* board)
{
    printf("\n");
    for(int i=MAX_ROW-1;i>=0;--i)
    {
        for(int j=0;j<MAX_COL;++j)
            printf("%d ",(*board).grid[i][j]);
        printf("\n");
    }
}

int add_piece(struct Board* board,int player, int column)
{
    if(column < 0 || column >= MAX_COL)
    {
        printf("Column %d does not exist.\n",column);
        return -1;
    }
    
    if(!check_depth(board,column))
    {
        printf("Column %d is filled.\n",column);
        return -1;
    }

    (*board).grid[(*board).depth[column]++][column] = player;
    return 1;
}

bool check_depth(struct Board* board, int column)
{
    return (*board).depth[column] < 6 ? true : false;
}

bool check_inside(struct Board* board, int row, int column)
{
    if(row < 0 || row >= MAX_ROW)
        return false;
    if(column < 0 || column >= MAX_COL)
        return false;
    return true;
}

bool check_player(struct Board* board, int player, int row, int column)
{
    return (*board).grid[row][column] == player;
}

bool check_filled(struct Board* board)
{
    for(int i = 0;i<MAX_COL;++i)
        if((*board).depth[i] < MAX_ROW- 1)
            return false;
    return true;
}

int count_left_pieces(struct Board* board, int player, int row, int column)
{

    if(!check_inside(board,row,column))
    {
        printf("Position out of grid.\n");
        return -1;
    }

    if(!check_player(board, player, row, column))
    {
        printf("Position [%d,%d] is not of player %d\n",row,column,player);
        return 0;
    }

    int count = 0;
    int index = column - 1;
    while(check_inside(board,row,index))
    {
        if((*board).grid[row][index] == player)
            ++count;
        else
            break;
        --index;
    }

    return count;   
}

int count_right_pieces(struct Board* board, int player, int row, int column)
{
    if(!check_inside(board,row,column))
    {
        printf("Position out of grid.\n");
        return -1;
    }

    if(!check_player(board, player, row, column))
    {
        printf("Position [%d,%d] is not of player %d",row,column,player);
        return 0;
    }

    int count = 0;
    int index = column + 1;
    while(check_inside(board,row,index))
    {
        if((*board).grid[row][index] == player)
            ++count;
        else
            break;
        ++index;
    }

    return count; 
}

int count_upper_pieces(struct Board* board, int player, int row, int column)
{
    if(!check_inside(board,row,column))
    {
        printf("Position out of grid.\n");
        return -1;
    }

    if(!check_player(board, player, row, column))
    {
        printf("Position [%d,%d] is not of player %d",row,column,player);
        return 0;
    }

    int count = 0;
    int index = row + 1;
    while(check_inside(board,row,index) && (*board).depth[column] > index)
    {
        if((*board).grid[index][column] == player)
            ++count;
        else
            break;
        ++index;
    }

    return count;
}

int count_below_pieces(struct Board* board, int player, int row, int column)
{
    if(!check_inside(board,row,column))
    {
        printf("Position out of grid.\n");
        return -1;
    }

    if(!check_player(board, player, row, column))
    {
        printf("Position [%d,%d] is not of player %d",row,column,player);
        return 0;
    }

    int count = 0;
    int index = row - 1;
    while(check_inside(board,row,index))
    {
        if((*board).grid[index][column] == player)
            ++count;
        else
            break;
        --index;
    }

    return count;
}

int count_diagonal_up_left(struct Board* board, int player, int row, int column)
{
    if(!check_inside(board,row,column))
    {
        printf("Position out of grid.\n");
        return -1;
    }

    if(!check_player(board, player, row, column))
    {
        printf("Position [%d,%d] is not of player %d",row,column,player);
        return 0;
    }

    int count = 0;
    int rindex = row - 1;
    int cindex = column - 1;
    while(check_inside(board,rindex,cindex) && (*board).depth[cindex] > rindex)
    {
        if((*board).grid[rindex][cindex] == player)
            ++count;
        else
            break;
        --rindex;
        --cindex;
    }

    return count;
}

int count_diagonal_up_right(struct Board* board, int player, int row, int column)
{
    if(!check_inside(board,row,column))
    {
        printf("Position out of grid.\n");
        return -1;
    }

    if(!check_player(board, player, row, column))
    {
        printf("Position [%d,%d] is not of player %d",row,column,player);
        return 0;
    }

    int count = 0;
    int rindex = row - 1;
    int cindex = column + 1;

    while(check_inside(board,rindex,cindex) && (*board).depth[cindex] > rindex)
    {
        if((*board).grid[rindex][cindex] == player)
            ++count;
        else
            break;
        --rindex;
        ++cindex;
    }

    return count;
}

int count_diagonal_down_left(struct Board* board, int player, int row, int column)
{
    if(!check_inside(board,row,column))
    {
        printf("Position out of grid.\n");
        return -1;
    }

    if(!check_player(board, player, row, column))
    {
        printf("Position [%d,%d] is not of player %d",row,column,player);
        return 0;
    }

    int count = 0;
    int rindex = row + 1;
    int cindex = column - 1;

    while(check_inside(board,rindex,cindex))
    {
        if((*board).grid[rindex][cindex] == player)
            ++count;
        else
            break;
        ++rindex;
        --cindex;
    }

    return count;
}

int count_diagonal_down_right(struct Board* board, int player, int row, int column)
{
    if(!check_inside(board,row,column))
    {
        printf("Position out of grid.\n");
        return -1;
    }

    if(!check_player(board, player, row, column))
    {
        printf("Position [%d,%d] is not of player %d",row,column,player);
        return 0;
    }

    int count = 0;
    int rindex = row + 1;
    int cindex = column + 1;

    while(check_inside(board,rindex,cindex))
    {
        if((*board).grid[rindex][cindex] == player)
            ++count;
        else
            break;
        ++rindex;
        ++cindex;
    }

    return count;
}

int decide_game(struct Board* board)
{
    if(check_filled(board))
        return -1;

    for(int player = 1; player<3;++player)
        if(decide_winner(board,player))
            return player;
    return 0;
}

int decide_winner(struct Board* board, int player)
{
    int row_count = 0, column_count = 0, main_diag = 0, second_diag = 0;
    for(int row = 0; row < MAX_ROW;++row)
    {
        for(int column = 0; column < MAX_COL; ++column)
        {
            if((*board).grid[row][column] == player)
            {
                row_count = column_count = main_diag = second_diag = 1;
                int left_result, right_result;
                left_result = count_left_pieces(board,player,row,column);
                if(left_result == -1)
                    return 0;
                right_result = count_right_pieces(board,player, row, column);
                if(right_result == -1)
                    return 0;

                row_count =  row_count + left_result + right_result;
                if(row_count == 4)
                    return 1;

                left_result = count_upper_pieces(board,player,row,column);
                if(left_result == -1)
                    return 0;
                right_result = count_below_pieces(board,player, row, column);
                if(right_result == -1)
                    return 0;
                column_count =  column_count + left_result + right_result;
                if(column_count == 4)
                    return 1;

                left_result = count_diagonal_down_left(board,player,row,column);
                if(left_result == -1)
                    return 0;
                right_result = count_diagonal_up_right(board,player,row,column);
                if(right_result == -1)
                    return 0;
                main_diag = main_diag + left_result + right_result;
                if(main_diag == 4)
                    return 1;

                left_result = count_diagonal_down_right(board,player,row,column);
                if(left_result == -1)
                    return 0;
                right_result = count_diagonal_up_left(board,player, row, column);
                if(right_result == -1)
                    return 0;
                second_diag =  second_diag + left_result + right_result;
                if(second_diag == 4)
                return 1;
            }
        }
    }
    return 0;
}