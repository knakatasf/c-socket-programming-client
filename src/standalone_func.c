#include "../include/config.h"
#include "../include/standalone_func.h"
#include "../include/headers.h"

/* this function generates header checksums */
unsigned short csum(unsigned short* buf, int nwords)
{
  unsigned long sum;
  for (sum = 0; nwords > 0; nwords--)
    sum += *buf++;
  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  return ~sum;
}

struct ipheader* populate_ipheader(char datagram[], const char* client_ip, struct sockaddr_in* server_addr, const unsigned short int payload_size, const unsigned char protocol_type)
{
    struct ipheader* iph = (struct ipheader*)datagram;

    iph->ip_hl = 5;
    iph->ip_v = 4;
    iph->ip_tos = 0;
    iph->ip_len = htons(sizeof(struct ipheader) + sizeof(struct tcpheader) + payload_size);
    iph->ip_id = htons(54321);
    iph->ip_off = 0;
    iph->ip_ttl = TTL;
    iph->ip_p = protocol_type;
    iph->ip_sum = 0;
    iph->ip_src = inet_addr(CLIENT_IP); // My ip address
    iph->ip_dst = server_addr->sin_addr.s_addr; // The server ip

    iph->ip_sum = csum((unsigned short*)datagram, sizeof(struct ipheader) >> 1);

    return iph;
}

struct tcpheader* populate_tcpheader(char datagram[], const char* client_ip, struct sockaddr_in* server_addr, const unsigned char flag)
{
    struct tcpheader* tcph = (struct tcpheader*)(datagram + sizeof(struct ipheader));

    tcph->th_sport = htons(54321);
    tcph->th_dport = server_addr->sin_port;
    tcph->th_seq = htonl(0);
    tcph->th_ack = 0;
    tcph->th_x2 = 0;
    tcph->th_off = 5; // 20 bytes 
    tcph->th_flags = flag; // SYN flag;
    tcph->th_win = htons(8192);
    tcph->th_sum = 0;
    tcph->th_urp = 0;

    struct pseudo_tcp_header psh;
    psh.source_address = inet_addr(client_ip);
    psh.dest_address = server_addr->sin_addr.s_addr;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_TCP;
    psh.tcp_length = htons(sizeof(struct tcpheader));
    
    memcpy(&psh.tcp, tcph, sizeof(struct tcpheader));
    
    tcph->th_sum = csum((unsigned short*)&psh, sizeof(struct pseudo_tcp_header) >> 1);

    return tcph;
}

struct udpheader* populate_udpheader(char datagram[], const char* client_ip, struct sockaddr_in* server_addr, const unsigned short int payload_size)
{
    struct udpheader* udph = (struct udpheader*)(datagram + sizeof(struct ipheader));

    udph->uh_sport = htons(65432);
    udph->uh_dport = server_addr->sin_port;
    udph->uh_len = htons(sizeof(struct udpheader) + payload_size);
    udph->uh_check = 0;

    struct pseudo_udp_header psh;
    psh.source_address = inet_addr(client_ip);
    psh.dest_address = server_addr->sin_addr.s_addr;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_UDP;
    psh.udp_length = htons(sizeof(struct udpheader) + payload_size);

    memcpy(&psh.udp, udph, sizeof(struct udpheader));

    udph->uh_check = csum((unsigned short*)&psh, sizeof(struct pseudo_udp_header) >> 1);
    return udph;
}

const int open_tcp_raw_socket(const int listening_timeout)
{
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sock < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int one = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0)
    {
        perror("Cannot set HDRINCL");
        close(sock);
        exit(EXIT_FAILURE);
    }

    struct timeval timeout;
    timeout.tv_sec = (listening_timeout / 1000000) - 5;
    timeout.tv_usec = 0;

    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Failed to set socket timeout");
        close(sock);
        exit(EXIT_FAILURE);
    }
    return sock;
}

void create_syn_packet(char* datagram, size_t datagram_size, struct sockaddr_in* sin)
{
    memset(datagram, 0, datagram_size);

    struct ipheader* iph = populate_ipheader(datagram, CLIENT_IP, sin, 0, IPPROTO_TCP);
    struct tcpheader* tcph = populate_tcpheader(datagram, CLIENT_IP, sin, TH_SYN);
}

void send_syn_packet(const int sock, const struct sockaddr_in sin, const char* syn_packet, size_t packet_size)
{
    if (sendto(sock, syn_packet, packet_size, 0, (struct sockaddr*)&sin, sizeof(sin)) < 0)
    {
        perror("Failed to send SYN packet.\n");
        close(sock);
    }
    printf("Sent a SYN packet.\n");
}

int send_syn_and_train(void* arg)
{
    int tcp_socket = open_tcp_raw_socket(INTER_MEASUREMENT_TIME);

    in_addr_t server_addr = inet_addr(SERVER_IP);
    struct sockaddr_in pre_sin, udp_sin, post_sin;

    memset(&pre_sin, 0, sizeof(pre_sin)); // Sets everything zeros.
    pre_sin.sin_family = AF_INET;
    pre_sin.sin_addr.s_addr = server_addr;
    pre_sin.sin_port = htons(UNCORP_TCP_HEAD);

    memset(&udp_sin, 0, sizeof(udp_sin)); // Sets everything zeros.
    udp_sin.sin_family = AF_INET;
    udp_sin.sin_addr.s_addr = server_addr;
    udp_sin.sin_port = htons(7777);

    memset(&post_sin, 0, sizeof(post_sin)); // Sets everything zeros.
    post_sin.sin_family = AF_INET;
    post_sin.sin_addr.s_addr = server_addr;
    post_sin.sin_port = htons(UNCORP_TCP_TAIL);

    char pre_syn_packet[sizeof(struct ipheader) + sizeof(struct tcpheader)];
    create_syn_packet(pre_syn_packet, sizeof(pre_syn_packet), &pre_sin);

    char post_syn_packet[sizeof(struct ipheader) + sizeof(struct tcpheader)];
    create_syn_packet(post_syn_packet, sizeof(post_syn_packet), &post_sin);

    int packet_size = UDP_SIZE + sizeof(struct ipheader) + sizeof(struct udpheader);
    char (*zero_packets)[packet_size] = malloc(NUM_OF_UDP_PACKETS * packet_size);  // Creates an empty train for the zero-packet train.
    char (*random_packets)[packet_size] = malloc(NUM_OF_UDP_PACKETS * packet_size); // Creates an empty train for the random-packet train.
    if (!zero_packets || !random_packets)
    {
        perror("Memory allocation for raw packet trains failed");
        free(zero_packets);
        free(random_packets);
        return thrd_error;
    }
    create_raw_packet_trains(CLIENT_IP, &udp_sin, packet_size, NUM_OF_UDP_PACKETS, zero_packets, random_packets);

    send_syn_packet(tcp_socket, pre_sin, pre_syn_packet, sizeof(pre_syn_packet));
    send_raw_packet_train(&udp_sin, packet_size, NUM_OF_UDP_PACKETS, zero_packets);
    send_syn_packet(tcp_socket, post_sin, post_syn_packet, sizeof(post_syn_packet));

    usleep(INTER_MEASUREMENT_TIME);

    send_syn_packet(tcp_socket, pre_sin, pre_syn_packet, sizeof(pre_syn_packet));
    send_raw_packet_train(&udp_sin, packet_size, NUM_OF_UDP_PACKETS, random_packets);
    send_syn_packet(tcp_socket, post_sin, post_syn_packet, sizeof(post_syn_packet));

    free(zero_packets);
    free(random_packets);

    close(tcp_socket);

    return thrd_success;
}

long receive_rsts(const int sock)
{
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    char buffer[1024];
    struct timeval received_time[2];

    struct ipheader* recv_iph;
    struct tcpheader* recv_tcph;

    int rst_count = 0;
    long train_duration = -1;

    unsigned server_ip_bin = inet_addr(SERVER_IP);

    while (rst_count < 2)
    {
        int bytes_received = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&server_addr, &addr_len);
        if (bytes_received < 0) 
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                printf("Timeout waiting for RST packet.\n");
            else
                perror("recvfrom() failed");
            return -1;
        }

        gettimeofday(&received_time[rst_count], NULL);

        recv_iph = (struct ipheader*)buffer;
        recv_tcph = (struct tcpheader*)(buffer + sizeof(struct ipheader));

        if (server_ip_bin != recv_iph->ip_src)
        {
            printf("Discarded a packet from unknown IP. Continue listening..\n");
            continue;
        }

        if (recv_tcph->th_flags & TH_RST)
        {
            rst_count++;
            printf("Received an RST packet from the server.\n");
        }
        else
            printf("Received a non-RST packet. Continue listening..\n");
    }

    train_duration = (received_time[1].tv_sec - received_time[0].tv_sec) * 1000 +
                     (received_time[1].tv_usec - received_time[0].tv_usec) / 1000;
    
    printf("Train reception duration: %ld ms.\n", train_duration);
    return train_duration;
}


int listen_to_rsts(void* arg)
{
    int tcp_socket = open_tcp_raw_socket(INTER_MEASUREMENT_TIME);

    long first_train_duration = receive_rsts(tcp_socket);

    usleep(INTER_MEASUREMENT_TIME - 5000000);

    long second_train_duration = receive_rsts(tcp_socket);

    if (first_train_duration > 0 && second_train_duration > 0)
    {
        printf("Succeeded in receiving four RSTs properly. The difference is: %ld.\n", labs(first_train_duration - second_train_duration));
        return thrd_success;
    }
    printf("Failed to detect due to insufficient information.");
    return thrd_error;
}

void create_raw_packet_trains(const char* client_ip, struct sockaddr_in* server_addr, const int packet_size, const int num_of_packets, char zero_packets[][packet_size], char random_packets[][packet_size])
{
    int urandom_fd = open("/dev/urandom", O_RDONLY);
    if (urandom_fd < 0)
    {
        perror("Failed to open /dev/urandom");
        return;
    }

    int payload_start = sizeof(struct ipheader) + sizeof(struct udpheader);
    int payload_size = packet_size - payload_start;
    for (int i = 0; i < num_of_packets; i++)
    {
        memset(zero_packets[i], 0, packet_size); // Populates with zeros
        zero_packets[i][payload_start] = (i >> 8) & 0xFF; // Overwrites the head of the packet with ID number.
        zero_packets[i][payload_start + 1] = i & 0xFF;
        populate_ipheader(zero_packets[i], client_ip, server_addr, payload_size, IPPROTO_UDP);
        populate_udpheader(zero_packets[i], client_ip, server_addr, payload_size);

        read(urandom_fd, random_packets[i], packet_size); // Populates with random number
        random_packets[i][payload_start] = (i >> 8) & 0xFF;
        random_packets[i][payload_start + 1] = i & 0xFF;
        memset(random_packets[i], 0, payload_start);
        populate_ipheader(random_packets[i], client_ip, server_addr, payload_size, IPPROTO_UDP);
        populate_udpheader(random_packets[i], client_ip, server_addr, payload_size);
    }
    close(urandom_fd);
    printf("Creation of raw packet trains finished.\n");
}

void send_raw_packet_train(struct sockaddr_in* server_addr, const int packet_size, const int num_of_packets, char train[][packet_size])
{
    int df_flag = IP_PMTUDISC_DO;
    int sock = socket(PF_INET, SOCK_RAW, IPPROTO_UDP);
    if (sock < 0)
    {
        perror("Socket creation failed.\n");
        exit(EXIT_FAILURE);
    }

    int one = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)))
    {
        perror("Couldn't set IP_HDRINCL");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (setsockopt(sock, IPPROTO_IP, IP_MTU_DISCOVER, &df_flag, sizeof(df_flag)) < 0) { // Don't fragment bit is set.
        perror("Failed to set Don't Fragment flag.\n");
        close(sock);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_of_packets; i++)
    {
        if (sendto(sock, train[i], ntohs(packet_size), 0, (struct sockaddr*)server_addr, sizeof(*server_addr)) < 0) {
            perror("Failed to send packet");
        } else {
            //printf("Raw UDP packet sent successfully!\n");
        }
    }
    close(sock);
}

