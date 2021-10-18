#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>

#include "sessions.h"

#define DEFAULT_ADDRESS "127.0.0.1"
#define DEFAULT_PORT 2728

int grid[6][7];

extern int errno;

int port;

void print_board();

int main(int argc, char *argv[])
{
    int sd;
    struct sockaddr_in server;
    char message[256];

    port = DEFAULT_PORT;

    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 7; ++j)
            grid[i][j] = 0;

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[CLIENT] Error on creating socket.\n");
        return errno;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(DEFAULT_ADDRESS);
    server.sin_port = htons(port);

    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[CLIENT] Error on connecting to server.\n");
        return errno;
    }

    bzero(message, 256);
    printf("[CLIENT] Enter name : ");
    fflush(stdout);
    //read(0, message, 128);
    readLine(message, MAX_READ_SIZE);
    int length = strlen(message);
    // message[length] = '\0';
    int *lenptr = &length;

    if (send_string_message(sd, message, lenptr) == -1)
    {
        printf("[CLIENT] Error sending name.\n");
        return -1;
    }

    int option;
    int *optionptr;
    while (1)
    {
        if (receive_string_message(sd, message, lenptr) == -1)
        {
            printf("[CLIENT] Error receive_string_message.\n");
            return -1;
        }
        printf("\n%s\n", message);
        printf("Insert option: ");
        scanf("%d", &option);

        optionptr = &option;
        if (send_int_message(sd, optionptr) == -1)
        {
            printf("[CLIENT] Error send_int_message.\n");
            perror("error_send_int_message");
            return -1;
        }

        switch (option)
        {
        case 1:
        {
            goto game;
            break;
        }
        case 2:
            goto join_game;
            break;
        case 3:
        {
            goto _goodbye_message;
            break;
        }
        default:
            break;
        }
    }

join_game:
{
    while (1)
    {
        bzero(message, sizeof(message));
        if (receive_string_message(sd, message, lenptr) == -1)
        {
            printf("[CLIENT] Error send_string_message.\n");
            perror("[JOIN]error_send_int_message");
            return -1;
        }

        printf("%s", message);
        scanf("%d", &option);

        if (send_int_message(sd, optionptr) == -1)
        {
            printf("[CLIENT] Error send_int_message.\n");
            perror("error_send_int_message");
            return -1;
        }

        bzero(message, sizeof(message));
        if (receive_string_message(sd, message, lenptr) == -1)
        {
            printf("[CLIENT] Error send_string_message.\n");
            perror("[JOIN]error_send_int_message");
            return -1;
        }

        printf("%s", message);

        char *check;
        check = strstr(message, "joined");
        if (check != NULL)
            break;
    }

    goto continuation;
}

game:
{
    bzero(message, sizeof(message));
    if (receive_string_message(sd, message, lenptr))
    {
        printf("[CLIENT] Error send_string_message.\n");
        perror("[JOIN]error_send_int_message");
        return -1;
    }
    length = strlen(message);
    message[length] = '\0';
    printf("%s", message);
    goto continuation;
}

continuation:
{
    bzero(message, sizeof(message));
    if (receive_string_message(sd, message, lenptr) == -1)
    {
        printf("[CLIENT] Error receive_string_message.\n");
        perror("[START] error_receive_string_message");
        return -1;
    }
    // receiving player seeding;
    length = strlen(message);
    message[length] = '\0';
    printf("%s\n", message);
}

end:
{   
    // receiving the board;
    if(receive_board(sd,grid) == -1)
    {
        printf("[CLIENT][GAME] Error receive_board.\n");
        perror("error_receive_board");
        return -1;
    }

    print_board();

    // message as to which player to insert piece
    bzero(message, sizeof(message));
    if (receive_string_message(sd, message, lenptr) == -1)
    {
        printf("[CLIENT] Error send_string_message.\n");
        perror("[JOIN]error_send_int_message");
        return -1;
    }

    length = strlen(message);
    message[length] = '\0';
    printf("%s\n", message);

    int condition = 0;
    while (condition == 0)
    {
        bzero(message, sizeof(message));
        length = 0;
        lenptr = &length;

        //receiving flag
        int flag = 0;
        int *flagptr = &flag;
        if (receive_int_message(sd, flagptr) == -1)
        {
            printf("[CLIENT][GAME] Error receive_int_message.\n");
            fflush(stdout);
            perror("error_receive_int_message");
            return -1;
        }

        if (flag == -10)
            goto flagged;

        // needs to insert column

        if (receive_string_message(sd, message, lenptr) == -1)
        {
            printf("[CLIENT][GAME] Error receive_string_message.\n");
            fflush(stdout);
            perror("error_receive_string_message");
            return -1;
        }

        printf("%s\n", message);

        // getting column
        int column;
        do
        {
            scanf("%d", &column);
        } while (column <= 0 || column > 7);

        // need to send column to server
        int *columnptr = &column;
        if (send_int_message(sd, columnptr) == -1)
        {
            printf("[CLIENT][GAME] Error send_int_message.\n");
            perror("error_send_int_message");
            return -1;
        }

        // receiving confirmation of position
        bzero(message, sizeof(message));
        if (receive_string_message(sd, message, lenptr) == -1)
        {
            printf("[CLIENT][GAME] Error receive_string_message.\n");
            fflush(stdout);
            perror("error_receive_string_message");
            return -1;
        }

        printf("%s\n", message);
        char *check2 = strstr(message, "fill");
        if (check2 != NULL)
            goto _repeat;

        // if valid column; awaiting message from server
        bzero(message, sizeof(message));
        if (receive_string_message(sd, message, lenptr) == -1)
        {
            printf("[CLIENT][GAME] Error receive_string_message.\n");
            fflush(stdout);
            perror("error_receive_string_message");
            return -1;
        }

        printf("%s\n", message);
        check2 = strstr(message, "fill");
        if (check2 != NULL)
            goto _exit;
        check2 = strstr(message, "win");
        if (check2 != NULL)
            goto _exit;
        goto _repeat;

    flagged:
    {
        // receiving status of game;
        bzero(message, sizeof(message));
        if (receive_string_message(sd, message, lenptr) == -1)
        {
            printf("[CLIENT][GAME] Error receive_string_message.\n");
            fflush(stdout);
            perror("error_receive_string_message");
            return -1;
        }
        printf("%s\n", message);
        check2 = strstr(message, "fill");
        if (check2 != NULL)
            goto _exit;
        check2 = strstr(message, "win");
        if (check2 != NULL)
            goto _exit;
        goto _repeat;
    }

    _repeat:
    {
        goto end;
    }
    }
}

_goodbye_message:
{
    bzero(message, sizeof(message));
    if(receive_string_message(sd, message, lenptr)  == -1)
    {
        printf("[CLIENT] Error receive_string_message.\n");
        perror("error_receive_string_message.\n");
        fflush(stdout);
        return -1;
    }

    printf("\n%s\n", message);
}

_exit:
    close(sd);
    return 0;
}

void print_board()
{
    printf("\n");
    printf("+");
    for(int i=1;i<30;++i) printf("=");
    printf("+");
    printf("\n");
    for(int i=5;i>=0;--i)
    {
        printf(" |");
        for(int j=0;j<7;++j)
            printf(" %d |",grid[i][j]);
        printf("\n ");

        for(int i=0;i<29;++i) printf("-");
        printf("\n");
    }
    printf("+");
    for(int i=1;i<30;++i) printf("=");
    printf("+");
    printf("\n\n");
}