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

const char* IV = "[09]{18}(73)46b5";
const char* KPRIME = "9173582640642513";
char K[17];
char encryptedK[17];

extern int errno;

char buffer[256];
char message[256];
int length;
int* lenptr = &length;

int ECB;

int port;

void decryptK(char* e);

int main(int argc,char* argv[])
{
    int sd;
    struct sockaddr_in server;
    
    port = PORT2;

    if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
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

    strcpy(message,"test");
    length = strlen(message);

    if(sendStringMessage(sd,message,lenptr) == -1)
    {
        printf("[CLINET] Error sending test message.\n");
        fflush(stdout);
    }

    if(receiveIntMessage(sd,&ECB) == -1)
    {
        printf("\nError receiving communication method\n");
        fflush(stdout);
    }

    printf("Received ECB: %d\n",ECB);

    // receiving encrypted K
    if(receiveStringMessage(sd,encryptedK,lenptr) == -1)
    {
        printf("[CLIENT] Error receiving encrypted K");
        fflush(stdout);
    }

    //decryptK(encryptedK);
    decryptBlockECB(encryptedK,K,KPRIME);

    printf("EK:%s\nK:%s\n\n",encryptedK,K);

    strcpy(message,"commence");
    length = strlen(message);

    printf("sending: %s",message);
    if(sendStringMessage(sd, message, lenptr) == -1)
    {
        printf("[CLIENT] Error sending confirmation message.\n");
        fflush(stdout);
    }

    printf("\n\nWaiting for blocks...\n\n");

    int finished = 0;

    char buff[17];
    char block[17];
    char chipertext[17];
    length = 16;
    lenptr = &length;

    FILE* file = fopen("result.txt","w");

    printf("Received:\n\n");
    fflush(stdout);
    if(ECB)
    {
        while(!finished)
        {
            if(receiveStringMessage(sd,buff,lenptr) == -1)
            {
                printf("[CLIENT] Error receiving encrypted text");
            }

            printf("Buff:%s\nLen%d\n\n",buff,length);
            if(length != 16)
                finished = 1;
            else
            {
                decryptBlockECB(buff,block,K);
                printf("Block:%s\nLen:%d\n\n",block,strlen(block));
                block[16] = 0;
                fputs(block,file);
                printf("Scriu in file.\n");
            }

            printf("Scriu int\n");
            if(sendIntMessage(sd,&finished) == -1)
            {
                printf("[CLIENT] Error sending int message.\n\n");
            }
            printf("AM scris int\n\n");
        }
    }
    else
    {
        while(!finished)
        {

        }
    }
    
    fclose(file);

    printf("\n\nDecrypted text:\n\n");
    file = fopen("result.txt","r");
    while(fgets(buff,17,file))
        printf("%s",buff);


    close(sd);

    return 0;
}

void decryptK(char* e)
{
    /*
    for(int i=0;i<16;++i)
        K[i] = e[i] - 97 + 48;
    K[16]=0;
    */
   AES_KEY aesKey;
   int res = AES_set_decrypt_key(KPRIME,128,&aesKey);
   AES_decrypt(e,K,&aesKey);
}