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
#include <netdb.h>

#include "communication.h"
#include "cryptography.h"

#define PORT 2728
#define PORT2 2729
#define DEFAULT_ADDRESS "127.0.0.1"

const char *IV = "[09]{18}(73)46b5";
const char *KPRIME = "9173582640642513";
char K[17];
char result[17];
char encryptedK[17];

extern int errno;

typedef struct thData
{
    int idThread;
    int cd;
} thData;

typedef struct Client
{
    struct thData data;
} Client;

static void *treat(void *);
void createClient(void *);

char buffer[256];
char message[256];
int length;
int *lenptr = &length;

int port;
int ECB = 0;
int chosen = 0;

void decryptK(char *result);
void sendFile();

void addPadding(char *buff, int pos);
int nonZeroChars(char *buff);

int main(int argc, char *argv[])
{

    //_client_A:
    int sd;
    struct sockaddr_in server;

    port = PORT;

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[CLIENT] - Error creating socket.\n");
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

    bzero(message, sizeof(message));
    printf("[CLIENT] - Test\n");
    fflush(stdout);

    sprintf(message, "%s", "abcdefghijklmnop");
    length = strlen(message);
    printf("Message:%s\nLength:%d\n", message, length);

    printf("Sending message...\n\n");

    if (sendStringMessage(sd, message, lenptr) == -1)
    {
        printf("[CLIENT] Error on sending message.\n");
        fflush(stdout);
    }

    printf("Message sent\n");

    bzero(message, sizeof(message));
    if (receiveStringMessage(sd, message, lenptr) == -1)
    {
        printf("[CLIENT] Error receiving K\n");
        fflush(stdout);
    }

    printf("Received: %s\n", message);
    strcpy(encryptedK, message);

    //decryptK(message);
    decryptBlockECB(message, K, KPRIME);

    printf("Decrypted K: %s\n", K);

    if (sendIntMessage(sd, lenptr) == -1)
    {
        printf("[CLIENT] Error sedining disconnecting message\n");
        fflush(stdout);
    }

    close(sd);

    fflush(stdout);
    fflush(stdin);

    //_server_A:
    //struct sockaddr_in server;
    struct sockaddr_in from;
    //int sd;
    pthread_t th[2];
    int i = 0;

    port = PORT2;

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

    server.sin_port = htons(PORT2);

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

    chosen = 0;
    while (!chosen)
    {
        printf("\n\nPlease insert the number corresponind to the desired encryption method:\n\t1. CFB\n\t2. ECB\n");
        printf("Insert option: ");
        scanf("%d", &ECB);
        ECB--;
        printf("ECB:%d\n\n", ECB);

        switch (ECB)
        {
        case 0:
        case 1:
            chosen = 1;
            break;
        defualt:
        {
            printf("\n\nPlease insert a valid option\n");
            fflush(stdout);
            fflush(stdin);
            break;
        }
        }
    }

    while (1)
    {
        int client;
        thData *td;
        int length = sizeof(from);

        printf("[SERVER] Waiting at port %d...\n", PORT);
        fflush(stdout);

        if ((client = accept(sd, (struct sockaddr *)&from, &length)) < 0)
        {
            perror("[SERVER] Error at accept.\n");
        }

        td = (struct thData *)malloc(sizeof(struct thData));
        td->idThread = i++;
        td->cd = client;
        pthread_create(&th[i], NULL, &treat, td);
    }

    close(sd);

    return 0;
}

static void *treat(void *arg)
{
    struct thData tdL;
    tdL = *((struct thData *)arg);
    printf("[THREAD]-%d - Connected\n", tdL.idThread);
    fflush(stdout);
    pthread_detach(pthread_self());
    createClient((struct thData *)arg);
    close((intptr_t)arg);
    return (NULL);
}

void createClient(void *arg)
{
    char buffer[256];
    char buff[17];
    char message[256];
    char result[17];
    int length;
    int *lenptr = &length;

    struct Client client;
    client.data = *((struct thData *)arg);
    struct Client *clientptr = &client;

    bzero(buffer, sizeof(buffer));

    printf("Testing connection with B\n\n");
    if (receiveStringMessage(client.data.cd, buffer, lenptr) == -1)
    {
        printf("[SERVER] Error receiving testing message");
    }

    printf("\n\nSending communication method: %d\n\n", ECB);

    if (sendIntMessage(client.data.cd, &ECB) == -1)
    {
        printf("\nError sending communication method\n");
        fflush(stdout);
    }

    //sending K

    strcpy(message, encryptedK);
    length = strlen(message);

    printf("\n\nSending EK: %s\n", encryptedK);

    if (sendStringMessage(client.data.cd, message, lenptr) == -1)
    {
        printf("[SERVER] Error sending encrypted K\n");
        fflush(stdout);
    }

    // receiving confirmation

    if (receiveStringMessage(client.data.cd, message, lenptr) == -1)
    {
        printf("[SERVER] Error receiving confirmation message\n");
        fflush(stdout);
    }

    printf("Received message:%s\n", message);

    bzero(buffer, sizeof(buffer));
    int finished = 0;
    FILE *file = fopen("plaintext.txt", "r");
    char block[17];
    char ciphertext[17];
    length = 16;
    lenptr = &length;
    int step = 0;

    if (!ECB)
    {
        fgets(buff, 17, file);

        if (strlen(buff) < 16)
            addPadding(buff, strlen(buff));

        encryptInitialCFB(buff, block, K, IV);
        strcpy(ciphertext, block);
        printf("====Step %d====\nBuff: %s\nBlock: %s\nCipher: %s\n\n", step++, buff, block, ciphertext);
        length = 16;

        if (sendStringMessage(client.data.cd, block, lenptr) == -1)
        {
            printf("[SERVER] Error sending encrypted block\n");
            fflush(stdout);
        }

        if (receiveIntMessage(client.data.cd, lenptr) == -1)
        {
            printf("[SERVER] Error receiving int message.\n");
            fflush(stdout);
        }

        bzero(buff, sizeof(buff));
        bzero(block, sizeof(block));

        while (fgets(buff, 17, file))
        {

            if (strlen(buff) < 16)
                addPadding(buff, strlen(buff));

            encryptCFB(buff, block, K, ciphertext);
            strcpy(ciphertext, block);
            printf("====Step %d====\nBuff: %s\nBlock: %s\nCipher: %s\n\n", step++, buff, block, ciphertext);
            length = 16;

            if (sendStringMessage(client.data.cd, block, lenptr) == -1)
            {
                printf("[SERVER] Error sending encrypted block\n");
                fflush(stdout);
            }

            if (receiveIntMessage(client.data.cd, lenptr) == -1)
            {
                printf("[SERVER] Error receiving int message.\n");
                fflush(stdout);
            }

            bzero(buff, sizeof(buff));
            bzero(block, sizeof(block));
        }

        strcpy(buffer, "exit");
        length = strlen(buffer);
        if (sendStringMessage(client.data.cd, message, lenptr) == -1)
        {
            printf("[SERVER] Error sending ending block\n");
            fflush(stdout);
        }
    }
    else
    {
        /*
        bzero(buff,sizeof(buff));
        bzero(block,sizeof(block));
        while(fgets(buff,17,file))
        {    
            
            printf("====STEP %d=====\n\n",step++);
            printf("Buff:%s\nLen: %d\n",buff,strlen(buff));

            if(strlen(buff) < 16) 
            {
                addPadding(buff,strlen(buff));
            }

            encryptBlockECB(buff,block,K);
            length = 16;
            printf("EBuff:%s\nLen: %d\n\n",block,strlen(block));
            
            if(sendStringMessage(client.data.cd,block,lenptr) == -1)
            {
                printf("[SERVER] Error sending encrypted block\n");
                fflush(stdout);
            }

            if(receiveIntMessage(client.data.cd,lenptr) == -1)
            {
                printf("[SERVER] Error receiving int message.\n");
                fflush(stdout);
            }
            bzero(buff,sizeof(buff));
            bzero(block,sizeof(block));
            
        }

        strcpy(buffer,"exit");
        length = strlen(buffer);
        if(sendStringMessage(client.data.cd,message,lenptr) == -1)
        {
            printf("[SERVER] Error sending ending block\n");
            fflush(stdout);
        }
        */
       
        bzero(buff,sizeof(buff));
        bzero(block,sizeof(block));
        fgets(buff, 17, file);

        if (strlen(buff) < 16)
            addPadding(buff, strlen(buff));

        //encryptInitialCFB(buff, block, K, IV);
        //strcpy(ciphertext, block);
        //printf("====Step %d====\nBuff: %s\nBlock: %s\nCipher: %s\n\n", step++, buff, block, ciphertext);
        encryptBlockECB(buff,block,K);
        printf("====Step %d====\nBuff: %s\nBlock: %s\n",step++,buff,block);
        length = 16;

        if (sendStringMessage(client.data.cd, block, lenptr) == -1)
        {
            printf("[SERVER] Error sending encrypted block\n");
            fflush(stdout);
        }

        if (receiveIntMessage(client.data.cd, lenptr) == -1)
        {
            printf("[SERVER] Error receiving int message.\n");
            fflush(stdout);
        }

        bzero(buff, sizeof(buff));
        bzero(block, sizeof(block));

        while (fgets(buff, 17, file))
        {

            if (strlen(buff) < 16)
                addPadding(buff, strlen(buff));

            encryptBlockECB(buff,block,K);
            printf("====Step %d====\nBuff: %s\nBlock: %s\n",step++,buff,block);
            length = 16;

            if (sendStringMessage(client.data.cd, block, lenptr) == -1)
            {
                printf("[SERVER] Error sending encrypted block\n");
                fflush(stdout);
            }

            if (receiveIntMessage(client.data.cd, lenptr) == -1)
            {
                printf("[SERVER] Error receiving int message.\n");
                fflush(stdout);
            }

            bzero(buff, sizeof(buff));
            bzero(block, sizeof(block));
        }

        strcpy(buffer, "exit");
        length = strlen(buffer);
        if (sendStringMessage(client.data.cd, message, lenptr) == -1)
        {
            printf("[SERVER] Error sending ending block\n");
            fflush(stdout);
        }
    }

    strcpy(buffer, "exit");
    length = strlen(buffer);
    if (sendStringMessage(client.data.cd, message, lenptr) == -1)
    {
        printf("[SERVER] Error sending ending block\n");
        fflush(stdout);
    }

    return;
}

void decryptK(char *result)
{
    /*
    for(int i=0;i<16;++i)
        K[i] = result[i] - 97 + 48;
    K[16]=0;
    */

    AES_KEY aesKey;
    int res = AES_set_decrypt_key(KPRIME, 128, &aesKey);
    AES_decrypt(result, K, &aesKey);
}

void addPadding(char *buff, int pos)
{
    for (int i = pos; i < 16; ++i)
        buff[i] = '*';
    buff[16] = 0;
}

int nonZeroChars(char *buff)
{
    int c = 0;
    for (int i = 0; i < 16; ++i)
        if (buff[i] != 0)
            c++;
    return c;
}