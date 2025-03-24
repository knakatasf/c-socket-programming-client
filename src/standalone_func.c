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

/* Pass the datagram and this function populates the head of the datagram with formatted IP header. */
struct ipheader* populate_ipheader(char datagram[], const char* client_ip, struct sockaddr_in* server_addr,
                                   const unsigned short int payload_size, const unsigned char protocol_type)
{
    /* The pointer of ipheader points to the head of the datagram */
    struct ipheader* iph = (struct ipheader*)datagram;

    iph->ip_hl = 5;
    iph->ip_v = 4; // IPv4
    iph->ip_tos = 0;
    iph->ip_len = htons(sizeof(struct ipheader) + sizeof(struct tcpheader) + payload_size); // The total size of the packet
    iph->ip_id = htons(54321);
    iph->ip_off = htons(IP_DF);
    iph->ip_ttl = TTL;
    iph->ip_p = protocol_type; // The protocol follows this ipheader (udp or tcp)
    iph->ip_sum = 0;
    iph->ip_src = inet_addr(CLIENT_IP); // The client's ip address
    iph->ip_dst = server_addr->sin_addr.s_addr; // The server ip

    /* Calculates the checksum and populates it with ip_sum */
    iph->ip_sum = csum((unsigned short*)datagram, sizeof(struct ipheader) >> 1);

    return iph;
}

/* Pass the datagram and this function populates it with formatted TCP header. */
struct tcpheader* populate_tcpheader(char datagram[], const char* client_ip,
                                     struct sockaddr_in* server_addr, 
                                     const unsigned short int payload_size,
                                     const unsigned char flag)
{
    /* The pointer of tcpheader points to the next to ipheader */
    struct tcpheader* tcph = (struct tcpheader*)(datagram + sizeof(struct ipheader));

    tcph->th_sport = htons(54321); // Placeholder
    tcph->th_dport = server_addr->sin_port; // Server's port
    tcph->th_seq = htonl(0);
    tcph->th_ack = 0;
    tcph->th_x2 = 0;
    tcph->th_off = 5; // 20 bytes 
    tcph->th_flags = flag; // Sets flags;
    tcph->th_win = htons(8192);
    tcph->th_sum = 0;
    tcph->th_urp = 0;

    /* pseudo_header_size is: source ip + dest ip + placeholder + protocol + tcp length */
    int pseudo_header_size = sizeof(struct pseudo_tcp_header) - sizeof(struct tcpheader);
    /* total_size needs to include tcpheader's size + payload's size */
    int total_size = pseudo_header_size + sizeof(struct tcpheader) + payload_size;

    /* Creates a temporary datagram including everything to caluculate tcp checksum */
    char* pseudogram = malloc(total_size);
    if (!pseudogram) {
        perror("Memory allocation failed for TCP checksum");
        return tcph;
    }

    /* Populates pseudo tcp header with data that is NOT included in actual tcp header */
    struct pseudo_tcp_header* psh = (struct pseudo_tcp_header*)pseudogram;
    psh->source_address = inet_addr(client_ip); // Client's ip address
    psh->dest_address = server_addr->sin_addr.s_addr; // Server's ip address
    psh->placeholder = 0;
    psh->protocol = IPPROTO_TCP;
    psh->tcp_length = htons(sizeof(struct tcpheader));
    
    /* Copies the actual tcp header to the pseudogram; actual tcp header will start from pseudogram + pseudo_header_size */
    memcpy(pseudogram + pseudo_header_size, tcph, sizeof(struct tcpheader));

    /* Copies the payload to the pseudogram if any */
    if (payload_size > 0) {
        memcpy(pseudogram + pseudo_header_size + sizeof(struct tcpheader),
               datagram + sizeof(struct ipheader) + sizeof(struct tcpheader),
               payload_size);
    }

    /* Calculates checksum over the entire pseudogram */
    tcph->th_sum = csum((unsigned short*)pseudogram, total_size >> 1);

    /* If the size is odd, we need to pad with a zero byte for checksum calculation */
    if (total_size % 2 != 0) {
        pseudogram[total_size] = 0;
        tcph->th_sum = csum((unsigned short*)pseudogram, (total_size + 1) >> 1);
    }

    free(pseudogram);
    return tcph;
}

/* Pass the datagram and this function populates it with formatted UDP header. */
struct udpheader* populate_udpheader(char datagram[], const char* client_ip, struct sockaddr_in* server_addr,
                                     const unsigned short int payload_size)
{
    /* The pointer of udpheader points to the next to ipheader */
    struct udpheader* udph = (struct udpheader*)(datagram + sizeof(struct ipheader));

    udph->uh_sport = htons(65432); // Placeholder
    udph->uh_dport = server_addr->sin_port; // The server's port
    udph->uh_len = htons(sizeof(struct udpheader) + payload_size); // The udp packet size = udp header + payload
    udph->uh_check = 0;

    /* Calculate the udp checksum including the payload */
    int total_size = sizeof(struct pseudo_udp_header) + payload_size;
    char *pseudogram = malloc(total_size);
    if (!pseudogram) {
        perror("Memory allocation failed for UDP checksum");
        return udph;
    }

    /* Populates pseudo_udp_header with the fields that in NOT included in actual udp header */
    struct pseudo_udp_header* psh = (struct pseudo_udp_header*)pseudogram;
    psh->source_address = inet_addr(client_ip); // Client's ip address
    psh->dest_address = server_addr->sin_addr.s_addr; // Server's ip address
    psh->placeholder = 0;
    psh->protocol = IPPROTO_UDP;
    psh->udp_length = htons(sizeof(struct udpheader) + payload_size);

    /* Copies udp header to pseudogram */
    memcpy(pseudogram + sizeof(struct pseudo_udp_header) - sizeof(struct udpheader), 
           udph, sizeof(struct udpheader));

    /* Copies the payload to pseudogram */
    memcpy(pseudogram + sizeof(struct pseudo_udp_header), 
           datagram + sizeof(struct ipheader) + sizeof(struct udpheader), 
           payload_size);

     /* Calculate checksum over the entire pseudogram */
     udph->uh_check = csum((unsigned short*)pseudogram, total_size >> 1);
    
     /* If the size is odd, we need to pad with a zero byte */
     if (total_size % 2 != 0) {
         pseudogram[total_size] = 0;
         udph->uh_check = csum((unsigned short*)pseudogram, (total_size + 1) >> 1);
     }
 
     free(pseudogram);
     return udph;
}

/*
 * Opens a tcp raw socket for sending a SYN and receiving an RST.
 * Need to setup the timeout since it would listen indefinitely for the RST packet
 */
int open_tcp_raw_socket(const int listening_timeout)
{
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sock < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    /* This tells Kernel that you manually write the ip header instead of Kernel */
    int one = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0)
    {
        perror("Cannot set HDRINCL");
        close(sock);
        exit(EXIT_FAILURE);
    }

    /* Set the timeout for the socket */
    struct timeval timeout;
    timeout.tv_sec = listening_timeout;
    timeout.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Failed to set socket timeout");
        close(sock);
        exit(EXIT_FAILURE);
    }

    return sock;
}

/* Creates a SYN packet, whose payload is empty */
void create_syn_packet(char* datagram, size_t datagram_size, struct sockaddr_in* sin)
{
    memset(datagram, 0, datagram_size);

    /* The payload is 0 */
    struct ipheader* iph = populate_ipheader(datagram, CLIENT_IP, sin, 0, IPPROTO_TCP);
    struct tcpheader* tcph = populate_tcpheader(datagram, CLIENT_IP, sin, 0, TH_SYN);
}

/* 
 * Pass the socket, the address of the server, and the packet itself
 * This function can send any data with tcp header.
 */
void send_syn_packet(const int sock, const struct sockaddr_in sin,
                     const char* syn_packet, size_t packet_size)
{
    if (sendto(sock, syn_packet, packet_size, 0, (struct sockaddr*)&sin, sizeof(sin)) < 0)
    {
        perror("Failed to send SYN packet.\n");
        close(sock);
    }
}

/* This function is for the sender thread, which takes no arguments */
int send_syn_and_train(void* arg)
{
    /* Before sending the SYN and packet trains, prepare everything */
    int tcp_socket = open_tcp_raw_socket(60); // 60 secs is reasonable for timeout of RST listener socket 

    /* Prepare the server's ip address and ports */
    in_addr_t server_addr = inet_addr(SERVER_IP);
    struct sockaddr_in pre_sin, udp_sin, post_sin;

    memset(&pre_sin, 0, sizeof(pre_sin));
    pre_sin.sin_family = AF_INET;
    pre_sin.sin_addr.s_addr = server_addr;
    pre_sin.sin_port = htons(UNCORP_TCP_HEAD); // Port for head SYN packet 

    memset(&udp_sin, 0, sizeof(udp_sin));
    udp_sin.sin_family = AF_INET;
    udp_sin.sin_addr.s_addr = server_addr;
    udp_sin.sin_port = htons(7777); // Port for the udp packet trains

    memset(&post_sin, 0, sizeof(post_sin));
    post_sin.sin_family = AF_INET;
    post_sin.sin_addr.s_addr = server_addr;
    post_sin.sin_port = htons(UNCORP_TCP_TAIL); // Port for tail SYN packet

    /* Prepares the head and tail SYN packets in advance */
    char pre_syn_packet[sizeof(struct ipheader) + sizeof(struct tcpheader)];
    create_syn_packet(pre_syn_packet, sizeof(pre_syn_packet), &pre_sin);
    char post_syn_packet[sizeof(struct ipheader) + sizeof(struct tcpheader)];
    create_syn_packet(post_syn_packet, sizeof(post_syn_packet), &post_sin);

    /* Prepare zero-packet and random-packet trains in advance */
    int packet_size = UDP_SIZE + sizeof(struct ipheader) + sizeof(struct udpheader);
    char (*zero_packets)[packet_size] = malloc(NUM_OF_UDP_PACKETS * packet_size); 
    char (*random_packets)[packet_size] = malloc(NUM_OF_UDP_PACKETS * packet_size);
    if (!zero_packets || !random_packets)
    {
        perror("Memory allocation for raw packet trains failed");
        free(zero_packets);
        free(random_packets);
        return thrd_error;
    }
    /* Populates the empty trains with the ip and udp headers and payload */
    create_raw_packet_trains(CLIENT_IP, &udp_sin, packet_size, NUM_OF_UDP_PACKETS, 
                             zero_packets, random_packets);

    /* Sends the head SYN packet, zero-packet train and tail SYN packet */
    send_syn_packet(tcp_socket, pre_sin, pre_syn_packet, sizeof(pre_syn_packet));
    send_raw_packet_train(&udp_sin, packet_size, NUM_OF_UDP_PACKETS, zero_packets);
    send_syn_packet(tcp_socket, post_sin, post_syn_packet, sizeof(post_syn_packet));

    sleep(INTER_MEASUREMENT_TIME);
    
    /* Sends the head SYN packet, random-packet train and tail SYN packet */
    send_syn_packet(tcp_socket, pre_sin, pre_syn_packet, sizeof(pre_syn_packet));
    send_raw_packet_train(&udp_sin, packet_size, NUM_OF_UDP_PACKETS, random_packets);
    send_syn_packet(tcp_socket, post_sin, post_syn_packet, sizeof(post_syn_packet));

    free(zero_packets);
    free(random_packets);

    close(tcp_socket);

    return thrd_success;
}

/* This function receives 2 RST packets from the server */
long receive_rsts(const int sock)
{
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    char buffer[1024];
    struct timeval received_time[2]; // Expects 2 RST packets' arrival time

    struct ipheader* recv_iph;
    struct tcpheader* recv_tcph;

    int rst_count = 0;
    long train_duration = -1;

    unsigned server_ip_bin = inet_addr(SERVER_IP); // The server's ip address

    while (rst_count < 2) // Until it receives 2 RST packets
    {
        int bytes_received = recvfrom(sock, buffer, sizeof(buffer), 0, 
                                     (struct sockaddr*)&server_addr, &addr_len);
        if (bytes_received < 0) // If timeout
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                printf("Timeout waiting for RST packet.\n");
            else
                perror("recvfrom() failed");
            return -1; // Breaks the while loop
        }
        gettimeofday(&received_time[rst_count], NULL); // Received a packet

        recv_iph = (struct ipheader*)buffer;
        recv_tcph = (struct tcpheader*)(buffer + sizeof(struct ipheader));

        if (server_ip_bin != recv_iph->ip_src) // If the packet didn't come from the server
            continue; // Do nothing, continue listening

        if (recv_tcph->th_flags & TH_RST) // If the packet is an RST
            rst_count++; // Increment the count of the RSTs
    }

    /* Get the time difference between the first and second arrival times */
    train_duration = (received_time[1].tv_sec - received_time[0].tv_sec) * 1000 +
                     (received_time[1].tv_usec - received_time[0].tv_usec) / 1000;
    
    // printf("Train reception duration: %ld ms.\n", train_duration);
    return train_duration;
}

/* This function is for the listener thread, which takes no arguments */
int listen_to_rsts(void* arg)
{
    int tcp_socket = open_tcp_raw_socket(TIME_OUT_FOR_SOCKET);

    /* Returns the duration of the first train */
    long first_train_duration = receive_rsts(tcp_socket);

    sleep(INTER_MEASUREMENT_TIME * 2 / 3);

    /* Return the duration of the second train */
    long second_train_duration = receive_rsts(tcp_socket);

    if (first_train_duration > 0 && second_train_duration > 0)
    {
        //printf("Succeeded in receiving four RSTs properly. The difference is: %ld.\n", labs(first_train_duration - second_train_duration));
        long result = labs(first_train_duration - second_train_duration);
        judge_result(result);
        return thrd_success;
    }
    printf("Failed to detect due to insufficient information.\n");
    return thrd_error;
}

/* 
 * Creates raw packet train with payload and ip & udp headers 
 * packet_size: the total sum of payload + ipheader + udpheader
 */
void create_raw_packet_trains(const char* client_ip, struct sockaddr_in* server_addr, 
                              const int packet_size, const int num_of_packets,
                              char zero_packets[][packet_size], char random_packets[][packet_size])
{
    int urandom_fd = open("/dev/urandom", O_RDONLY);
    if (urandom_fd < 0)
    {
        perror("Failed to open /dev/urandom");
        return;
    }

    /* Want to populate the payload skipping ipheader and udpheader */
    int payload_start = sizeof(struct ipheader) + sizeof(struct udpheader);
    int payload_size = packet_size - payload_start; // Subtracts the size of the ipheader and udpheader
    for (int i = 0; i < num_of_packets; i++)
    {
         // Populates with zeros and overwrites the head of the packet with headers and ID number
        memset(zero_packets[i], 0, packet_size);
        zero_packets[i][payload_start] = (i >> 8) & 0xFF; // Skips ipheader and udpheader
        zero_packets[i][payload_start + 1] = i & 0xFF; // Skips ipheader and udpheader
        populate_ipheader(zero_packets[i], client_ip, server_addr, payload_size, IPPROTO_UDP);
        populate_udpheader(zero_packets[i], client_ip, server_addr, payload_size);

        // Populate with random numbers and overwrites the head of the packet with headers and ID number
        read(urandom_fd, random_packets[i], packet_size);
        random_packets[i][payload_start] = (i >> 8) & 0xFF; // Skips ipheader and udpheader
        random_packets[i][payload_start + 1] = i & 0xFF; // Skips ipheader and udpheader
        memset(random_packets[i], 0, payload_start); // Initializes the header parts with zeros
        populate_ipheader(random_packets[i], client_ip, server_addr, payload_size, IPPROTO_UDP);
        populate_udpheader(random_packets[i], client_ip, server_addr, payload_size);
    }
    close(urandom_fd);
}

/* Opens a udp raw socket and send the packet train */
void send_raw_packet_train(struct sockaddr_in* server_addr, const int packet_size,
                           const int num_of_packets, char train[][packet_size])
{
    int df_flag = IP_PMTUDISC_DO; // Don't fragment flag
    int sock = socket(PF_INET, SOCK_RAW, IPPROTO_UDP);
    if (sock < 0)
    {
        perror("Socket creation failed.\n");
        exit(EXIT_FAILURE);
    }

    /* This tells Kernel that you will manually write ip header */
    int one = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)))
    {
        perror("Couldn't set IP_HDRINCL");
        close(sock);
        exit(EXIT_FAILURE);
    }

    /* Don't fragment flag is set */
    if (setsockopt(sock, IPPROTO_IP, IP_MTU_DISCOVER, &df_flag, sizeof(df_flag)) < 0)
    {
        perror("Failed to set Don't Fragment flag.\n");
        close(sock);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < num_of_packets; i++)
    {
        if (sendto(sock, train[i], ntohs(packet_size), 0, (struct sockaddr*)server_addr, sizeof(*server_addr)) < 0)
            perror("Failed to send packet");
    }
    close(sock);
}

