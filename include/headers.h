#ifndef HEADERS_H
#define HEADERS_H

#define TH_FIN 0x01
#define TH_SYN 0x02
#define TH_RST 0x04
#define TH_PSH 0x08
#define TH_ACK 0x10
#define TH_URG 0x20
#define TH_ECE 0x40
#define TH_CWR 0x80

struct tcpheader
{
    unsigned short int th_sport;    // 2 bytes Source Port
    unsigned short int th_dport;    // 2 bytes Destination Port
    unsigned int th_seq;            // 4 bytes Sequence Number
    unsigned int th_ack;            // 4 bytes Acknowledge Number
    unsigned char th_x2:4, th_off:4; // 1 byte  TCP Header Length, Unused 
    unsigned char th_flags;         // 1 byte  URG, ACK, PSH, RST, SYN, FIN
    unsigned short int th_win;      // 2 bytes Window Size
    unsigned short int th_sum;      // 2 bytes Checksum
    unsigned short int th_urp;      // 2 bytes Urgent Pointer
}; /* total tcp header length: 20 bytes */

struct pseudo_tcp_header
{
    unsigned int source_address;
    unsigned int dest_address;
    unsigned char placeholder;
    unsigned char protocol;
    unsigned short tcp_length;
    struct tcpheader tcp;
};

struct udpheader
{
    unsigned short int uh_sport;    // 2 bytes Source Port
    unsigned short int uh_dport;    // 2 bytes Destination Port
    unsigned short int uh_len;      // 2 bytes UDP Length
    unsigned short int uh_check;    // 2 bytes UDP Checksum
}; /* total udp header length: 8 bytes */

struct pseudo_udp_header
{
    unsigned int source_address;
    unsigned int dest_address;
    unsigned char placeholder;
    unsigned char protocol;
    unsigned short udp_length;
    struct udpheader udp;
};

struct ipheader
{
    unsigned char ip_hl:4, ip_v:4;  // 1 byte  IHL (IP Header Length), Version (4 or 6)
    unsigned char ip_tos;           // 1 byte  Differentiated Service
    unsigned short int ip_len;      // 2 bytes Total Length
    unsigned short int ip_id;       // 2 bytes Identification
    unsigned short int ip_off;      // 2 bytes Fragment Offset
    unsigned char ip_ttl;           // 1 byte  TTL
    unsigned char ip_p;             // 1 byte  Protocol
    unsigned short int ip_sum;      // 2 bytes Header Checksum
    unsigned int ip_src;            // 4 bytes Source Address
    unsigned int ip_dst;            // 4 bytes Destination Address
}; /* total ip header length: 20 bytes */

#endif