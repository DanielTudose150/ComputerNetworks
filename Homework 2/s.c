#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <ctype.h>

#define PORT 2908

extern int errno;

typedef struct thData
{
    int idThread; //thread id to be tinut in evidenta;
    int cl;       //descriptorul intors de accept;
} thData;

//functions

struct database
{
    char username[100];
    int logged;
} users[100];
int userSize = 0;

static void *threadHandler(void *arg);
void nospaces(char buff[], int size);
int exists(char *buff);
void messageHandler(void *arg);
int commandHandler(char buff[], void *arg);
void toLower(char buff[]);
int checkCommand(char *buff);
void loginHandler(char buff[],void* arg);

int indx = 0;

// the code has some memory problems I couldn't fix before 
// giving in the assignement
// this code has been built on top of the code given at the course

int main()
{
    struct sockaddr_in server;
    struct sockaddr_in from;

    int nr;
    int sd; //socket descriptor;
    pthread_t th[100];

    //creating socket
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[SERVER] Error creating socket\n");
        return errno;
    }

    int on = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    //attaching the socket
    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[SERVER] Error binding socket\n");
        return errno;
    }

    // listening for connections
    if (listen(sd, 10) == -1)
    {
        perror("[SERVER] Error listening\n");
        return errno;
    }

    //serving clients concurrently

    while (1)
    {
        int client;
        thData *td;
        int length = sizeof(from);

        printf("[SERVER] Waiting at port %d ... \n\n", PORT);
        fflush(stdout);

        if ((client = accept(sd, (struct sockaddr *)&from, &length)) == -1)
        {
            perror("[SERVER] Error accepting client connection\n");
            continue;
        }

        td = (struct thData *)malloc(sizeof(struct thData));
        td->idThread = indx++;
        td->cl = client;

        pthread_create(&th[indx], NULL, &threadHandler, td);
    }

    close(sd);
    return 0;
}

static void *threadHandler(void *arg)
{
    struct thData tdH;
    tdH = *((struct thData *)arg);
    printf("[THREAD] : %d : Waiting for message...\n\n", tdH.idThread);
    fflush(stdout);

    pthread_detach(pthread_self());

    //function call
    messageHandler((struct thData *)arg);

    --indx;
    close(tdH->cl);
    return (NULL);
}

void nospaces(char buff[], int size)
{
    for (int i = 0; i < size;)
        if (buff[i] == ' ')
        {
            strcpy(buff + i, buff + i + 1);
            --size;
        }
        else
            ++i;
}

int exists(char *buff)
{
    if (userSize == 0)
        return -1;
    for (int i = 0; i < userSize; i++)
        if (strcmp(buff, users[i].username) == 0)
            return 1;
    return -1;
}

void messageHandler(void *arg)
{
    char buffer[512];

    struct thData tdH;
    tdH = *(struct thData *)arg;

    int ok = 1;
    int buffLen;
    while (ok)
    {
        bzero(buffer, sizeof(buffer));
        if (read(tdH.cl, &buffLen, sizeof(buffLen)) == -1)
        {
            perror("[SERVER] Error reading message from client.\n");
            printf("[SERVER] [THREAD %d]\n\n", tdH.idThread);
        }

        if (read(tdH.cl, buffer, buffLen) == -1)
        {
            perror("[SERVER] Error at reading from client\n\n");
            printf("[SERVER] [THREAD %d]\n\n", tdH.idThread);
        }

        buffer[buffLen-1] = '\0';
        buffLen--;
        printf("[SERVER] [THREAD %d] Message : %s\n\n", tdH.idThread, buffer);
        nospaces(buffer, strlen(buffer));
        printf("[SERVER] [THREAD %d] Message after edit: %s\n\n", tdH.idThread, buffer);

        fflush(stdout);

        if(commandHandler(buffer,arg) == -1)
        {
            printf("[SERVER] [THREAD %d] Invalid command.\n",tdH.idThread);
            perror("[SERVER] Invalid command.\n");
            const char answer[] = "[CLIENT] Invalid command.\n";
            int lanswer = (-1) * (strlen(answer));

            if(write(tdH.cl,&lanswer,sizeof(int)) <= 0)
            {
                perror("[SERVER] Error writing to client.\n");
            }

            if(write(tdH.cl,answer,-lanswer) <= 0)
            {
                perror("[SERVER] Error writing to client.\n");
            }
        }
    }
}

int commandHandler(char buff[], void *arg)
{
    toLower(buff);
    nospaces(buff, strlen(buff));
    printf("[SERVER] Command after processing: %s\n", buff);
    int command = checkCommand(buff);

    switch(command)
    {
        case 1:
        {
            loginHandler(buff,arg);
            return 1;
            break;
        }

        default:
        {
            return -1;
            break;
        }
    }
}

void toLower(char buff[])
{
    for (int i = strlen(buff) - 1; i >= 0; --i)
        buff[i] = tolower(buff[i]);
}

int checkCommand(char *buff)
{
    if (strcmp(buff, "login") == 0)
        return 1;

    return -1;
}

void loginHandler(char buff[],void* arg)
{
    printf("Intru login handler\n");
    struct thData tdH;
    tdH = *(struct thData*)arg;

    char buffer[512];
    int bufferLength;

    strcpy(buffer,"Insert username: \0");
    bufferLength = strlen(buffer);

    printf("Scriu catre client insert\n");
    if(write(tdH.cl,&bufferLength,sizeof(bufferLength)) <= 0)
    {
        perror("[SERVER] Error writing to client.\n");
    }

    if(write(tdH.cl,buffer,bufferLength) <= 0)
    {
        perror("[SERVER] Error writing to client.\n");
    }

    printf("Am scris spre client\n");
    printf("\n\n%s\n\n%d\n\n",buffer,bufferLength);
    bzero(buffer,bufferLength);
    printf("Citesc de la client ce am obtinut\n");
    if(read(tdH.cl,&bufferLength,sizeof(bufferLength)) == -1)
    {
        perror("[SERVER] Error reading from client.\n");
    }
    if(read(tdH.cl,buffer,bufferLength) == -1)
    {
        perror("[SERVER] Error reading from client.\n");
    }
    buffer[bufferLength-1] = '\0';
    bufferLength--;

    printf("Am citit de la client\n");

    if(exists(buffer) == -1)
    {
        users[userSize].logged = 1;
        strcpy(users[userSize].username,buffer);
        userSize++;
        strcpy(buffer,"Logged in successfully.\n");
        bufferLength = strlen(buffer);
    }
    else
    {
        strcpy(buffer,"Already logged in.\n");
        bufferLength = strlen(buffer);
    }
    printf("Am adaugat user-ul\n");
    printf("Scriu spre client\n");
    if(write(tdH.cl,&bufferLength,sizeof(bufferLength)) <= 0)
    {
        perror("[SERVER] Error writing to client.\n");
    }

    if(write(tdH.cl,buffer,bufferLength) <= 0)
    {
        perror("[SERVER] Error writing to client.\n");
    }
    printf("Am scris spre client\n");
}