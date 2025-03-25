#ifndef CONFIG_H
#define CONFIG_H

#include <cjson/cJSON.h>

/*
 * Waiting time for the server to be ready for the post-probing phase.abort
 * Longer than 60 secs which is the udp socket's timout of the server.
 */
#define WAIT_TIME 65 
#define THRESHOLD 100 // 100 ms
#define TIME_OUT_FOR_SOCKET 60

#define IP_HEADER_ID 54321 // IP header's ID
#define TCP_WIN_SIZE 8192 // TCP's window size
#define TCP_PORT_STANDALONE 54321 // Client's port used for head and tail SYNs
#define UDP_PORT_STANDALONE 65432 // Client's port used for packet trains for standalone mode
#define SERVER_UDP_PORT_STANDALONE 7777 // Server's port used for standalone mode, which should be closed

#define APP_SOCK_BUFFER_SIZE 1024 // Socker buffer size at the application layer level

extern int PRE_TCP_PORT; // The server's port for pre-probing; default 7777
extern int POST_TCP_PORT; // The server's port for post-probing; default 6666
extern int CLIENT_UDP_PORT; // The client's port for probing; default 9876
extern int SERVER_UDP_PORT; // The server's port for probing; default 8765

extern int UNCORP_TCP_HEAD; // The server's port for the head SYN packet; default 9999
extern int UNCORP_TCP_TAIL; // The server's port for the tail SYN packet; default 8888

extern char CLIENT_IP[17]; // This machine's IP Address; "192.168.64.2"
extern char SERVER_IP[17]; // The server's IP address; "192.168.64.7"

extern int UDP_SIZE; // The payload size of the packet in the packet train; default 1000B
extern int INTER_MEASUREMENT_TIME; // 15 secs
extern int NUM_OF_UDP_PACKETS; // The number of the packets in the packet train; default 6000
extern int TTL; // The TTL for the udp packets in the train; default 255

void load_config(const char*);

#endif