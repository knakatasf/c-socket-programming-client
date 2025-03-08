#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
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
    long zero_packet_duration = measure_time_diff(CLIENT_IP, SERVER_IP, UNCORP_TCP_HEAD, UNCORP_TCP_TAIL, NUM_OF_UDP_PACKETS, UDP_SIZE);
}