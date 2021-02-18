#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>

extern int errno;

int port;

int portDefault = 2908;
char *addressDefault = "127.0.0.1";

int main(int argc, char *argv[])
{
    int sd; //socket descriptor
    struct sockaddr_in server;

    char buffer[512];

    char *address;

    switch (argc)
    {
    case 1:
    {
        port = portDefault;
        address = strdup(addressDefault);
        break;
    }

    case 2:
    {
        port = portDefault;
        address = strdup(argv[1]);
        break;
    }

    case 3:
    {
        port = atoi(argv[2]);
        address = strdup(argv[1]);
        break;
    }

    default:
        return 0;
    }

    //creating socket
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[CLIENT] Error creating socket\n");
        return errno;
    }

    //filling struct sockaddr_in structure
    server.sin_family = AF_INET;                 // socket family
    server.sin_addr.s_addr = inet_addr(address); // socket address
    server.sin_port = htons(port);               // socket port

    //connecting to server
    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[CLIENT] Error connecting to server\n");
        return errno;
    }

    //reading message

    int ok = 1;
    int buffLen;
    while (ok)
    {
        bzero(buffer, sizeof(buffer));
        printf("[CLIENT] Insert command: ");
        fflush(stdout);

        read(0, buffer, sizeof(buffer));

        printf("\n[CLIENT] Message read : %s\n", buffer);

        buffLen=strlen(buffer);
        if(write(sd,&buffLen,sizeof(buffLen)) <= 0)
        {
            perror("[CLIENT] Error writing to server.\n");
            return errno;
        }

        if(write(sd,buffer,buffLen) <= 0)
        {
            perror("[CLIENT] Error writing to server.\n");
            return errno;
        }

        bzero(buffer, sizeof(buffer));

        if(read(sd,&buffLen,sizeof(buffLen)) == -1)
        {
            perror("[CLIENT] Error reading from server.\n");
            return errno;
        }

        if(buffLen < 0)
        {
            buffLen *= (-1);
            if(read(sd,buffer,buffLen) == -1)
            {
                perror("[CLIENT] Error reading message from server.\n");
                return errno;
            }

            printf("%s\n",buffer);
            continue;
        }
        else
        {
            printf("\n\n%d\n\n",buffLen);
            if(read(sd, buffer,buffLen) == -1)
            {
                perror("[CLIENT] Error reading message from server.\n");
                return errno;
            }

            printf("%s",buffer);

            bzero(buffer, sizeof(buffer));

            if(read(0,&buffLen,sizeof(buffLen)) == -1)
            {
                perror("[CLIENT] Error reading message from server.\n");
                return errno;
            }

            if(read(0, buffer, buffLen) == -1)
            {
                perror("[CLIENT] Error reading message from server.\n");
                return errno;
            }

            buffer[buffLen-1] = '\0';
            buffLen--;

            if(write(sd,&buffLen,sizeof(buffLen)) <= 0)
            {
                perror("[CLIENT] Error writing to server.\n");
                return errno;
            }

            if(write(sd, buffer, buffLen) <= 0)
            {
                perror("[CLIENT] Error writing to server.\n");
                return errno;
            }

            bzero(buffer, sizeof(buffer));

            if(read(sd,&buffLen, sizeof(buffLen)) == -1)
            {
                perror("[CLIENT] Error reading from server.\n");
                return errno;
            }

            if(read(sd, buffer, buffLen) == -1)
            {
                perror("[CLIENT] Error reading from server.\n");
                return errno;
            }

            printf("[CLIENT] %s\n",buffer);
        }
        

        
    }

    close(sd);


    return 0;
}