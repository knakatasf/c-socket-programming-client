#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/config.h"

int PRE_TCP_PORT;
int POST_TCP_PORT;
int CLIENT_UDP_PORT;
int SERVER_UDP_PORT;

int UNCORP_TCP_HEAD;
int UNCORP_TCP_TAIL;

char SERVER_IP[17];

int UDP_SIZE;
int INTER_MEASUREMENT_TIME;
int NUM_OF_UDP_PACKETS;
int TTL;

/*
 * Opens a configuration json file and parses data for this application.
 * Loads the data to the global environment variables.
 * Exit failure if the json file doesn't follow the pre-defined format.
 */
void load_config(const char* filename)
{
    FILE* file = fopen(filename, "r");
    if (!file)
    {
        perror("Couldn't open config file");
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* data = (char*)malloc(length + 1);
    fread(data, 1, length, file);
    fclose(file);
    data[length] = '\0';

    cJSON* json = cJSON_Parse(data);
    if (!json)
    {
        printf("Error parsing JSON\n");
        free(data);
        exit(EXIT_FAILURE);
    }

    /* Assumes the json file include these keys */
    const cJSON* pre_tcp_port = cJSON_GetObjectItem(json, "pre_tcp_port");
    const cJSON* post_tcp_port = cJSON_GetObjectItem(json, "post_tcp_port");
    const cJSON* client_udp_port = cJSON_GetObjectItem(json, "client_udp_port");
    const cJSON* server_udp_port = cJSON_GetObjectItem(json, "server_udp_port");

    const cJSON* uncorp_tcp_head = cJSON_GetObjectItem(json, "uncorp_tcp_head");
    const cJSON* uncorp_tcp_tail = cJSON_GetObjectItem(json, "uncorp_tcp_tail");

    const cJSON* server_ip = cJSON_GetObjectItem(json, "server_ip");

    const cJSON* udp_size = cJSON_GetObjectItem(json, "udp_payload_size");
    const cJSON* inter_measurement_t = cJSON_GetObjectItem(json, "inter_measurement_time");
    const cJSON* num_of_udp_packets = cJSON_GetObjectItem(json, "num_of_udp_packets");
    const cJSON* ttl = cJSON_GetObjectItem(json, "ttl");

    /* Only when the json file has those keys and proper values, parses data */
    if (pre_tcp_port && cJSON_IsNumber(pre_tcp_port)
        && post_tcp_port && cJSON_IsNumber(post_tcp_port)
        && client_udp_port && cJSON_IsNumber(client_udp_port)
        && server_udp_port && cJSON_IsNumber(server_udp_port)
        && uncorp_tcp_head && cJSON_IsNumber(uncorp_tcp_head)
        && uncorp_tcp_tail && cJSON_IsNumber(uncorp_tcp_tail)
        && server_ip && cJSON_IsString(server_ip)
        && udp_size && cJSON_IsNumber(udp_size)
        && num_of_udp_packets && cJSON_IsNumber(num_of_udp_packets)
        && ttl && cJSON_IsNumber(ttl))
    {
        PRE_TCP_PORT = pre_tcp_port->valueint;
        POST_TCP_PORT = post_tcp_port->valueint;
        CLIENT_UDP_PORT = client_udp_port->valueint;
        SERVER_UDP_PORT = server_udp_port->valueint;

        UNCORP_TCP_HEAD = uncorp_tcp_head->valueint;
        UNCORP_TCP_TAIL = uncorp_tcp_tail->valueint;

        strcpy(SERVER_IP, server_ip->valuestring);
        
        UDP_SIZE = udp_size->valueint;
        INTER_MEASUREMENT_TIME = inter_measurement_t->valueint;
        NUM_OF_UDP_PACKETS = num_of_udp_packets->valueint;
        TTL = ttl->valueint;
    }
    else
    {
        cJSON_Delete(json);
        free(data);
        printf("The json file doesn't follow the pre-defined format.");
        exit(EXIT_FAILURE);
    }
    cJSON_Delete(json);
    free(data);
}