#include <arpa/inet.h>

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