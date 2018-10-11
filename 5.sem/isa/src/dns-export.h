#include <arpa/inet.h>
#include <iostream> // No comment needed...
#include <unistd.h> // Getopt
#include <signal.h> // Signal shandling
#include <pcap.h>   // No comment needed...
#include <string.h>
#include <list>
#include <iterator>

using namespace std;

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

// Structure representing parameters of running program in raw string format
typedef struct
{
    string file;
    string interface;
    string syslogServer;
    string time;
} raw_parameters;

/////////////////////////////////////////////////////////////////

// Basic network layer information.
typedef struct {
    ip_addr src;
    ip_addr dst;
    uint32_t length;
    uint8_t proto;
} ipInfo;

#define IPv4 0x04
#define IPv6 0x06

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

char IP_STR_BUFF[INET6_ADDRSTRLEN];

#define UDP 0x11
#define TCP 0x06