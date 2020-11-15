#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <ctype.h>
#include "fs/operations.h"

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

int numberThreads = 0;

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
        char type, token;
        char name[MAX_INPUT_SIZE];
        char dir1[MAX_INPUT_SIZE];
        char dir2[MAX_INPUT_SIZE];
        int numTokens;
        printf("LINE: %c\n", line[0]);
        if (strcmp(&line[0], "m") == 0) {
            numTokens = sscanf(line, "%c %s %s", &token, dir1, dir2);
        }
        else {
            numTokens = sscanf(line, "%c %s %c", &token, name, &type);
        }
        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }

        switch (token) {
            case 'c':
                if(numTokens != 3){
                    printf("C erro: %d\n", numTokens);
                    errorParse();
                }
                if(insertCommand(line))
                    break;
                return;
            
            case 'm':
                if(numTokens != 3){
                    printf("M erro: %d\n", numTokens);
                    errorParse();
                }
                if(insertCommand(line))
                    break;
                return;
            
            case 'l':
                if(numTokens != 2){
                    printf("L erro: %d\n", numTokens);
                    errorParse();
                }
                if(insertCommand(line))
                    break;
                return;
            
            case 'd':
                if(numTokens != 2){
                    printf("D erro: %d\n", numTokens);
                    errorParse();
                }
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

void* applyCommands(){


    while (numberCommands > 0){

        const char* command = removeCommand();

        if (command == NULL){
            continue;
        }

        char type, token;
        char name[MAX_INPUT_SIZE];
        char dir1[MAX_INPUT_SIZE];
        char dir2[MAX_INPUT_SIZE];
        int numTokens;

        if (command[0] == 'm') {
            numTokens = sscanf(command, "%c %s %s", &token, dir1, dir2);
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
                        create(name, T_FILE);
                        break;
                    case 'd':
                         
                        //printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l':
                
                lookup(name);
                 
                //if (searchResult >= 0)
                    //printf("Search: %s found\n", name);
                //else
                    //printf("Search: %s not found\n", name);
                
                break;
            
            case 'd':

                //printf("Delete: %s\n", name);
                delete(name);

                break;

            case 'm':
                move(dir1, dir2);
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
    if (argc != 4) {
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
}

int main(int argc, char* argv[]) {
    struct timeval t0, t1, totalT;
    /* init filesystem */
    init_fs();

    /* process input and print tree */
    verify_inputs(argc, argv);
    /* argv[1] aka inputfile given as atribute */
    processInput(argv[1]);
    numberThreads = atoi(argv[3]);
    pthread_t tid[numberThreads];

    gettimeofday(&t0, NULL);
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
