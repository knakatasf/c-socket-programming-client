#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cjson/cJSON.h>
#include <fcntl.h>

#include "../include/client_func.h"
#include "../include/config.h"

int main(int argc, char* argv[])
{   
    if (argc != 2)
    {
        fprintf(stderr, "Program argument error: %s <config.json>\n", argv[0]);
        return EXIT_FAILURE;
    }

    load_config(argv[1]);
    send_json(argv[1], SERVER_IP, PRE_TCP_PORT);
    send_packet_trains(CLIENT_IP, CLIENT_UDP_PORT, SERVER_IP, SERVER_UDP_PORT, NUM_OF_UDP_PACKETS, UDP_SIZE);
    sleep(65); // Waits longer than the server's udp socket timeout (= 60 secs)
    long result = receive_result(SERVER_IP, POST_TCP_PORT);
    judge_result(result);
}
