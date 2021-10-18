#include "sessions.h"

int readLine(char *line, int size)
{
    if (fgets(line, size, stdin) == NULL)
    {
        printf("Error reading from stdin.\n");
        fflush(stdin);
        return -1;
    }

    line[strlen(line) - 1] = 0;
}

int receive_string_message(int fd, char *message, int *length)
{
    //char buffer[256];
    char *buffer = (char *)malloc(sizeof(char) * 256);
    int bytes;

    int len;

    if ((bytes = read(fd, &len, sizeof(len))) == -1)
    {
        perror("Error receiving length of string.\n");
        return -1;
    }

    bzero(buffer, sizeof(buffer));
    if (read(fd, buffer, len) == -1)
    {
        perror("Error reading string.\n");
        return -1;
    }

    *length = len;
    buffer[len] = '\0';
    strcpy(message, buffer);
    message[len] = '\0';
    free(buffer);
    return 0;
}

int receive_int_message(int fd, int *message)
{

    int response;
    if ((response = read(fd, (message), sizeof(int))) == -1)
    {
        perror("Error reading int message.\n");
        return -1;
    }

    return 0;
}

int send_string_message(int fd, char *message, int *length)
{
    int len = *length;
    if (write(fd, &len, sizeof(len)) == -1)
    {
        perror("Error sending size of string.\n");
        return -1;
    }

    if (write(fd, message, len) == -1)
    {
        perror("Error sending string.\n");
        return -1;
    }

    return 0;
}

int send_int_message(int fd, int *message)
{
    //int response = *message;
    //printf("param : %d\n",response);
    if (write(fd, message, sizeof(int)) == -1)
    {
        perror("Error sending int message.\n");
        return -1;
    }

    //printf("param : %d\n",response);
    return 0;
}

int send_board(int fd, int matrix[][7])
{
    int *element;
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 7; ++j)
        {
            element = &matrix[i][j];
            if (send_int_message(fd, element) == -1)
            {
                perror("Error: send_board : send_int_message");
                return -1;
            }
        }
}

int receive_board(int fd, int matrix[][7])
{
    int *element;
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 7; ++j)
        {
            element = &matrix[i][j];
            if (receive_int_message(fd, element) == -1)
            {
                perror("Error : receive_board : receive_int_message");
                return -1;
            }
        }
}