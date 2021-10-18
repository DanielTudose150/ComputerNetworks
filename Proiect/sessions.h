#ifndef SESSSIONS_H_GUARD
#define SESSSIONS_H_GUARD

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MAX_READ_SIZE 256

int readLine(char* line,int size);

int receive_string_message(int fd,char* message,int* length);
int receive_int_message(int fd,int* message);
int send_string_message(int fd,char* message,int* length);
int send_int_message(int fd,int* message);

int send_board(int fd,int matrix[][7]);
int receive_board(int fd,int matrix[][7]);

#endif