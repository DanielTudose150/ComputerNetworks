#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>

#define READ 0
#define WRITE 1
#define FIFO_NAME "TGDFIFO"

const int MAX_READ_SIZE = 256;
const int MAX_ANSWER_SIZE = 10000;

const char* userConfig = "users.txt";

const char* successfulLogin = "@> Login succeeded!\n";
const char* failedLogin = "@> Login failed! User doesn't exist.\n";
const char* notLoggedIn = "@> Command cannot be executed because there is no user logged in.\n";
const char* invalidCommand = "@> Invalid command. Please insert a valid command.\n";

// https://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c
int fileExists(const char* file) 
{
    return access(file, F_OK) != -1;
}

void UserConfigExists() 
{
    // access returneaza 0 daca exista fisierul. 0 != -1 => 1. daca returneaza 0, inseamna ca nu exista
    if(!fileExists(userConfig))
    {
        printf("@> \"%s\" doesn't exist and program cannot be run. Create the file and run the program again. 6\n",userConfig);
        exit(6);
    }
}

//  https://stackoverflow.com/questions/13566082/how-to-check-if-a-file-has-content-or-not-using-c
void UserConfigEmpty()
{
    FILE* file;
    if(NULL == ( file = fopen(userConfig,"r") ))
    {
        perror("Erorr opening user config file. 7\n");
        exit(7);
    }

    if( -1 == fseek( file, 0, SEEK_END ) ) 
    {
        perror("Error at seeking end of user config file. 8\n");
        exit(8);
    }

    int filePosition = ftell( file );
    if(filePosition == 0)
    {
        printf("@> %s is empty. Insert values in the file and run the program again.\n 9",userConfig);
        exit(9);
    }

    fclose(file);
}

void readLine(char* line,int size)
{
    // fgets returneaza NULL la eroare sau EOF fara sa citeasca ceva;
    if( fgets(line,size,stdin) == NULL )
    {
        return;
    }

    // daca if e bypassed, inseamna ca citirea a fost un sucess;
    // fgets also read newlines, deci suprascriu newline.
    line[strlen(line)-1] = 0;

}


int UserExists(char* username)
{
    FILE* file;
    if(NULL == (file = fopen(userConfig,"r") ) )
    {
        perror("Error at opening user config file. 14\n");
        exit(14);
    }

    if( -1 == fseek(file, 0,SEEK_SET) )
    {
        perror("Error at seeking start of file. 41\n");
        exit(41);
    }


    char userLine[MAX_READ_SIZE];

    while(fgets(userLine, MAX_READ_SIZE, file))
    {
        if(!strncmp(userLine,username,strlen(userLine)-1))
            {   
                fclose(file);
                return 1;
            }
    }

    fclose(file);
    return 0;

}

int myStat(char* path,char* answer,int* answerLength)
{
    
    struct stat mystat;
    int isDirectory = 0;

    if( -1 == stat(path,&mystat) )
    {
        perror("Error on stat for mystat. 31\n");
        return -31;
    }

    /*
        https://stackoverflow.com/questions/2674312/how-to-append-strings-using-sprintf

        For SPRINTF with append
    */

    // https://stackoverflow.com/questions/18318483/how-to-display-uid-size-and-files-type
    sprintf(answer,"UID: %u\n",(unsigned int)mystat.st_uid);
    sprintf(answer + strlen(answer),"GID: %u\n",(unsigned int)mystat.st_gid);

    // LINUX MANUAL PAGE 2 FOR STAT
    sprintf(answer + strlen(answer),"File type: ");
    switch(mystat.st_mode & S_IFMT) 
    {
        case S_IFBLK:
            sprintf(answer + strlen(answer),"block device.\n");
            break;
        case S_IFCHR:
            sprintf(answer + strlen(answer),"character device.\n");
            break;
        case S_IFDIR:
            {
                isDirectory = 1;
                sprintf(answer + strlen(answer),"directory.\n");
                break;
            }
        case S_IFIFO:
            sprintf(answer + strlen(answer),"FIFO or pipe\n");
            break;
        case S_IFLNK:
            sprintf(answer + strlen(answer),"Symbolic link\n");
            break;
        case S_IFREG:
            sprintf(answer + strlen(answer),"regular file.\n");
            break;
        case S_IFSOCK:
            sprintf(answer + strlen(answer),"socket\n");
            break;
        default:
            sprintf(answer + strlen(answer),"unknown\n");
            break;
    }

    // LINUX MANUAL PAGE 2 FOR CHMOD
    sprintf(answer + strlen(answer),"Permissions: ");
    char permissions[9];
    mode_t perm;
    permissions[0] = (perm & S_IRUSR) ? 'r' : '-';
    permissions[1] = (perm & S_IWUSR) ? 'w' : '-';
    permissions[2] = (perm & S_IXUSR) ? 'x' : '-';
    permissions[3] = (perm & S_IRGRP) ? 'r' : '-';
    permissions[4] = (perm & S_IWGRP) ? 'w' : '-';
    permissions[5] = (perm & S_IXGRP) ? 'x' : '-';
    permissions[6] = (perm & S_IROTH) ? 'r' : '-';
    permissions[7] = (perm & S_IWOTH) ? 'w' : '-';
    permissions[8] = (perm & S_IXOTH) ? 'x' : '-';
    sprintf(answer + strlen(answer),"%s\n",permissions);

    sprintf(answer + strlen(answer),"File size: %ld bytes\n",(long)mystat.st_size);
    sprintf(answer + strlen(answer),"Blocks allocated: %ld of 512-byte units\n",(long)mystat.st_blocks);
    sprintf(answer + strlen(answer),"Last status change: %s\n",ctime(&mystat.st_ctime));
    sprintf(answer + strlen(answer),"Last file access: %s",ctime(&mystat.st_atime));
    sprintf(answer + strlen(answer),"Last file modification: %s",ctime(&mystat.st_mtime));

    *answerLength = strlen(answer);
    answer[(*answerLength)] = 0;
    ++(*answerLength);
    return isDirectory;
}

int searchDirectory(char* path,char* answer,int* answerLength,int found,int isDirectory)
{

    // FUNCTIA MYSFIND VA AVEA COMPORTAMENT SIMILAR CU COMANDA FIND DIN LINUX


    if(!found)
    {

        char info[MAX_ANSWER_SIZE];
        int infoLength;
        int* infoptr = &infoLength;

        // fuctia este recursiva. acest parametru va evita reapelarea functiei mystat
        // pentru fiecare element al lui path
        found = 1;
        isDirectory = myStat(path,info,infoptr);

        if(isDirectory < 0)
        {
            perror("Error in \"myStat\" function inside the \"searchDirectory\" function. 43\n");
            return -43;
        }

        if(!isDirectory)
        {
            sprintf(answer,"\n%s\n\n",path);
            sprintf(answer+strlen(answer),"%s\n",info);
            *answerLength = strlen(answer);
            answer[*answerLength] = 0;
            ++(*answerLength);
            return 0;
        }

        // IS DIRECTORY
        sprintf(answer,"%s is a directory. Information:\n%s\nResult of find:\n%s\n",path,info,path);
        
    }

    // IS DIRECTORY

    DIR* dir;

    if( NULL == (dir=opendir(path)) )
    {
        perror("Error at opening directory in \"searchDirectory\". 42\n");
        return -42;
    }

    struct dirent* dirptr;
    char dirPath[MAX_ANSWER_SIZE];
    strcpy(dirPath,path);

    while( (dirptr = readdir(dir)) != NULL)
    {
        if(strcmp(dirptr->d_name,".") && strcmp(dirptr->d_name,".."))
        {
            sprintf(answer + strlen(answer),"%s/%s\n",path,dirptr->d_name);
            if( dirptr->d_type == DT_DIR )
            {
                int len=strlen(dirPath);
                sprintf(dirPath + len,"/%s",dirptr->d_name);
                searchDirectory(dirPath,answer,answerLength,found,isDirectory);
            }

        }
    }

    *answerLength = strlen(answer);
    answer[*answerLength]=0;
    ++(*answerLength);
    return 0;
}

int main()
{

    UserConfigExists();
    UserConfigEmpty();

    int loggedIn = 0;
    int running = 1;

    char line[MAX_READ_SIZE];
    char answer[MAX_ANSWER_SIZE];
    int answerLength;
    int *answerptr = &answerLength;
    int found = 0;
    int isDirectory = 0;

    while(running)
    {
        printf("@> ");

        pid_t PID;

        readLine(line,MAX_READ_SIZE);

        if(!strncmp(line,"quit",4) ){
            running = 0;
        }
        else
        if(!strncmp(line,"login : ",8) )
        {
            int parentToChild[2];
            int childToParent[2];

            if(-1 == pipe(parentToChild))
            {
                perror("Error creating pipe parentToChild. 1\n");
                exit(1);
            }

            if(-1 == pipe(childToParent))
            {
                perror("Error creating pipe childToParent. 2\n");
                exit(2);
            }

            if( -1 == (PID = fork() ) ) 
            {
                perror("Error forking for \"login\" command. 11\n");
                exit(11);
            }
            
            if(PID)
            {
                // PARENT PROCESS

                // PIPE COMMUNICATION

                if( -1 == close(parentToChild[READ]))
                {
                    perror("Error at closing parentToChild pipe. 15\n");
                    exit(15);
                }
                if( -1 == close(childToParent[WRITE]))
                {
                    perror("Error at closing childToParent pipe. 16\n");
                    exit(16);
                }
                if( -1 == write(parentToChild[WRITE],line,strlen(line)+1))
                {
                    perror("Error at writing to parentToChild pipe.\n 19");
                    exit(19);
                }
                int answerCode;
                if( -1 == read(childToParent[READ],&answerCode,sizeof(int)) )
                {
                    perror("Error at reading from childToParent pipe. 25\n");
                    exit(25);
                }
                if( -1 == read(childToParent[READ],answer,answerCode))
                {
                    perror("Error at reading from childToParent pipe. 28\n");
                    exit(28);
                }
                if(!strncmp(answer,successfulLogin,answerCode))
                {
                    loggedIn = 1;
                    printf("%s",successfulLogin);
                }
                else
                {
                    if(!strncmp(answer,failedLogin,answerCode))
                    {
                        loggedIn = 0;
                        printf("%s",failedLogin);
                    }
                }
                if( -1 == close(parentToChild[WRITE]))
                {
                    perror("Error at closing parentToChild pipe. 29\n");
                    exit(29);
                }
                if( -1 == close(childToParent[READ]))
                {
                    perror("Error at closing childToParent pipe. 30\n");
                    exit(30);
                }
            }
            else
            {
                // CHILD PROCESS
                if(-1 == close(parentToChild[WRITE]))
                {
                    perror("Error at closing parentToChild pipe. 17\n");
                    exit(17);
                }
                if(-1 == close(childToParent[READ]))
                {
                    perror("Error at closing childToParent pipe. 18\n");
                    exit(18);
                }
                if(-1 == read(parentToChild[READ],line,MAX_READ_SIZE))
                {
                    perror("Error at reading from parentToChild pipe. 20\n");
                    exit(20);
                }
                char* param = line + 8; 
                int paramLen = -1;        
                if(UserExists(param))
                {
                    paramLen = strlen(successfulLogin);
                    if( -1 == write(childToParent[WRITE],&paramLen,sizeof(int)))
                    {
                        perror("Error writing to childToParent pipe. 26\n");
                        exit(26);
                    }
                    if( -1 == write(childToParent[WRITE],successfulLogin,strlen(successfulLogin)+1))
                    {
                        perror("Error writing to childToParent pipe. 27\n");
                        exit(27);
                    }
                }
                else
                {
                    paramLen = strlen(failedLogin);
                    if( -1 == write(childToParent[WRITE],&paramLen,sizeof(paramLen)) )
                    {
                        perror("Error at writing to childToParent pipe. 21\n");
                        exit(21);
                    }
                    if(-1 == write(childToParent[WRITE],failedLogin,strlen(failedLogin)+1))
                    {
                        perror("Error at writing to childToParent pipe. 22\n");
                        exit(22);
                    }
                }
                if( -1 == close(parentToChild[READ])){
                    perror("Error at closing childToParent pipe. 23\n");
                    exit(23);
                }
                if( -1 == close(childToParent[WRITE]))
                {
                    perror("Error at closing childToParent pipe. 24\n");
                    exit(24);
                }
                //killing child process;
                exit(0);
            }
            

        }
        else
        if(!strncmp(line,"myfind",6) )
        {
            if(!loggedIn)
            {
                printf("%s",notLoggedIn);
                continue;
            }

            if(fileExists(FIFO_NAME))
            {
                if( -1 == remove(FIFO_NAME) )
                {
                    perror("Error at removing fifo. 100\n");
                    exit(100);
                }
            }

            if(-1 == mknod(FIFO_NAME,S_IFIFO | 0666,0))
            {
                perror("Error creating FIFO. 3\n");
                exit(3);
            }
            
            int forFIFO;

            if(-1 == (PID = fork() ) )
            {
                perror("Error forking for \"myfind\" command. 12\n");
                exit(12);
            }
            
            if(PID)
            {
                // PARENT PROCESS

                // FIFO COMMUNICATION

                // DEFAULT FILE IS THE CURRENT DIRECTORY
                if(!strcmp(line,"myfind"))
                {
                    strcat(line," .");
                }

                // PROCESARE

                // deschid fifo, scriu fifo, inchid fifo. astept. 

                if( -1 == (forFIFO = open(FIFO_NAME,O_WRONLY)) )
                {
                    perror("Error at opening fifo for myfind parent. 44\n");
                    exit(44);
                }

                if(-1 == write(forFIFO,line,strlen(line)+1) )
                {
                    perror("Error at writing to fifo in parent for myfind. 45\n");
                    exit(45);
                }

                if( -1 == close(forFIFO) )
                {
                    perror("Error at writing to fifo in parent for myfind. 49\n");
                    exit(49);
                }

                sleep(2);

                // deschid fifo, citesc fifo, inchid fifo. afisez ce trebuie

                if( -1 == (forFIFO = open(FIFO_NAME,O_RDONLY)) ) 
                {
                    perror("Error at opening fifo read-only parent. 56.\n");
                    exit(56);
                }

                if( -1 == read(forFIFO,&answerLength,sizeof(int)) )
                {
                    perror("Error at reading from FIFO parent. 57.\n");
                    exit(57);
                }

                if( -1 == read(forFIFO,answer,answerLength) )
                {
                    perror("Error at reading from FIFO parent. 58.\n");
                    exit(58);
                }

                printf("%s",answer);

                if( -1 == close(forFIFO) )
                {
                    perror("Error at closing FIFO. 59\n");
                    exit(59);
                }

            }
            else
            {
                // CHILD PROCESS

                // deschide fifo. citeste fifo. inchide fifo

                if( -1 == (forFIFO = open(FIFO_NAME,O_RDONLY) ) )
                {
                    perror("Error at opening FIFO in child. 47\n");
                    exit(47);
                }

                if( -1 == read(forFIFO,line,MAX_READ_SIZE) )
                {
                    perror("Error at reading FIFO in child. 48\n");
                    exit(48);
                }

                if(-1 == close(forFIFO) )
                {
                    perror("Error at closing FIFO in child. 50\n");
                    exit(50);
                }

                // deschide fifo. scrie fifo. inchide fifo

                char* param = line + 7;
                int* answerptr = &answerLength;

                if( searchDirectory(param,answer,answerptr,found,isDirectory) < 0)
                {
                    perror("Error at \"searchDirectory\" function call in child. 51\n");
                    exit(51);
                }

                if( -1 == (forFIFO = open(FIFO_NAME,O_WRONLY)) )
                {
                    perror("Error at \"open\" function call in child. 52\n");
                    exit(52);
                }

                if( -1 == write(forFIFO,&answerLength,sizeof(int)) )
                {
                    perror("Error at \"write\" function call in child. 53\n");
                    exit(53);
                }

                if( -1 == (write(forFIFO,answer,answerLength)) )
                {
                    perror("Error at \"write\" function call in child. 54\n");
                    exit(54);
                }

                if( -1 == close(forFIFO) )
                {
                    perror("Error at \"close\" function call in child. 55\n");
                    exit(55);
                }

                exit(0);
            }


        }
        else
        if(!strncmp(line,"mystat ",7) )
        {
            if(!loggedIn)
            {
                printf("%s",notLoggedIn);
                continue;
            }


            int socket[2];

            if(-1 == socketpair(AF_UNIX,SOCK_STREAM,0,socket))
            {
                perror("Error creating socket. 4\n");
                exit(4); 
            } 


            if( -1 == (PID = fork() ) )
            {
                perror("Error forking for \"mystat\" command. 13\n");
                exit(13);
            }

            if(PID)
            {
                // PARENT PROCESS

                // SOCKET COMMUNICATION

                if( -1 == close(socket[1]) )
                {
                    perror("Error on closing socketpair of parent. 32\n");
                    exit(32);
                }
                
                if( -1 == write(socket[0],line,MAX_READ_SIZE) )
                {
                    perror("Error at writing into parent socketpair. 33\n");
                    exit(33);
                }

                sleep(1);
                printf("\n\nanswerLength before %d ; ",answerLength);
                if( -1 == read(socket[0],&answerLength,sizeof(int)) )
                {
                    perror("Error at reading from parent socket. 35\n");
                    exit(35);
                }   
                
                if( -1 == read(socket[0],answer,answerLength) )
                {
                    perror("Error at reading from parent socketpair. 34\n");
                    exit(34);
                }

                printf("%s",answer);

                if( -1 == close(socket[0]) )
                {
                    perror("Error at closing parent socketpair. 36\n");
                    exit(36);
                }

            }
            else
            {
                // CHILD PROCESS

                if( -1 == close(socket[0]) )
                {
                    perror("Eror on closing socketpair of child. 33\n");
                    exit(33);
                }

                if( -1 == read(socket[1],line,MAX_READ_SIZE) )
                {
                    perror("Error at reading from child socketpair. 37\n");
                    exit(37);
                }
                
                char* param = line + 7;
                int *answerptr = &answerLength;
                if(myStat(param,answer,answerptr) < 0)
                {
                    // ERROR ON STAT FUNCTION
                    exit(31);
                }

                if( -1 == write(socket[1],&answerLength,sizeof(int)) )
                {
                    perror("Error on writing into chuld socketpair. 38\n");
                    exit(38);
                }

                if( -1 == write(socket[1],answer,answerLength) )
                {
                    perror("Error on writing into chuld socketpair. 39\n");
                    exit(39);
                }

                if( -1 == close(socket[1]) )
                {
                    perror("Error on closing child socketpair. 40\n");
                    exit(40);
                }

                exit(0);
            }
            
        }
        else
        {
            // INVALID COMMAND

            printf("%s",invalidCommand);
        }
        
    }

    return 0;
}