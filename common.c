#include "common.h"

#include <sys/socket.h>
#include <sys/types.h>

/*
    TODO 1.1: Rescrieți funcția de mai jos astfel încât ea să facă primirea
    a exact len octeți din buffer.
*/
int recv_packet(int sockfd, void *buffer, size_t len) {
  size_t bytes_received = 0;
  size_t bytes_remaining = len;
  char *buff = buffer;
  
    while(bytes_remaining > 0) {
      bytes_received = recv(sockfd, buff, bytes_remaining, 0);
      bytes_remaining -= bytes_received;
      buff = (char *)buff + bytes_received;
    }

  /*
    TODO: Returnam exact cati octeti am citit
  */
  return len;
}

/*
    TODO 1.2: Rescrieți funcția de mai jos astfel încât ea să facă trimiterea
    a exact len octeți din buffer.
*/

int send_packet(int sockfd, void *buffer, size_t len) {
  size_t bytes_sent = 0;
  size_t bytes_remaining = len;
  char *buff = buffer;
  
  while(bytes_remaining) {
      bytes_sent = send(sockfd, buff, bytes_remaining, 0);
      bytes_remaining -= bytes_sent;
      buff = (char *)buff + bytes_sent;
  }
  

  /*
    TODO: Returnam exact cati octeti am trimis
  */
  return len;
}
