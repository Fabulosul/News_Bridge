#ifndef __COMMON_H__
#define __COMMON_H__

#include <stddef.h>
#include <stdint.h>

int send_packet(int sockfd, void *buff, size_t len);
int recv_packet(int sockfd, void *buff, size_t len);

#endif
