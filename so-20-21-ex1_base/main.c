#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include "fs/operations.h"

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

int numberThreads = 0;
pthread_mutex_t lock;
pthread_rwlock_t rwl;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

char* removeCommand() {
    if(numberCommands > 0){
        numberCommands--;
        return inputCommands[headQueue++];  
    }
    return NULL;
}

void errorParse(){
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
    /* break loop with ^Z or ^D */
    while (fgets(line, sizeof(line)/sizeof(char), file)) {
        char token, type;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %c", &token, name, &type);

        /* perform minimal validation */
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
            
            default: { /* error */
                errorParse();
            }
        }
    }
    fclose(file);
}

void* applyCommands(void* oldF){

    int flag = *(int*) oldF;

    while (numberCommands > 0){

        if(flag == 1) 
            pthread_mutex_lock(&lock);
        if (flag == 2)
            /* rw lock*/
            pthread_rwlock_wrlock(&rwl);

        const char* command = removeCommand();

        if(flag == 1) 
            pthread_mutex_unlock(&lock);
        if (flag == 2)
            /* rw unlock*/
            pthread_rwlock_unlock(&rwl);

        if (command == NULL){
            continue;
        }

        char token, type;
        char name[MAX_INPUT_SIZE];

        if(flag == 1) 
            pthread_mutex_lock(&lock);
        if (flag == 2)
            /* rw lock*/
            pthread_rwlock_rdlock(&rwl);

        int numTokens = sscanf(command, "%c %s %c", &token, name, &type);

        if(flag == 1) 
            pthread_mutex_unlock(&lock);
        if (flag == 2)
            /* rw lock*/
            pthread_rwlock_unlock(&rwl);

        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }
        
        int searchResult;
        switch (token) {
            case 'c':

                if(flag == 1) 
                    pthread_mutex_lock(&lock);
                if (flag == 2)
                    /* rw lock*/
                    pthread_rwlock_wrlock(&rwl);

                switch (type) {
                    case 'f':
                         
                        printf("Create file: %s\n", name);
                        create(name, T_FILE);
                        break;
                    case 'd':
                         
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }

                if(flag == 1) 
                    pthread_mutex_unlock(&lock);
                if (flag == 2)
                    /* rw unlock*/
                    pthread_rwlock_unlock(&rwl);

                break;
            case 'l':
                
                if(flag == 1) 
                    pthread_mutex_lock(&lock);
                if (flag == 2)
                    /* rw lock*/
                    pthread_rwlock_rdlock(&rwl);

                searchResult = lookup(name);
                 
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                
                if(flag == 1) 
                    pthread_mutex_unlock(&lock);
                if (flag == 2)
                    /* rw unlock*/
                    pthread_rwlock_unlock(&rwl);

                break;
            case 'd':
                
                if(flag == 1) 
                    pthread_mutex_lock(&lock);
                if (flag == 2)
                    /* rw lock*/
                    pthread_rwlock_wrlock(&rwl);

                printf("Delete: %s\n", name);
                delete(name);
                
                if(flag == 1) 
                    pthread_mutex_unlock(&lock);
                if (flag == 2)
                    /* rw unlock*/
                    pthread_rwlock_unlock(&rwl);

                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    return NULL;
}

void verify_inputs(int argc, char* argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Error: invalid number of arguments\n");
        exit(EXIT_FAILURE);
    }
    else if (!isalpha(*argv[1])) {
        fprintf(stderr, "Error: the given inputfile is not a char\n");
        exit(EXIT_FAILURE);
    }
    else if (!isalpha(*argv[2])) {
        fprintf(stderr, "Error: the given outputfile is not a char\n");
        exit(EXIT_FAILURE);
    }
    else if (!isdigit(*argv[3])) {
        fprintf(stderr, "Error: the number of threads must be a positive integer\n");
        exit(EXIT_FAILURE);
    }
    else if (!isalpha(*argv[4])) {
        fprintf(stderr, "Error: the given synch strategy is not a char\n");
        exit(EXIT_FAILURE);
    }
    else if (strcmp(argv[4], "nosync") != 0 && strcmp(argv[4], "mutex") != 0 && strcmp(argv[4], "rwlock") != 0) {
        fprintf(stderr, "Error: the given synch strategy is not valid (mutex, rwlock or nosync)\n");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[]) {
    /* init filesystem */
    init_fs();
    /* process input and print tree */
    verify_inputs(argc, argv);
    /* argv[1] aka inputfile given as atribute */
    processInput(argv[1]);
    numberThreads = atoi(argv[3]);
    pthread_t tid[numberThreads];
    int flag = 0;
    if (strcmp(argv[4], "mutex") == 0) {
        flag = 1;
    }
    else if (strcmp(argv[4], "rwlock") == 0) {
        flag = 2;
    }

    if (pthread_mutex_init(&lock, NULL) != 0) { 
        fprintf(stderr, "Error: the mutex failed to initialize\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_rwlock_init(&rwl, NULL) != 0) { 
        fprintf(stderr, "Error: the rwlock failed to initialize\n");
        exit(EXIT_FAILURE);
    }

    for (int i=0; i < numberThreads; i++) {
        if (pthread_create(&(tid[i]), NULL, applyCommands, &flag) != 0) {
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
    
    print_tecnicofs_tree(stdout);

    /* release allocated memory */
    destroy_fs();
    exit(EXIT_SUCCESS);
}
