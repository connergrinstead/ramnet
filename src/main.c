/**
 * @file main.c
 * 
 * @brief Source for ramnet server/controller.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "../include/common/server.h"


#define IP_ADDR "0.0.0.0"
#define IP_PORT "5555"

volatile sig_atomic_t gb_shutdown = 0;


/* CTRL-C signal handler */
void
handle_sigint(int32_t sig)
{
    gb_shutdown = 1;
}

void
auth_client(int64_t client_fd)
{
    
}


void *
c2_thread(void * p_arg)
{
    int64_t client_fd = *(int64_t *)p_arg;

    char p_buffer[32]     = { 0 };
    char p_op_buffer[16]  = { 0 };
    char p_arg_buffer[16] = { 0 };
    ssize_t buffer_len = 0;

    printf("[\033[31m*\033[0m] Thread creation: c2_thread(%zu)\n", client_fd);

    buffer_len = recv(client_fd, p_buffer, 31, 0);
    p_buffer[31] = 0;

    printf("Buffer:\n%s\n", p_buffer);

    memcpy(p_op_buffer, p_buffer, 4);
    memcpy(p_arg_buffer, p_buffer + 5, 16);

    if (!strcmp(p_op_buffer, "AUTH"))
    {
        printf("AUTH received.\n");
        send(client_fd, "AUTHENTICATED\n", 15, 0);
    }
    else
    {
        printf("No AUTH received.\n");
    }
}


int
main(int argc, char ** pp_argv)
{
    /* Sigaction setup for CTRL-C termination */
    struct sigaction sa = { 0 };
    sa.sa_handler       = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    /* Client and c2 socket setup */
    int64_t c2_fd     = -1;
    int64_t client_fd = -1;
    struct sockaddr_in client_addr = { 0 };
    socklen_t addr_len = sizeof(client_addr);

    /* Thread construction */
    pthread_t t_server_thread  = { 0 };

    c2_fd = create_tcp_listener(IP_ADDR, IP_PORT);
    if (c2_fd > 0)
    {
        printf("[\033[36m*\033[0m] C2 socket successfully created.\n"
        "[\033[36m*\033[0m] Listening on socket \033[36m%s:%s\033[0m [%zu].\n",
        IP_ADDR, IP_PORT, c2_fd);
    }
    else
    {
        fprintf(stderr, "ERROR: Failed to create C2 socket.\n");
        goto cleanup;
    }

    /* Wait for client connection */
    client_fd = accept(c2_fd, (struct sockaddr *)&client_addr, &addr_len);
    if (client_fd > 0)
    {
        printf("[\033[35m*\033[0m] Received initial client connection.\n");
        close(c2_fd);
        c2_fd = -1;
    }
    else
    {
        goto cleanup;
    }

    /* Launch TCP command thread */
    if (pthread_create(&t_server_thread, NULL, c2_thread, &client_fd) != 0)
    {
        fprintf(stderr, "ERROR: Failed to launch C2 thread.\n");
        goto cleanup;
    }
    pthread_join(t_server_thread, NULL);

cleanup:
    if (client_fd > 0)
    {
        shutdown(client_fd, SHUT_RDWR);
        close(client_fd);
    }

    if (c2_fd > 0)
    {
        close(c2_fd);
    }

    return 0;
}

/*** end of file ***/
