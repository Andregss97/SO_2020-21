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

int numberThreads = 0;
pthread_mutex_t lock;



/*void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void processInput(char* filename){
    char line[MAX_INPUT_SIZE];

    FILE *file = fopen(filename,"r");
    if( file == NULL ) {
        fprintf(stderr, "Error: no such file\n");
        exit(EXIT_FAILURE);
    }
    // break loop with ^Z or ^D
    while (fgets(line, sizeof(line)/sizeof(char), file)) {
        char token, type;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %c", &token, name, &type);

        // perform minimal validation
        if (numTokens < 1) {
            continue;
        }

        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'l':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case '#':
                break;
            
            default: { // error
                errorParse();
            }
        }
    }
    fclose(file);
}*/

void* applyCommands(void* param){

    char token, type;
    char name[MAX_INPUT_SIZE];
    FILE *stdout;
    // char file1[MAX_INPUT_SIZE];
    // char file2[MAX_INPUT_SIZE];

    char* command = (char*) param;
    printf("COMMAND: %s\n", command);

/*-------------------------------------------------------------------------------------------------*/
    // if(flag == 1) 
    pthread_mutex_lock(&lock);
    // if (flag == 2)
        /* rw lock*/
        // pthread_rwlock_rdlock(&rwl);

    int numTokens = sscanf(command, "%c %s %c", &token, name, &type);

    // if(flag == 1) 
    //     pthread_mutex_unlock(&lock);
    // if (flag == 2)
        /* rw unlock*/
        // pthread_rwlock_unlock(&rwl);

    if (numTokens < 2) {
        fprintf(stderr, "Error: invalid command in Queue\n");
        exit(EXIT_FAILURE);
    }

    int searchResult;
    switch (token) {
        case 'c':

            // if(flag == 1) 
            //     pthread_mutex_lock(&lock);
            // if (flag == 2)
                /* rw lock*/
                // pthread_rwlock_wrlock(&rwl);

            switch (type) {
                case 'f':
                        
                    printf("Create file: %s\n", name);
                    create(name, T_FILE);
                    printf("ACABEI CREATE");
                    break;
                case 'd':
                        
                    printf("Create directory: %s\n", name);
                    create(name, T_DIRECTORY);
                    break;
                default:
                    fprintf(stderr, "Error: invalid node type\n");
                    exit(EXIT_FAILURE);
            }

            // if(flag == 1) 
            //     pthread_mutex_unlock(&lock);
            // if (flag == 2)
                /* rw unlock*/
                // pthread_rwlock_unlock(&rwl);

            break;
        case 'l':
            
            // if(flag == 1) 
            //     pthread_mutex_lock(&lock);
            // if (flag == 2)
                /* rw lock*/
                // pthread_rwlock_rdlock(&rwl);

            searchResult = lookup(name);
                
            if (searchResult >= 0)
                printf("Search: %s found\n", name);
            else
                printf("Search: %s not found\n", name);
            
            // if(flag == 1) 
            //     pthread_mutex_unlock(&lock);
            // if (flag == 2)
                /* rw unlock*/
                // pthread_rwlock_unlock(&rwl);

            break;
        
        case 'd':
            
            // if(flag == 1) 
            //     pthread_mutex_lock(&lock);
            // if (flag == 2)
                /* rw lock*/
                // pthread_rwlock_wrlock(&rwl);

            printf("Delete: %s\n", name);
            delete(name);
            
            // if(flag == 1) 
            //     pthread_mutex_unlock(&lock);
            // if (flag == 2)
                /* rw unlock*/
                // pthread_rwlock_unlock(&rwl);

            break;

        /*case 'm':
            move(file1, file2);
            break;*/

        case 'p':
            stdout = fopen(name, "w");
            print_tecnicofs_tree(stdout);
            fclose(stdout);
            break;

        default: { /* error */
            fprintf(stderr, "Error: command to apply\n");
            exit(EXIT_FAILURE);
        }
    }
    // if(flag == 1) 
    pthread_mutex_unlock(&lock);
/*-------------------------------------------------------------------------------------------------*/
    printf("AQUI");
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
        char in_buffer[MAX_INPUT_SIZE], out_buffer[OUTDIM];
        int c;

        addrlen=sizeof(struct sockaddr_un);
        c = recvfrom(sockfd, in_buffer, sizeof(in_buffer)-1, 0,
            (struct sockaddr *)&client_addr, &addrlen);
        if (c <= 0) continue;
        //Preventivo, caso o cliente nao tenha terminado a mensagem em '\0', 
        in_buffer[c]='\0';
        
        printf("Recebeu mensagem %s de %s\n", in_buffer, client_addr.sun_path);

        gettimeofday(&t0, NULL);
        for (int i=0; i < numberThreads; i++) {
            if (pthread_create(&(tid[i]), NULL, applyCommands, &in_buffer) != 0) {
                printf("Error creating thread.\n");
                return -1;
            }
        }

        for (int i=0; i < numberThreads; i++) {
            if(pthread_join(tid[i], NULL) != 0) {
                printf("Error joining thread.\n");
                return -1;
            }
            printf("JOIN");
        }
        gettimeofday(&t1, NULL);
        timersub(&t1, &t0, &totalT);    

        printf("cheguei");

        c = sprintf(out_buffer, "TecnicoFS completed in %ld.%04ld seconds.\n\n", totalT.tv_sec, totalT.tv_usec);
        
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
