#ifndef _STRUCTURES_H_
#define _STRUCTURES_H_

#include <string.h>
#include <iostream>     // Strings
#include <arpa/inet.h>  // in_addr

using namespace std;

/******************************************* CONSTS ******************************/

const string help = "Invalid parameters!\n\n"
                    "Usage: dns-export [-r file.pcap] [-i interface] [-s syslog-server] [-t seconds] \n";

#define FORCE 1

#define IPv4 0x04
#define IPv6 0x06

#define UDP 0x11
#define TCP 0x06

#define REORDER_LIMIT 100000

#define NULL_DOC "This data is simply hex escaped. \n"\
"Non printable characters are given as a hex value (\\x30), for example."

static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


/***************************************** MACROS ********************************/

#define ERR_RET(message)     \
    cerr << message << " Terminating." << endl; \
    exit(EXIT_FAILURE);

#define LOGGING(message) \
    cerr << "LOG: " <<   \
    message << endl;

// Move IPv4 addr at pointer P into ip object D, and set it's type.
#define IPv4_MOVE(D, P) D.addr.v4.s_addr = *(in_addr_t*)(P); \
                        D.vers = IPv4;
// Move IPv6 addr at pointer P into ip object D, and set it's type.
#define IPv6_MOVE(D, P) memcpy(D.addr.v6.s6_addr, P, 16); D.vers = IPv6;

// Compare two IP addresses.
#define IP_CMP(ipA, ipB) ((ipA.vers == ipB.vers) &&\
                          (ipA.vers == IPv4 ? \
                            ipA.addr.v4.s_addr == ipB.addr.v4.s_addr : \
                ((ipA.addr.v6.s6_addr32[0] == ipB.addr.v6.s6_addr32[0]) && \
                 (ipA.addr.v6.s6_addr32[1] == ipB.addr.v6.s6_addr32[1]) && \
                 (ipA.addr.v6.s6_addr32[2] == ipB.addr.v6.s6_addr32[2]) && \
                 (ipA.addr.v6.s6_addr32[3] == ipB.addr.v6.s6_addr32[3])) \
                 ))

// Get the value of the BITth bit from byte offset O bytes from base B.
#define GET_BIT(B,O,BIT) (uint8_t)(((*(B+O)) & (1 << (BIT))) >> BIT )
// Get a two byte little endian u_int at base B and offset O.
#define LE_U_SHORT(B,O) (uint16_t)((B[O]<<8)+B[O+1])
// Get a four byte little endian u_int at base B and offset O.
#define LE_U_INT(B,O) (uint32_t)((B[O]<<24)+(B[O+1]<<16)+(B[O+2]<<8)+B[O+3])
// Get the DNS tcp length prepended field.
#define TCP_DNS_LEN(P,O) ((P[O]<<8) + P[O+1])

// Structure representing parameters of running program in raw string format
typedef struct
{
    string file;
    string interface;
    string syslogServer;
    string time;
} raw_parameters;

/////////////////////////////////////////////////////////////////

// IP address container that is IP version agnostic.
// The IPvX_MOVE macros handle filling these with packet data correctly.
typedef struct ip_addr {
    // Should always be either 4 or 6.
    uint8_t vers;
    union {
        struct in_addr v4;
        struct in6_addr v6;
    } addr;
} ip_addr;

// Basic network layer information.
typedef struct {
    ip_addr src;
    ip_addr dst;
    uint32_t length;
    uint8_t proto;
} ip_info;

// Holds the information for a dns question.
typedef struct dns_question {
    char * name;
    uint16_t type;
    uint16_t cls;
    struct dns_question * next;
} dns_question;

// Holds the information for a dns resource record.
typedef struct dns_rr {
    char * name;
    uint16_t type;
    uint16_t cls;
    const char * rr_name;
    uint16_t ttl;
    uint16_t rdlength;
    uint16_t data_len;
    char * data;
    struct dns_rr * next;
} dns_rr;

// Holds general DNS information.
typedef struct {
    uint16_t id;
    char qr;
    char AA;
    char TC;
    uint8_t rcode;
    uint8_t opcode;
    uint16_t qdcount;
    dns_question * queries;
    uint16_t ancount;
    dns_rr * answers;
    uint16_t nscount;
    dns_rr * name_servers;
    uint16_t arcount;
    dns_rr * additional;
} dns_info;

// TCP header information. Also contains pointers used to connect to this
// to other TCP streams, and to connect this packet to other packets in
// it's stream.
typedef struct tcp_info {
    struct timeval ts;
    ip_addr src;
    ip_addr dst;
    uint16_t srcport;
    uint16_t dstport;
    uint32_t sequence;
    uint32_t ack_num;
    // The length of the data portion.
    uint32_t len;
    uint8_t syn;
    uint8_t ack;
    uint8_t fin;
    uint8_t rst;
    uint8_t * data;
    // The next item in the list of tcp sessions.
    struct tcp_info * next_sess;
    // These are for connecting all the packets in a session. The session
    // pointers above will always point to the most recent packet.
    // next_pkt and prev_pkt make chronological sense (next_pkt is always 
    // more recent, and prev_pkt is less), we just hold the chain by the tail.
    struct tcp_info * next_pkt;
    struct tcp_info * prev_pkt;
} tcp_info;

typedef char * rr_data_parser(const uint8_t*, uint32_t, uint32_t, uint16_t, uint32_t);

typedef struct {
    uint16_t cls;
    uint16_t rtype;
    rr_data_parser * parser;
    const char * name;
    const char * doc;
    unsigned long long count;
} rr_parser_container;

// IP fragment information.
typedef struct ip_fragment {
    uint32_t id;
    ip_addr src;
    ip_addr dst;
    uint32_t start;
    uint32_t end;
    uint8_t * data;
    uint8_t islast; 
    struct ip_fragment * next;
    struct ip_fragment * child; 
} ip_fragment;

// Transport information.
typedef struct {
    uint16_t srcport;
    uint16_t dstport;
    // Length of the payload.
    uint16_t length;
    uint8_t transport; 
} transport_info;

#endif