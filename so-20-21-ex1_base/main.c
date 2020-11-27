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
#define MAX_LOCKS 50        //same as inode_table size
#define SERVER "/tmp/server"
#define INDIM 30
#define OUTDIM 512

int numberThreads = 0;

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void* applyCommands(char* command){

    char type, token;
    char name[MAX_INPUT_SIZE];
    char file1[MAX_INPUT_SIZE];
    char file2[MAX_INPUT_SIZE];
    int numTokens;
    pthread_rwlock_t *rwl;

    int* buffer_locks = malloc(MAX_LOCKS * sizeof(int));
    int i = 0;
    int* count = &i;

    if (command[0] == 'm') {
        numTokens = sscanf(command, "%c %s %s", &token, file1, file2);
    }
    else {
        numTokens = sscanf(command, "%c %s %c", &token, name, &type);
    }

    // numTokens = sscanf(command, "%c %s %c", &token, name, &type);

    if (numTokens < 2) {
        fprintf(stderr, "Error: invalid command in Queue\n");
        exit(EXIT_FAILURE);
    }

    //int searchResult;
    switch (token) {
        case 'c':

            switch (type) {
                case 'f':
                        
                    //printf("Create file: %s\n", name);
                    printf("CREATE: buffer %d %d %d %d %d\n", buffer_locks[0], buffer_locks[1], buffer_locks[2], buffer_locks[3], buffer_locks[4]);
                    create(name, T_FILE, buffer_locks);
                    break;
                case 'd':
                        
                    //printf("Create directory: %s\n", name);
                    printf("CREATE: buffer %d %d %d %d %d\n", buffer_locks[0], buffer_locks[1], buffer_locks[2], buffer_locks[3], buffer_locks[4]);
                    create(name, T_DIRECTORY, buffer_locks);
                    break;
                default:
                    fprintf(stderr, "Error: invalid node type\n");
                    exit(EXIT_FAILURE);
            }
            break;
        case 'l':
            
            printf("LOOKUP: buffer %d %d %d %d %d\n", buffer_locks[0], buffer_locks[1], buffer_locks[2], buffer_locks[3], buffer_locks[4]);
            lookup(name, buffer_locks, LOOKUP, count);

            for (int i=0; i < *count; i++){
                inode_get_lock(buffer_locks[i], &rwl);
                pthread_rwlock_unlock(rwl);
                printf("\t[%ld] Buffer index: %d // UnLock no iNode: %d\n", pthread_self(), i, buffer_locks[i]);
            }

            printf("[%ld] DOES FREE lookup\n", pthread_self());
            free(buffer_locks);

                
            //if (searchResult >= 0)
                //printf("Search: %s found\n", name);
            //else
                //printf("Search: %s not found\n", name);
            
            break;
        
        case 'd':

            //printf("Delete: %s\n", name);
            printf("DELETE: buffer %d %d %d %d %d\n", buffer_locks[0], buffer_locks[1], buffer_locks[2], buffer_locks[3], buffer_locks[4]);
            delete(name, buffer_locks);

            break;

        case 'm':
            move(file1, file2, buffer_locks);
            break;
            
        default: { /* error */
            fprintf(stderr, "Error: command to apply\n");
            exit(EXIT_FAILURE);
        }
    }
    // printf("DOES FREE");
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

    struct timeval t0, t1, totalT;
    /* init filesystem */
    init_fs();
    /* process input and print tree */
    
    /* argv[1] aka inputfile given as atribute */
    numberThreads = atoi(argv[1]);
    //pthread_t tid[numberThreads];

    if (argc < 2)
    exit(EXIT_FAILURE);

    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("server: can't open socket");
        exit(EXIT_FAILURE);
    }

    path = SERVER;

    unlink(path);

    addrlen = setSockAddrUn (SERVER, &server_addr);
    if (bind(sockfd, (struct sockaddr *) &server_addr, addrlen) < 0) {
        perror("server: bind error");
        exit(EXIT_FAILURE);
    }
    
    while (1) {
        struct sockaddr_un client_addr;
        char in_buffer[INDIM], out_buffer[OUTDIM];
        int c;

        addrlen=sizeof(struct sockaddr_un);
        c = recvfrom(sockfd, in_buffer, sizeof(in_buffer)-1, 0,
            (struct sockaddr *)&client_addr, &addrlen);
        if (c <= 0) continue;
        //Preventivo, caso o cliente nao tenha terminado a mensagem em '\0', 
        in_buffer[c]='\0';
        
        printf("Recebeu mensagem de %s\n", client_addr.sun_path);

        c = sprintf(out_buffer, "Ola' %s, que tal vai isso?", in_buffer);
        
        sendto(sockfd, out_buffer, c+1, 0, (struct sockaddr *)&client_addr, addrlen);

    }

    //Fechar e apagar o nome do socket, apesar deste programa 
    //nunca chegar a este ponto
    close(sockfd);
    unlink(SERVER);
    exit(EXIT_SUCCESS);

    gettimeofday(&t0, NULL);
    /*for (int i=0; i < numberThreads; i++) {
        if (pthread_create(&(tid[i]), NULL, applyCommands, NULL) != 0) {
            printf("Error creating thread.\n");
            return -1;
        }
    }

    for (int i=0; i < numberThreads; i++) {
        printf("THREAD NUMBER [%d]: %ld\n\n\n", i, tid[i]);
        if(pthread_join(tid[i], NULL) != 0) {
            printf("Error joining thread.\n");
            return -1;
        }
    }*/
    gettimeofday(&t1, NULL);
    timersub(&t1, &t0, &totalT);

    printf("TecnicoFS completed in %ld.%04ld seconds.\n\n", totalT.tv_sec, totalT.tv_usec);
    
    FILE *stdout = fopen(argv[2],"w");
    print_tecnicofs_tree(stdout);
    fclose(stdout);

    /* release allocated memory */
    destroy_fs();
    exit(EXIT_SUCCESS);
}
