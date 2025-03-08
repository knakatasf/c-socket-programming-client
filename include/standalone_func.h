#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <arpa/inet.h>
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

struct tcpheader* populate_tcpheader(char[], const char*, struct sockaddr_in*, const unsigned char);

struct timeval send_syn(const char*, const int);

struct timeval receive_rst(const int);

void send_raw_packet_train(struct sockaddr_in*, const int packet_size, const int, char train[][packet_size]);

long measure_time_diff(const char* client_ip, const char* server_ip, const int server_port_head, const int server_port_tail, const int num_of_packets, const int payload_size);

