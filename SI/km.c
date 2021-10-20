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

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/conf.h>
#include <openssl/err.h>

#include "communication.h"
#include "cryptography.h"

#define PORT 2728
#define DEFAULT_ADDRESS "127.0.0.1"
#define AES_KEY_SIZE 16
#define AES_BLOCK_SIZE 16

const char* KPRIME = "9173582640642513";

char K[17];

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

static void* treat(void*);
void createClient(void*);

char buffer[256];
int messageLength;

void createK();
void encryptK(char* result);

void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}

int main(int argc,char* argv[])
{

    struct sockaddr_in server;
    struct sockaddr_in from;
    int sd;
    pthread_t th[2];
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

    while(1)
    {
        int client;
        thData* td;
        int length = sizeof(from);

        printf("[SERVER] Waiting at port %d...\n", PORT);
        fflush(stdout);

        if((client = accept(sd, (struct sockaddr*)&from,&length)) < 0)
        {
            perror("[SERVER] Error at accept.\n");
        }

        td = (struct thData*)malloc(sizeof(struct thData));
        td -> idThread = i++;
        td -> cd = client;
        pthread_create(&th[i],NULL,&treat, td);
    }
    
    close(sd);

    return 0;
}

static void* treat(void* arg)
{
    struct thData tdL;
    tdL = *((struct thData*)arg);
    printf("[THREAD]-%d - Connected\n",tdL.idThread);
    fflush(stdout);
    pthread_detach(pthread_self());
    createClient((struct thData*) arg);
    close((intptr_t)arg);
    return (NULL);
}

void createClient(void* arg)
{
    char buffer[256];
    char buffer2[256];
    char message[256];
    char result[17];
    int length;
    int* lenptr = &length;

    struct Client client;
    client.data = *((struct thData*)arg);
    struct Client* clientptr = &client;

    bzero(buffer,sizeof(buffer));

    printf("Waiting for message....");
    if(receiveStringMessage(client.data.cd,message,lenptr) == -1)
    {
        printf("[SERVER] Error receiving string message\n");
        fflush(stdout);
    }

    printf("Message received:\n\n");

    printf("Message:%s\nLength:%d\n",message,length);

    printf("Creating K\n");

    createK();
    printf("K: %s\n",K);
    //encryptK(result);
    encryptBlockECB(K,result,KPRIME);
    printf("Encrypted K: %s\n",result);

    length = 16;
    if(sendStringMessage(client.data.cd,result, lenptr) == -1)
    {
        printf("[SERVER] Error sending encrypted K\n");
        fflush(stdout);
    }
    
    printf("Encrypted key sent\n");

    if(receiveIntMessage(client.data.cd,lenptr) == -1)
    {
        printf("[SERVER] Error receiving int\n");
        fflush(stdout);
    }

    exit(0);
}

void createK()
{
    /*
    for(int i=0;i<16;++i)
        K[i] = (i % 10) + 48;
    K[16] = 0;
    */

   int res = RAND_bytes(K,16);
   K[16] = 0;
}

void encryptK(char* result) 
{
    /*
    for(int i=0;i<16;++i)
        result[i] = 97 + K[i] - 48;
    result[16] = 0;
    */
   AES_KEY aesKey;
   int res = AES_set_encrypt_key(KPRIME,128,&aesKey);
   AES_encrypt(K,result,&aesKey);
   result[16] = 0;
}