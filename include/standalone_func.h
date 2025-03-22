#include "client_func.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <threads.h>
#include <cjson/cJSON.h>
#include <fcntl.h>

#define __USE_BSD	/* use bsd'ish ip header */
#include <sys/socket.h>	/* these headers are for a Linux system, but */
#include <netinet/in.h>	/* the names on other systems are easy to guess.. */
#include <netinet/ip.h>
#define __FAVOR_BSD	/* use bsd'ish tcp header */
#include <netinet/tcp.h>
#include <unistd.h>

unsigned short csum (unsigned short*, int);

struct ipheader* populate_ipheader(char[], const char*, struct sockaddr_in*, const unsigned short int, const unsigned char);

struct tcpheader* populate_tcpheader(char[], const char*, struct sockaddr_in*, const unsigned short int, const unsigned char);

int open_tcp_raw_socket();

void send_syn_packet(const int sock, const struct sockaddr_in sin, const char* syn_packet, size_t packet_size);

void create_syn_packet(char* datagram, size_t datagram_size, struct sockaddr_in* sin);

int send_syn_and_train(void*);

long receive_rsts(const int sock);

int listen_to_rsts(void*);

void create_raw_packet_trains(const char* client_ip, struct sockaddr_in* server_addr, const int packet_size, const int num_of_packets, char zero_packets[][packet_size], char random_packets[][packet_size]);

void send_raw_packet_train(struct sockaddr_in*, const int packet_size, const int, char train[][packet_size]);