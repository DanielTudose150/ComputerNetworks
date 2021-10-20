#include "communication.h"

int readLine(char* line, int size)
{
    if(fgets(line,size,stdin) == NULL) 
    {
        printf("Error reading from standard input\n");
        fflush(stdin);
        return -1;
    }

    line[strlen(line)-1] = 0;
}

int receiveStringMessage(int fd, char* message, int* length)
{
    char* buffer = (char*)malloc(sizeof(char) * MAX_READ_SIZE);
    int bytes;

    int len;

    if((bytes = read(fd,&len, sizeof(len)) == -1))
    {
        perror("Error receiving length of string.\n");
        return -1;
    }

    bzero(buffer,sizeof(buffer));
    if(read(fd,buffer,len) == -1)
    {
        perror("Error reading from standard input.\n");
        return -1;
    }

    *length = len;
    buffer[len] = 0;
    strcpy(message,buffer);
    free(buffer);
    return 0;
}

int receiveIntMessage(int fd, int* message)
{
    int response;
    if ((response = read(fd, (message), sizeof(int))) == -1)
    {
        perror("Error reading int message.\n");
        return -1;
    }

    return 0;
}

int sendStringMessage(int fd, char* message, int* length)
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

int sendIntMessage(int fd, int* message)
{
    if(write(fd,message,sizeof(int)) == -1)
    {
        perror("Error sending int message.\n");
        return -1;
    }

    return 0;
}