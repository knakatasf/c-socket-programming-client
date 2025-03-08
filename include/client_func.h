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

void send_packet_train(const int client_port, const int server_port, const int num_of_packets, const int packet_size);

void create_trains(int num_of_packets, int packet_size, char zero_packets[][packet_size], char random_packets[][packet_size]);
void send_udp_train(int sock, const struct sockaddr_in* server_addr, int num_of_packets, int packet_size, char udp_train[][packet_size]);
long receive_result(const char* server_ip, const int server_port);

void judge_result(long);

#endif