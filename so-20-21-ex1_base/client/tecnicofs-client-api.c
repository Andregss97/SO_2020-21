#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

#define CLIENT "client"

socklen_t servlen, clilen;
struct sockaddr_un serv_addr, client_addr;
char buffer[1024];
int sockfd;

int setSockAddrUn(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}

int tfsCreate(char *filename, char nodeType) {

  if (sendto(sockfd, filename, strlen(filename)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
      perror("client: sendto error");
      exit(EXIT_FAILURE);
  } 

  if (recvfrom(sockfd, buffer, sizeof(buffer), 0, 0, 0) < 0) {
      perror("client: recvfrom error");
      exit(EXIT_FAILURE);
  } 

  printf("Recebeu resposta do servidor: %s\n", buffer);

  return EXIT_SUCCESS;
}

int tfsDelete(char *path) {
  return -1;
}

int tfsMove(char *from, char *to) {
  return -1;
}

int tfsLookup(char *path) {
  return -1;
}

int tfsMount(char * sockPath) {

  /*if (argc < 3) {
    printf("Argumentos esperados:\n path_client_socket path_server_socket string_a_enviar\n");
    return EXIT_FAILURE;
  }*/

  if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0) ) < 0) {
    perror("client: can't open socket");
    exit(EXIT_FAILURE);
  }

  unlink(CLIENT);
  clilen = setSockAddrUn (CLIENT, &client_addr);
  if (bind(sockfd, (struct sockaddr *) &client_addr, clilen) < 0) {
    perror("client: bind error");
    exit(EXIT_FAILURE);
  }  

  servlen = setSockAddrUn(sockPath, &serv_addr);

  return EXIT_SUCCESS;
}

int tfsUnmount() {
  if(!sockfd){
    return EXIT_FAILURE;
  }
  close(sockfd);

  unlink(CLIENT);
 
  return EXIT_SUCCESS;
}
