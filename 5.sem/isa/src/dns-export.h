#include <arpa/inet.h>
#include <iostream> // No comment needed...
#include <unistd.h> // Getopt
#include <signal.h> // Signal shandling
#include <pcap.h>   // No comment needed...
#include <string.h>
#include <list>
#include <iterator>
#include <stdio.h>
#include "declarations.h"

using namespace std;

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


#define UDP 0x11
#define TCP 0x06

rr_data_parser opts;

static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

#define FORCE 1