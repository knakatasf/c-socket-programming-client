#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <threads.h>
#include <cjson/cJSON.h>
#include <fcntl.h>

#include "../include/client_func.h"
#include "../include/standalone_func.h"
#include "../include/config.h"

int main(int argc, char* argv[])
{   
    if (argc != 2)
    {
        fprintf(stderr, "Program argument error: %s <config.json>\n", argv[0]);
        return EXIT_FAILURE;
    }
    load_config(argv[1]);

    thrd_t sender_thread, listener_thread;

    if (thrd_create(&sender_thread, send_syn_and_train, NULL) != thrd_success)
    {
        perror("Failed to create sender thread.\n");
        return EXIT_FAILURE;
    }

    if (thrd_create(&listener_thread, listen_to_rsts, NULL) != thrd_success)
    {
        perror("Failed to create listener thread.\n");
        return EXIT_FAILURE;
    }

    thrd_join(sender_thread, NULL);
    thrd_join(listener_thread, NULL);

    printf("Program finished.\n");
    return EXIT_SUCCESS;
}