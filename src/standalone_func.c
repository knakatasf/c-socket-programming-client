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

struct timeval send_syn(const char* server_ip, const int server_port)
{
    struct timeval error_time = {0, 0};
    int sock = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sock < 0)
    {
        perror("Socket creation failed");
        return error_time;
    }

    int one = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0)
    {
        perror("Cannot set HDRINCL");
        close(sock);
        return error_time;
    }

    char datagram[60];
    memset(datagram, 0, sizeof(datagram));
    
    struct sockaddr_in sin; // Populate with destionation address and port.
    in_addr_t server_addr = inet_addr(server_ip);

    memset(&sin, 0, sizeof(sin)); // Sets everything zeros.
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = server_addr;
    sin.sin_port = htons(server_port);

    struct ipheader* iph = populate_ipheader(datagram, CLIENT_IP, &sin, 0, IPPROTO_TCP);
    struct tcpheader* tcph = populate_tcpheader(datagram, CLIENT_IP, &sin, TH_SYN);

    if (sendto(sock, datagram, ntohs(iph->ip_len), 0, (struct sockaddr*)&sin, sizeof(sin)) < 0)
    {
        perror("Failed to send SYN packet.\n");
        close(sock);
        return error_time;
    }
    printf("Sent a syn packet.\n");

    struct timeval received_time = receive_rst(sock);
    close(sock);
    return received_time;
}

struct timeval receive_rst(const int sock)
{
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    char buffer[1024];

    struct timeval received_time;

    int bytes_received = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&server_addr, &addr_len);
    gettimeofday(&received_time, NULL);

    if (bytes_received > 0)
    {
        struct ipheader* recv_iph = (struct ipheader*)buffer;
        struct tcpheader* recv_tcph = (struct tcpheader*)(buffer + sizeof(struct ipheader));

        if (recv_tcph->th_flags & TH_RST)
        {
            printf("Received RST pakcet at: %ld.%06ld seconds.\n", received_time.tv_sec, received_time.tv_usec);
            return received_time;
        }
    }
    struct timeval error_time = {0, 0};
    return error_time;
}

void create_raw_packet_trains(const char* client_ip, struct sockaddr_in* server_addr, const int packet_size, const int num_of_packets, char zero_packets[][packet_size], char random_packets[][packet_size])
{
//    int packet_size = payload_size + sizeof(struct ipheader) + sizeof(struct udpheader);
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
            printf("Raw UDP packet sent successfully!\n");
        }
    }    
}

long measure_time_diff(const char* client_ip, const char* server_ip, const int server_port_head, const int server_port_tail, const int num_of_packets, const int payload_size)
{
    int packet_size = payload_size + sizeof(struct ipheader) + sizeof(struct udpheader);
    char (*zero_packets)[packet_size] = malloc(num_of_packets * packet_size);  // Creates an empty train for the zero-packet train.
    char (*random_packets)[packet_size] = malloc(num_of_packets * packet_size); // Creates an empty train for the random-packet train.
    if (!zero_packets || !random_packets)
    {
        perror("Memory allocation for raw packet trains failed");
        free(zero_packets);
        free(random_packets);
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in server_addr_in_udp;
    in_addr_t server_addr = inet_addr(server_ip);
    memset(&server_addr_in_udp, 0, sizeof(server_addr_in_udp)); // Sets everything zeros.
    server_addr_in_udp.sin_family = AF_INET;
    server_addr_in_udp.sin_addr.s_addr = server_addr;
    server_addr_in_udp.sin_port = htons(7777);
    create_raw_packet_trains(client_ip, &server_addr_in_udp, packet_size, num_of_packets, zero_packets, random_packets);

    struct timeval zero_head_time = send_syn(server_ip, server_port_head);
    send_raw_packet_train(&server_addr_in_udp, packet_size, num_of_packets, zero_packets);
    struct timeval zero_tail_time = send_syn(server_ip, server_port_tail);

    long zero_train_duration = (zero_tail_time.tv_sec - zero_head_time.tv_sec) * 1000 + 
                               (zero_tail_time.tv_usec - zero_head_time.tv_usec) / 1000;
    printf("Zero-packet train duration: %ld ms.\n", zero_train_duration);

    usleep(INTER_MEASUREMENT_TIME);

    struct timeval random_head_time = send_syn(server_ip, server_port_head);
    send_raw_packet_train(&server_addr_in_udp, packet_size, num_of_packets, random_packets);
    struct timeval random_tail_time = send_syn(server_ip, server_port_tail);

    long random_train_duration = (random_tail_time.tv_sec - random_head_time.tv_sec) * 1000 + 
                                 (random_tail_time.tv_usec - random_head_time.tv_usec) / 1000;
    printf("Randome-packet train duration: %ld ms.\n", random_train_duration);

    return labs(zero_train_duration - random_train_duration);
}

