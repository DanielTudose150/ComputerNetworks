#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <stdint.h>

#include "connect4.h"
#include "sessions.h"

#define PORT 2728
#define DEFAULT_ADDRESS "127.0.0.1"
#define GAME_SIZE 2
#define MAX_GAMES 30

extern int errno;

typedef struct thData
{
    int idThread;
    int cl;
} thData;

typedef struct Client
{
    struct thData data;
    int client_id;
    int player;
} Client;

typedef struct Game
{
    struct Client *clients;
    int game_id;
    int client_number;
    struct Board *board;
    bool filled;
    bool started;
} Game;

static struct Game **games;
static int game_index = 0;
static int GAME_ID = 10;

static void *treat(void *);
void create_client(void *);
bool accepted_option(int option);

int create_game(struct Game *games, struct Client *client);
int join_game(struct Game **games, struct Client *client, int game_id);

void start_game(struct Game *game);

char buffer[256];
int message_length;
int client_id = 10;

int main(int argc, char *argv[])
{

    struct sockaddr_in server;
    struct sockaddr_in from;
    int nr;
    int sd;
    int pid;
    pthread_t th[100];
    int i = 0;

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[server]Eroare la socket().\n");
        return errno;
    }

    int on = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    server.sin_family = AF_INET;

    server.sin_addr.s_addr = htonl(INADDR_ANY);

    server.sin_port = htons(PORT);

    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[server]Eroare la bind().\n");
        return errno;
    }

    if (listen(sd, 2) == -1)
    {
        perror("[server]Eroare la listen().\n");
        return errno;
    }

    games = (struct Game **)malloc(sizeof(struct Game *) * MAX_GAMES);
    for (int i = 0; i < MAX_GAMES; i++)
    {
        games[i] = (struct Game *)malloc(sizeof(struct Game));
        games[i]->clients = (struct Client *)malloc(sizeof(struct Client) * GAME_SIZE);
        games[i]->board = (struct Board *)malloc(sizeof(struct Board));
        init_board(games[i]->board);
    }

    while (1)
    {
        int client;
        thData *td;
        int length = sizeof(from);

        printf("[server]Asteptam la portul %d...\n", PORT);
        fflush(stdout);

        if ((client = accept(sd, (struct sockaddr *)&from, &length)) < 0)
        {
            perror("[server]Eroare la accept().\n");
            continue;
        }

        td = (struct thData *)malloc(sizeof(struct thData));
        td->idThread = i++;
        td->cl = client;

        pthread_create(&th[i], NULL, &treat, td);
    }

    close(sd);
    return 0;
}

static void *treat(void *arg)
{
    struct thData tdL;
    tdL = *((struct thData *)arg);
    printf("[thread]- %d - Asteptam mesajul...\n", tdL.idThread);
    fflush(stdout);
    pthread_detach(pthread_self());
    create_client((struct thData *)arg);
    close((intptr_t)arg);
    return (NULL);
}

void create_client(void *arg)
{
    char buffer[256];
    char buffer2[256];
    char message[256];
    int length;
    int *lenptr = &length;

    // struct thData tdL;
    // tdL = *((struct thData *)arg);
    struct Client client;
    client.client_id = client_id++;
    client.data = *((struct thData *)arg);
    struct Client *clientptr = &client;

    int option = 0;
    int *optionptr = &option;
    bzero(buffer, sizeof(buffer));

    if (receive_string_message(client.data.cl, buffer, lenptr) == -1)
    {
        printf("[SERVER] Error receiving string message.\n");
        fflush(stdout);
        perror("receive_string_message");
    }

    strcpy(buffer2, buffer);
    while (!accepted_option(option))
    {
        strcpy(buffer, buffer2);
        bzero(message, sizeof(message));
        sprintf(message, "\nWelcome %s. Select an option:\n", buffer);
        length = strlen(message);
        message[length] = '\0';

        strcpy(buffer, "\n1. Create game\n2. Join game\n3. Exit\n");

        sprintf(message + length, "%s\n", buffer);
        length = strlen(message);
        message[length] = '\0';

        strcpy(buffer, message);

        printf("\nSending : %s\n", buffer);
        if (send_string_message(client.data.cl, buffer, lenptr) == -1)
        {
            printf("[SERVER] Error sending string message.\n");
            fflush(stdout);
            perror("send_string_message");
        }

        optionptr = &option;

        if (receive_int_message(client.data.cl, optionptr) == -1)
        {
            printf("[SERVER] Error receiving int message.\n");
            fflush(stdout);
            perror("receive_int_message");
        }

    }

    switch (option)
    {
    case 1:
    {
        int game_session_id = create_game(games[game_index++], clientptr);
        if (game_session_id == -1)
        {
            printf("FAILURE OF CREATING GAME SESSION\n");
            perror("[SERVER] Game could not be created.\n");
            fflush(stdout);
        }

        bzero(buffer, sizeof(buffer));
        sprintf(buffer, "\nYour game session id is %d\n", game_session_id);
        length = strlen(buffer);
        buffer[length] = '\0';
        if (send_string_message(client.data.cl, buffer, lenptr) == -1)
        {
            printf("[SERVER] Error sending string message.\n");
            fflush(stdout);
            perror("send_string_message");
        }
        option = game_session_id;
        break;
    }

    case 2:
    {
        bool stop = false;
        while (1)
        {

            bzero(buffer, sizeof(buffer));
            strcpy(buffer, "Insert game session id: ");
            length = strlen(buffer);
            buffer[length] = '\0';

            if (send_string_message(client.data.cl, buffer, lenptr) == -1)
            {
                printf("[SERVER][JOIN] Error sending string message.\n");
                fflush(stdout);
                perror("[SERVER][JOIN] error_send_string_message");
                break;
            }

            if (receive_int_message(client.data.cl, optionptr) == -1)
            {
                printf("[SERVER][JOIN] Error receiving int message.\n");
                fflush(stdout);
                perror("receive_int_message");
            }

            bzero(buffer, sizeof(buffer));
            if (!join_game(games, clientptr, option))
            {

                strcpy(buffer, "Game session is either full or non-existant\n");
                length = strlen(buffer);
                buffer[length] = '\0';
            }
            else
            {

                strcpy(buffer, "Successfully joined game session.\n");
                length = strlen(buffer);
                buffer[length] = '\0';

                int gindex = game_index;
                for (int i = 0; i < gindex; ++i)
                    if (games[i]->game_id == option)
                    {
                        games[i]->filled = true;
                        break;
                    }

                stop = true;
            }

            if (send_string_message(client.data.cl, buffer, lenptr) == -1)
            {
                printf("[SERVER][JOIN] Error sending string message.\n");
                fflush(stdout);
                perror("[SERVER][JOIN] error_send_string_message");
                break;
            }

            if (stop)
                break;
        }
        break;
    }

    case 3:
    {
        bzero(buffer, sizeof(buffer));
        sprintf(buffer, "\nGoodbye %s!", buffer2);
        length = strlen(buffer);
        buffer[length] = '\0';

        if (send_string_message(client.data.cl, buffer, lenptr) == -1)
        {
            printf("[SERVER][JOIN] Error sending string message.\n");
            fflush(stdout);
            perror("[SERVER][JOIN] error_send_string_message");
        }

        return;

        break;
    }
    default:
    {
        printf("[SERVER] option not supported.\n");
        fflush(stdout);
        perror("[SERVER] error_option");
        break;
    }
    }

    int gameindex = game_index;
    struct Game *iterator;
    for (int i = 0; i < gameindex; ++i)
    {
        iterator = games[i];
        if (iterator->game_id == option)
            if (iterator->filled)
            {
                iterator->started = true;
                start_game(iterator);
                break;
            }
    }
}

bool accepted_option(int option)
{
    switch (option)
    {
    case 1:
    case 2:
    case 3:
        return true;
        break;
    default:
        break;
    }
    return false;
}

int create_game(struct Game *games, struct Client *client)
{

    games->game_id = GAME_ID;
    games->client_number = 1;

    GAME_ID++;

    games->clients[0] = *client;

    games->clients[0].player = 1;

    games->filled = false;
    games->started = false;

    return games->game_id;
}

int join_game(struct Game **games, struct Client *client, int game_id)
{
    int gindex = game_index;
    struct Game *iterator;
    for (int i = 0; i < gindex; ++i)
    {
        if (games[i] == NULL)
            break;
        iterator = games[i];
        if (iterator->game_id == game_id)
        {
            if (iterator->client_number < GAME_SIZE)
            {
                iterator->clients[1] = *client;
                iterator->clients[1].player = 2;
                iterator->client_number++;
                return 1;
            }
        }
    }
    return 0;
}

void start_game(struct Game *game)
{
    char buffer[256];
    int length = 0;
    int *lenptr = &length;

    // sending player seeding
    for (int i = 0; i < game->client_number; ++i)
    {
        bzero(buffer, sizeof(buffer));
        sprintf(buffer, "\nYou are player %d! Player 1 always goes first.\n", game->clients[i].player);
        length = strlen(buffer);
        buffer[length] = '\0';

        if (send_string_message(game->clients[i].data.cl, buffer, lenptr) == -1)
        {
            printf("[SERVER][GAME] Error send_string_message.\n");
            fflush(stdout);
            perror("[SERVER][GAME] error_send_string_message");
            close(game->clients[i].data.cl);
        }
    }

    printf("Game should start soon : %d\n\n", game->game_id);

    int player_turn = game->clients[0].player;
    int winner = 0;
    unsigned int turns = 0;
    int valid_move = 0;
    int flag = -10;
    int *flagptr = &flag;
    while (winner == 0)
    {
        ++turns;
        bzero(buffer, sizeof(buffer));
        sprintf(buffer, "\nPlayer %d adds a piece on turn %d\n", player_turn, turns);
        length = strlen(buffer);
        lenptr = &length;
        buffer[length] = '\0';
        valid_move=0;

        // need to send the current board

        for(int i =0;i<game->client_number; ++i)
        {
            if(send_board(game->clients[i].data.cl,game->board->grid) == -1)
            {
                printf("[SERVER][GAME] Error sending board.\n");
                fflush(stdout);
                perror("error_send_board");
                return;
            }
        }

        //sending the message as to which player to insert piece
        for (int i = 0; i < game->client_number; ++i)
        {
            if (send_string_message(game->clients[i].data.cl, buffer, lenptr) == -1)
            {
                printf("[SERVER][GAME] Error sending string message.\n");
                fflush(stdout);
                perror("error_send_string_message");
                return;
            }
        }

        while (valid_move == 0)
        {
            //creating message asking for column from player
            flag = -10;
            bzero(buffer, sizeof(buffer));
            sprintf(buffer, "\nInsert a column in which a piece will be added: ");
            length = strlen(buffer);
            buffer[length] = '\0';

            // sending the other a player a flag to not read things and jump
            // and sending a flag to the main player to confirm asking for column
            
            if (send_int_message(game->clients[2 - player_turn].data.cl, flagptr) == -1)
            {
                printf("[SERVER][GAME] Error sending int message.\n");
                fflush(stdout);
                perror("error_send_int_message");
                return;
            }

            // to main player
            flag *= (-1);
            if (send_int_message(game->clients[player_turn - 1].data.cl, flagptr) == -1)
            {
                printf("[SERVER][GAME] Error sending int message.\n");
                fflush(stdout);
                perror("error_send_int_message");
                return;
            }

            //sending request message of column
            if(send_string_message(game->clients[player_turn-1].data.cl, buffer, lenptr) == -1)
            {
                printf("[SERVER][GAME] Error sending string message.\n");
                fflush(stdout);
                perror("error_send_string_message");
                return;
            }

            // receiving column number from client
            int column = 0;
            int* columnptr = &column;
            if(receive_int_message(game->clients[player_turn-1].data.cl,columnptr) == -1)
            {
                printf("[SERVER][GAME] Error receive_int_message.\n");
                fflush(stdout);
                perror("error_receive_int_message");
                return;
            }
            column = column - 1;

            int piece = 0;
            int* pieceptr = &piece;
            piece = add_piece(game->board,player_turn,column);
            

            // checking if piece was added or not and sending a message;
            bzero(buffer,sizeof(buffer));

            if(piece == -1)
            {
                sprintf(buffer,"\nColumn %d is either filled or does not exist.\n",column+1);
                length = strlen(buffer);
                buffer[length] = '\0';
            }
            else
            {
                sprintf(buffer,"\nColumn %d is a valid position.\n",column+1);
                length = strlen(buffer);
                buffer[length] = '\0';
            }

            if(send_string_message(game->clients[player_turn-1].data.cl,buffer, lenptr) == -1)
            {
                printf("[SERVER][GAME] Error sending string message.\n");
                fflush(stdout);
                perror("error_send_string_message");
                return;
            }

            // going to the beginning of the loop looking for valid column
            if(piece == -1)
                continue;            

            // if not continue => need to check if there is a winner or draw
            valid_move = 1;
            piece = decide_game(game->board);

            bzero(buffer, sizeof(buffer));
            if(piece == -1)
            {
                // filled;
                sprintf(buffer,"\nThis match is a draw. Board is filled.\n");
                length = strlen(buffer);
                buffer[length] = '\0';
                winner=1;
            }
            else if(piece != 0)
            {
                //winner;
                sprintf(buffer,"\nPlayer %d wins the game.\n",player_turn);
                length = strlen(buffer);
                buffer[length] = '\0';
                winner=1;
            }
            else
            {
                // undecided;
                sprintf(buffer,"\nMatch is still undecided.\n");
                length = strlen(buffer);
                buffer[length] = '\0';
            }

            //sending message to clients
            for (int i = 0; i < game->client_number; ++i)
            {
                if (send_string_message(game->clients[i].data.cl, buffer, lenptr) == -1)
                {
                    printf("[SERVER][GAME] Error sending string message.\n");
                    fflush(stdout);
                    perror("error_send_string_message");
                    return;
                }
            }

        }

        player_turn = 3 - player_turn;    
    }

    if(winner)
        return;
}