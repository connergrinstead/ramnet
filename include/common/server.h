/**
 * @file server.h
 * 
 * @brief Header file for server.c.
 */

#ifndef SERVER_H
#define SERVER_H

#include <stdlib.h>

#define SERVER_BACKLOG 1U

int64_t create_tcp_listener(char * p_ip, char * p_port);


#endif /* SERVER_H */

/*** end of file ***/
