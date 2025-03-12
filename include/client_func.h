#ifndef PROJECT1_CLIENT_FUNC_H
#define PROJECT1_CLIENT_FUNC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cjson/cJSON.h>
#include <fcntl.h>

void send_json(const char*, const char*, const int);

int open_tcp_socket(const char*, const int);

void send_packet_trains(const char*, const int, const char*, const int, const int, const int);

void create_trains(const int, const int packet_size, char[][packet_size], char[][packet_size]);

void send_udp_train(const int, const struct sockaddr_in*, const int, const int packet_size, const char udp_train[][packet_size]);

long receive_result(const char*, const int);

void judge_result(const long);

#endif