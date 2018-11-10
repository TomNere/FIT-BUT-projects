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

#define SYSLOG_PORT 514
#define MESSAGE_SIZE 900

#define SIZE_ETHERNET (14)       // offset of Ethernet header to L3 protocol

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

#endif