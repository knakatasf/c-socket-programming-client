#include "../include/client_func.h"
#include "../include/config.h"

void send_json(const char* config_file, const char* server_ip, const int server_port)
{
    FILE* file = fopen(config_file, "r");
    if (!file)
    {
        perror("Failed to open config file");
        exit(EXIT_FAILURE);
    }
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char* json_buffer = (char*)malloc(file_size + 1);
    fread(json_buffer, 1, file_size, file);
    fclose(file);
    json_buffer[file_size] = '\0';

    int sock = open_tcp_socket(server_ip, server_port);
    if (send(sock, json_buffer, file_size, 0) < 0) // Sends the json data to the server.
        perror("Failed to send config data");
    else
        printf("Suceeded in sending config data\n");

    close(sock);
    free(json_buffer);
}

int open_tcp_socket(const char* server_ip, const int server_port)
{
    int sock = socket(PF_INET, SOCK_STREAM, 0); // Socket for the tcp protocol.
    struct sockaddr_in sin;
    in_addr_t server_addr = inet_addr(server_ip);

    memset(&sin, 0, sizeof(sin)); // Sets everything zeros.
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = server_addr;
    sin.sin_port = htons(server_port);

    if (connect(sock, (struct sockaddr*)&sin, sizeof(sin)) < 0) // Opens the socket for the server.
    {
        perror("Couldn't connect to the server");
        close(sock);
        exit(EXIT_FAILURE);
    }
    return sock;
}

void send_packet_trains(const char* client_ip, const int client_port, const char* server_ip, const int server_port, const int num_of_packets, const int packet_size)
{
    struct sockaddr_in client_addr, server_addr;
    int df_flag = IP_PMTUDISC_DO;

    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = inet_addr(client_ip);
    client_addr.sin_port = htons(client_port);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    int sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (setsockopt(sock, IPPROTO_IP, IP_MTU_DISCOVER, &df_flag, sizeof(df_flag)) < 0) { // Don't fragment bit is set.
        perror("Failed to set Don't Fragment flag.\n");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (bind(sock, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0) // Specifies the client port.
    {
        perror("Bind failed.\n");
        close(sock);
        exit(EXIT_FAILURE);
    }

    char (*zero_packets)[packet_size] = malloc(num_of_packets * packet_size);  // Creates an empty train for the zero-packet train.
    char (*random_packets)[packet_size] = malloc(num_of_packets * packet_size); // Creates an empty train for the random-packet train.
    if (!zero_packets || !random_packets) {
        perror("Memory allocation failed");
        close(sock);
        free(zero_packets);
        free(random_packets);
        exit(EXIT_FAILURE);
    }

    create_trains(num_of_packets, packet_size, zero_packets, random_packets);
    send_udp_train(sock, &server_addr, num_of_packets, packet_size, zero_packets); // Sends the zero-packet train.
    printf("Sent zero-packet train.\n");

    usleep(INTER_MEASUREMENT_TIME);
    
    send_udp_train(sock, &server_addr, num_of_packets, packet_size, random_packets); // Sends the random-packet train.
    printf("Sent random-packet train.\n");

    close(sock);
    free(zero_packets);
    free(random_packets);
}

/*
 *  Creates a zero-packet train and random-packet train.
 *  Pass two empty trains and populate data: ID and zeros or random number.
 *  num_of_packets: The number of packets in each train.
 *  packet_size: The size of the packet.
 */
void create_trains(const int num_of_packets, const int packet_size, char zero_packets[][packet_size], char random_packets[][packet_size])
{   
    int urandom_fd = open("/dev/urandom", O_RDONLY);
    if (urandom_fd < 0)
    {
        perror("Failed to open /dev/urandom");
        return;
    }

    for (int i = 0; i < num_of_packets; i++)
    {
        memset(zero_packets[i], 0, packet_size); // Populates with zeros
        zero_packets[i][0] = (i >> 8) & 0xFF; // Overwrites the head of the packet with ID number.
        zero_packets[i][1] = i & 0xFF;

        read(urandom_fd, random_packets[i], packet_size); // Populates with random number
        random_packets[i][0] = (i >> 8) & 0xFF;
        random_packets[i][1] = i & 0xFF;
    }
    close(urandom_fd);
    printf("Creation of packet trains finished.\n");
}

void send_udp_train(const int sock, const struct sockaddr_in* server_addr, const int num_of_packets, const int packet_size, const char udp_train[][packet_size])
{
    for (int i = 0; i < num_of_packets; i++)
    {
        sendto(sock, udp_train[i], packet_size, 0, (struct sockaddr*)server_addr, sizeof(*server_addr));
    }
}

long receive_result(const char* server_ip, const int server_port)
{
    int sock = open_tcp_socket(server_ip, server_port);
    long result;
    ssize_t recv_size = recv(sock, &result, sizeof(long), 0); // Receives the result, which is long data type.
    if (recv_size < 0)
    {
        perror("Error receiving the result.\n");
        close(sock);
        return -1;
    }
    close(sock);
    return result;
}

void judge_result(const long result)
{
    if (result < 0)
        printf("Invalid result input.\n");
    else if (result <= THRESHOLD)
        printf("No comporesion was detected.\n");
    else
        printf("Compression detected!");
}