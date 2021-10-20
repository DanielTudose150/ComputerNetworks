#ifndef COMMUNICATION_H_GUARD
#define COMMUNICATION_H_GUARD

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MAX_READ_SIZE 256
#define BLOCK_SIZE 16

int readLine(char* line, int size);

int receiveStringMessage(int fd, char* message, int* length);
int receiveIntMessage(int fd, int* message);

int sendStringMessage(int fd, char* messsage, int* length);
int sendIntMessage(int fd, int* message);

#endif