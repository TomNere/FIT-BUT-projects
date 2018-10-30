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

// EtherType values for some notable protocols
// https://en.wikipedia.org/wiki/EtherType
#define VLAN_TAGGED 0x8100
#define MPLS_UNI 0x8847
#define IPv4Prot 0x0800
#define IPv6Prot 0x86DD

#define IPv4 0x04
#define IPv6 0x06

#define UDP 0x11
#define TCP 0x06

#define REORDER_LIMIT 100000

#define NULL_DOC "This data is simply hex escaped. \n"\
"Non printable characters are given as a hex value (\\x30), for example."

static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

const string red("\033[0;31m");
const string yellow("\033[1;33m");
const string reset("\033[0m");


/***************************************** MACROS ********************************/

#define ERR_RET(message)     \
    cerr << red << message << " Terminating." << reset << endl; \
    exit(EXIT_FAILURE);

#define LOGGING(message) \
    cerr << yellow << "LOG: " <<   \
    message << reset << endl;

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
} rawParameters;

/////////////////////////////////////////////////////////////////

// IP address container that is IP version agnostic.
// The IPvX_MOVE macros handle filling these with packet data correctly.
typedef struct ipAddr {
    // Should always be either 4 or 6.
    uint8_t vers;
    union {
        struct in_addr v4;
        struct in6_addr v6;
    } addr;
} ipAddr;

// Basic network layer information.
typedef struct {
    ipAddr src;
    ipAddr dst;
    uint32_t length;
    uint8_t proto;
} ipInfo;

// Holds the information for a dns question.
typedef struct dnsQuestion {
    char * name;
    uint16_t type;
    uint16_t cls;
    struct dnsQuestion * next;
} dnsQuestion;

// Holds the information for a dns resource record.
typedef struct dnsRR {
    char * name;
    uint16_t type;
    uint16_t cls;
    const char * rrName;
    uint16_t ttl;
    uint16_t rdlength;
    uint16_t dataLen;
    char * data;
    struct dnsRR * next;
} dnsRR;

// Holds general DNS information.
typedef struct {
    uint16_t id;
    char qr;
    char AA;
    char TC;
    uint8_t rcode;
    uint8_t opcode;
    uint16_t qdcount;
    dnsQuestion * queries;
    uint16_t ancount;
    dnsRR * answers;
    uint16_t nscount;
    dnsRR * nameServers;
    uint16_t arcount;
    dnsRR * additional;
} dnsInfo;

// TCP header information. Also contains pointers used to connect to this
// to other TCP streams, and to connect this packet to other packets in
// it's stream.
typedef struct tcpInfo {
    struct timeval ts;
    ipAddr src;
    ipAddr dst;
    uint16_t srcport;
    uint16_t dstport;
    uint32_t sequence;
    uint32_t ackNum;
    // The length of the data portion.
    uint32_t len;
    uint8_t syn;
    uint8_t ack;
    uint8_t fin;
    uint8_t rst;
    uint8_t * data;
    // The next item in the list of tcp sessions.
    struct tcpInfo * nextSess;
    // These are for connecting all the packets in a session. The session
    // pointers above will always point to the most recent packet.
    // nextPkt and prevPkt make chronological sense (nextPkt is always 
    // more recent, and prevPkt is less), we just hold the chain by the tail.
    struct tcpInfo * nextPkt;
    struct tcpInfo * prevPkt;
} tcpInfo;

typedef char * rrDataParser(const uint8_t*, uint32_t, uint32_t, uint16_t, uint32_t);

typedef struct {
    uint16_t cls;
    uint16_t rtype;
    rrDataParser * parser;
    const char * name;
    const char * doc;
    unsigned long long count;
} rrParserContainer;

// Transport information.
typedef struct {
    uint16_t srcport;
    uint16_t dstport;
    // Length of the payload.
    uint16_t length;
    uint8_t transport; 
} transportInfo;

#endif