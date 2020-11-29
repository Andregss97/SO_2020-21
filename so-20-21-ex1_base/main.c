#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <ctype.h>
#include "tecnicofs-api-constants.h"
#include "fs/operations.h"

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100
#define OUTDIM 512
#define SUCCESS_STATUS 0
#define FAILURE_STATUS -1

int numberThreads = 0;
char* command;
int flag;
pthread_mutex_t lock;



void* applyCommands(){

    char token, type;
    char name[MAX_INPUT_SIZE];
    FILE *stdout;
    // char file1[MAX_INPUT_SIZE];
    // char file2[MAX_INPUT_SIZE];

    printf("COMMAND: (%s)\n", command);

/*-------------------------------------------------------------------------------------------------*/

    pthread_mutex_lock(&lock);

    int numTokens = sscanf(command, "%c %s %c", &token, name, &type);

    if (numTokens < 2) {
        fprintf(stderr, "Error: invalid command in Queue\n");
        exit(EXIT_FAILURE);
    }

    int searchResult;
    switch (token) {
        case 'c':

            switch (type) {
                case 'f':
                        
                    printf("Create file: %s\n", name);
                    if(create(name, T_FILE) == 0)
                        flag = SUCCESS_STATUS;
                    else
                        flag = FAILURE_STATUS;
                    break;
                case 'd':
                        
                    printf("Create directory: %s\n", name);
                    if(create(name, T_DIRECTORY) == 0)
                        flag = SUCCESS_STATUS;
                    else
                        flag = FAILURE_STATUS;
                    break;
                default:
                    fprintf(stderr, "Error: invalid node type\n");
                    exit(EXIT_FAILURE);
            }

            break;
        case 'l':

            searchResult = lookup(name);
                
            if (searchResult >= 0){
                printf("Search: %s found\n", name);
                flag = SUCCESS_STATUS;
            }else{
                printf("Search: %s not found\n", name);
                flag = FAILURE_STATUS;
            }
            break;
        
        case 'd':

            printf("Delete: %s\n", name);
            if(delete(name) == 0)
                flag = SUCCESS_STATUS;
            else
                flag = FAILURE_STATUS;

            break;

        /*case 'm':
            move(file1, file2);

            break;*/

        case 'p':
            stdout = fopen(name, "w");
            print_tecnicofs_tree(stdout);
            fclose(stdout);
            flag = SUCCESS_STATUS;
            break;

        default: { /* error */
            fprintf(stderr, "Error: command to apply\n");
            exit(EXIT_FAILURE);
        }
    }

    pthread_mutex_unlock(&lock);

/*-------------------------------------------------------------------------------------------------*/

    return NULL;
}

int setSockAddrUn(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}


int main(int argc, char* argv[]) {
    int sockfd;
    struct sockaddr_un server_addr;
    socklen_t addrlen;
    char *path;

    // struct timeval t0, t1, totalT;
    /* init filesystem */
    init_fs();
    
    if (pthread_mutex_init(&lock, NULL) != 0) { 
        fprintf(stderr, "Error: the mutex failed to initialize\n");
        exit(EXIT_FAILURE);
    }
    
    /* argv[1] aka number of threads*/
    numberThreads = atoi(argv[1]);
    pthread_t tid[numberThreads];

    if (argc < 2)
    exit(EXIT_FAILURE);

    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("server: can't open socket");
        exit(EXIT_FAILURE);
    }

    path = argv[2];

    unlink(path);

    addrlen = setSockAddrUn (argv[2], &server_addr);
    if (bind(sockfd, (struct sockaddr *) &server_addr, addrlen) < 0) {
        perror("server: bind error");
        exit(EXIT_FAILURE);
    }
    
    while (1) {
        struct sockaddr_un client_addr;
        char in_buffer[MAX_INPUT_SIZE], out_buffer[MAX_INPUT_SIZE];
        int c;

        addrlen=sizeof(struct sockaddr_un);
        c = recvfrom(sockfd, in_buffer, sizeof(in_buffer)-1, 0,
            (struct sockaddr *)&client_addr, &addrlen);
        
        command = in_buffer;

        printf("Recebeu mensagem %s de %s\n", command, client_addr.sun_path);

        // gettimeofday(&t0, NULL);
        for (int i=0; i < numberThreads; i++) {
            if (pthread_create(&(tid[i]), NULL, applyCommands, NULL) != 0) {
                printf("Error creating thread.\n");
                return -1;
            }
        }

        for (int i=0; i < numberThreads; i++) {
            if(pthread_join(tid[i], NULL) != 0) {
                printf("Error joining thread.\n");
                return -1;
            }
        }
        // gettimeofday(&t1, NULL);
        // timersub(&t1, &t0, &totalT);    

        c = sprintf(out_buffer, "%d", flag);
        
        sendto(sockfd, out_buffer, c+1, 0, (struct sockaddr *)&client_addr, addrlen);

    }

    //Fechar e apagar o nome do socket, apesar deste programa 
    //nunca chegar a este ponto
    close(sockfd);
    unlink(argv[2]);
    exit(EXIT_SUCCESS);

    /* release allocated memory */
    destroy_fs();
    exit(EXIT_SUCCESS);
}
