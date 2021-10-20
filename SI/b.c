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

#include "cryptography.h"
#include "communication.h"

#define PORT 2728
#define PORT2 2729
#define DEFAULT_ADDRESS "127.0.0.1"

const char *IV = "[09]{18}(73)46b5";
const char *KPRIME = "9173582640642513";
char K[17];
char encryptedK[17];

extern int errno;

char buffer[256];
char message[256];
int length;
int *lenptr = &length;

int ECB;

int port;

void decryptK(char *e);
void removePadding(char *buff);

int main(int argc, char *argv[])
{
    int sd;
    struct sockaddr_in server;

    port = PORT2;

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
    printf("[CLIENT] - B\n");
    fflush(stdout);

    strcpy(message, "test");
    length = strlen(message);

    if (sendStringMessage(sd, message, lenptr) == -1)
    {
        printf("[CLINET] Error sending test message.\n");
        fflush(stdout);
    }

    if (receiveIntMessage(sd, &ECB) == -1)
    {
        printf("\nError receiving communication method\n");
        fflush(stdout);
    }

    printf("Received ECB: %d\n", ECB);

    // receiving encrypted K
    if (receiveStringMessage(sd, encryptedK, lenptr) == -1)
    {
        printf("[CLIENT] Error receiving encrypted K");
        fflush(stdout);
    }

    //decryptK(encryptedK);
    decryptBlockECB(encryptedK, K, KPRIME);

    printf("EK:%s\nK:%s\n\n", encryptedK, K);

    strcpy(message, "commence");
    length = strlen(message);

    printf("sending: %s", message);
    if (sendStringMessage(sd, message, lenptr) == -1)
    {
        printf("[CLIENT] Error sending confirmation message.\n");
        fflush(stdout);
    }

    printf("\n\nWaiting for blocks...\n\n");

    int finished = 0;

    char buff[17];
    char block[17];
    char ciphertext[17];
    length = 16;
    lenptr = &length;

    FILE *file = fopen("result.txt", "w");
    FILE *cipher = fopen("ciphertext.txt", "w");

    printf("Received:\n\n");
    fflush(stdout);

    int step = 0;
    if (ECB)
    {
        /*
        while(!finished)
        {
            bzero(buff,sizeof(buff));
            bzero(block,sizeof(block));
            printf("====STEP %d=====\n\n",step++);
            if(receiveStringMessage(sd,buff,lenptr) == -1)
            {
                printf("[CLIENT] Error receiving encrypted text");
            }

            if(length < 16)
                break;

            printf("Buff:%s\nLen%d\n",buff,length);
            fputs(buff,cipher);
            
            
            decryptBlockECB(buff,block,K);
            printf("Block:%s\nLen:%d\n\n",block,strlen(block));
            //block[16] = 0;
            removePadding(block);
            fputs(block,file);
            

            if(sendIntMessage(sd,&ECB) == -1)
            {
                printf("[CLIENT] Error sending int message.\n\n");
            }
        }
        */

        bzero(buff, sizeof(buff));
        bzero(block, sizeof(block));

        if (receiveStringMessage(sd, buff, lenptr) == -1)
        {
            printf("[CLIENT] Error receiving encrypted text");
        }
        printf("====Step %d====\nBuff: %s\nLen: %d\n", step++, buff, strlen(buff));
        fputs(buff, cipher);

        /*
        encryptInitialCFB(buff, block, K, IV);
        strcpy(ciphertext, buff);
        printf("Block: %s\n\n", block);
        removePadding(block);
        fputs(block, file);
        */

        decryptBlockECB(buff,block,K);
        printf("Block: %s\n",block);
        removePadding(block);
        fputs(block,file);

        if (sendIntMessage(sd, &ECB) == -1)
        {
            printf("[CLIENT] Error sending int message.\n\n");
        }

        while (!finished)
        {
            bzero(buff, sizeof(buff));
            bzero(block, sizeof(block));

            if (receiveStringMessage(sd, buff, lenptr) == -1)
            {
                printf("[CLIENT] Error receiving encrypted text");
            }

            if (length < 16)
                break;

            printf("====Step %d====\nBuff: %s\nLen: %d\n", step++, buff, strlen(buff));
            fputs(buff, cipher);

            decryptBlockECB(buff,block,K);
            printf("Block: %s\n",block);
            removePadding(block);
            fputs(block,file);

            if (sendIntMessage(sd, &ECB) == -1)
            {
                printf("[CLIENT] Error sending int message.\n\n");
            }
        }
    }
    else
    {
        bzero(buff, sizeof(buff));
        bzero(block, sizeof(block));

        if (receiveStringMessage(sd, buff, lenptr) == -1)
        {
            printf("[CLIENT] Error receiving encrypted text");
        }
        printf("====Step %d====\nBuff: %s\nLen: %d\n", step++, buff, strlen(buff));
        fputs(buff, cipher);

        encryptInitialCFB(buff, block, K, IV);
        strcpy(ciphertext, buff);
        printf("Block: %s\n\n", block);
        removePadding(block);
        fputs(block, file);

        if (sendIntMessage(sd, &ECB) == -1)
        {
            printf("[CLIENT] Error sending int message.\n\n");
        }

        while (!finished)
        {
            bzero(buff, sizeof(buff));
            bzero(block, sizeof(block));

            if (receiveStringMessage(sd, buff, lenptr) == -1)
            {
                printf("[CLIENT] Error receiving encrypted text");
            }

            if (length < 16)
                break;

            printf("====Step %d====\nBuff: %s\nLen: %d\n", step++, buff, strlen(buff));
            fputs(buff, cipher);

            encryptCFB(buff, block, K, ciphertext);
            strcpy(ciphertext, buff);
            printf("Block: %s\n", block);
            removePadding(block);
            fputs(block, file);

            if (sendIntMessage(sd, &ECB) == -1)
            {
                printf("[CLIENT] Error sending int message.\n\n");
            }
        }
    }

    fclose(file);

    if(sendIntMessage(sd, &ECB) == -1)
    {
        printf("Error sending int");
    }

    printf("\n\nDecrypted text:\n\n");
    bzero(buff, sizeof(buff));
    file = fopen("result.txt", "r");
    while (fgets(buff, 17, file))
        printf("%s", buff);

    fclose(file);
    fclose(cipher);
    close(sd);

    return 0;
}

void decryptK(char *e)
{
    /*
    for(int i=0;i<16;++i)
        K[i] = e[i] - 97 + 48;
    K[16]=0;
    */
    AES_KEY aesKey;
    int res = AES_set_decrypt_key(KPRIME, 128, &aesKey);
    AES_decrypt(e, K, &aesKey);
}

void removePadding(char *buff)
{
    int i = 16;
    while (buff[i] == '*' || buff[i] == 0)
        buff[i--] = 0;
}