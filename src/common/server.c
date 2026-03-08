/**
 * @file network.c
 * 
 * @brief Source for basic network operations.
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

#include "../../include/common/server.h"


static struct addrinfo *
get_addr_structs(const char * p_addr,
                 const char * p_port,
                 int32_t socktype)
{
    struct addrinfo hints       = { 0 };
    struct addrinfo * p_results = NULL;

    if ((p_addr == NULL) || (p_port == NULL))
    {
        return NULL;
    }

    hints.ai_family     = AF_UNSPEC;
    hints.ai_socktype   = socktype;
    hints.ai_flags      = AI_PASSIVE;

    if (getaddrinfo(p_addr, p_port, &hints, &p_results) != 0)
    {
        p_results = NULL;
    }

    return p_results;
}


static int64_t
bind_addr(const char * p_addr,
          const char * p_port,
          int32_t socktype)
{
    int64_t fd = -1;

    struct addrinfo * p_socket_addr_list = NULL;
    struct addrinfo * p_socket_addr      = NULL;

    if ((p_addr == NULL) || (p_port == NULL))
    {
        return -1;
    }

    p_socket_addr_list = get_addr_structs(p_addr, p_port, socktype);

    if (p_socket_addr_list != NULL)
    {
        for (p_socket_addr = p_socket_addr_list;
             p_socket_addr != NULL;
             p_socket_addr = p_socket_addr->ai_next)
        {
            fd = socket(p_socket_addr->ai_family,
                        p_socket_addr->ai_socktype,
                        p_socket_addr->ai_protocol);

            if (fd < 0)
            {
                continue;
            }

            int32_t option = 1;
            setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                       &option, sizeof(option));

            if (bind(fd,
                     p_socket_addr->ai_addr,
                     p_socket_addr->ai_addrlen) < 0)
            {
                close(fd);
                fd = -1;
            }
            else
            {
                break;
            }
        }

        freeaddrinfo(p_socket_addr_list);
    }

    return fd;
}


int64_t
create_tcp_listener(char * p_ip, char * p_port)
{
    int64_t fd = -1;

    if ((p_ip == NULL) || (p_port == NULL))
    {
        goto cleanup;
    }

    fd = bind_addr(p_ip, p_port, SOCK_STREAM);

    if (fd < 0)
    {
        goto cleanup;
    }

    if (listen(fd, SERVER_BACKLOG) != 0)
    {
        goto cleanup;
    }

    return fd;

cleanup:
    if (fd > 0)
    {
        close(fd);
    }

    return -1;
}


int64_t
create_udp_listener(char * p_ip, char * p_port)
{
    int64_t fd = -1;

    if ((p_ip == NULL) || (p_port == NULL))
    {
        goto cleanup;
    }

    fd = bind_addr(p_ip, p_port, SOCK_DGRAM);

    if (fd < 0)
    {
        goto cleanup;
    }

    return fd;

cleanup:
    if (fd > 0)
    {
        close(fd);
    }

    return -1;
}


/*** end of file ***/