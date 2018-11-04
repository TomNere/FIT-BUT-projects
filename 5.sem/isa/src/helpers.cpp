#include "dns-export.h"
#include "helpers.h"
#include "structures.h"

using namespace std;

char IP_STR_BUFF[INET6_ADDRSTRLEN];

char* mk_error(const char * msg, const uint8_t * packet, uint32_t pos,
                uint16_t rdlength) {
    return "MK error";
}

char * b64encode(const uint8_t * data, uint32_t pos, uint16_t length) {
    char * out;
    uint32_t endPos = pos + length;
    uint32_t op = 0;

    // We allocate a little extra here sometimes, but in this application
    // these strings are almost immediately de-allocated anyway.
    out = (char*) malloc(sizeof(char) * ((length/3 + 1)*4 + 1));

    while (pos + 2 < endPos) {
        out[op] = cb64[ data[pos] >> 2 ];
        out[op+1] = cb64[ ((data[pos] & 0x3) << 4) | 
                          ((data[pos+1] & 0xf0) >> 4) ];
        out[op+2] = cb64[ ((data[pos+1] & 0xf) << 2) | 
                          ((data[pos+2] & 0xc0) >> 6) ];
        out[op+3] = cb64[ data[pos+2] & 0x3f ];

        op = op + 4;
        pos = pos + 3;
    }

    if ((endPos - pos) == 2) {
        out[op] = cb64[ data[pos] >> 2 ];
        out[op+1] = cb64[ ((data[pos] & 0x3) << 4) | 
                          ((data[pos+1] & 0xf0) >> 4) ];
        out[op+2] = cb64[ ((data[pos+1] & 0xf) << 2) ];
        out[op+3] = '=';
        op = op + 4;
    } else if ((endPos - pos) == 1) {
        out[op] = cb64[ data[pos] >> 2 ];
        out[op+1] = cb64[ ((data[pos] & 0x3) << 4) ];
        out[op+2] = out[op+3] = '=';
        op = op + 4;
    }
    out[op] = 0; 

    return out;
}

// Convert an ip struct to a string. The returned buffer is internal, 
// and need not be freed. 
char * iptostr(ipAddr * ip) {
    if (ip->vers == IPv4) {
        inet_ntop(AF_INET, (const void *) &(ip->addr.v4),
                  IP_STR_BUFF, INET6_ADDRSTRLEN);
    } else { // IPv6
        inet_ntop(AF_INET6, (const void *) &(ip->addr.v6),
                  IP_STR_BUFF, INET6_ADDRSTRLEN);
    }
    return IP_STR_BUFF;
}



// Print all resource records in the given section.
void printRRSection(list<dnsRR> rrs, string name) {
    list <dnsRR> :: iterator it; 
        
    for(it = rrs.begin(); it != rrs.end(); it++) 
    {
        // Print the rr seperator and rr section name.
        cout << '\t' << name;
        // Search the excludes list to see if we should not print this
        // rtype.
        
        string name, data;
        name = (it->name.empty()) ? "*empty*" : it->name;
        data = (it->data.empty()) ? "*empty*" : it->data;
        cout << "RECORD 1. output\n";
            
        if (it->rrName.empty())
        {
            // Handle bad records.
            cout <<  name <<" UNKNOWN(" << it->type << it->cls << data;
        }
        else
            // Print the string rtype name with the rest of the record.
            cout << name << " " << it->rrName << " " << data;

                cout << "RECORD 2. output";
                // The -r option case. 
                // Print the rtype and class number with the record.
                cout << name << " " << " " << it->type << " " << it->cls << " " << data;
    }
}

// // Free a dnsRR struct.
// void dnsRR_free(dnsRR * rr) {
//     if (rr == NULL) return;
//     if (rr->name.empty()) free(rr->name);
//     if (rr->data != NULL) free(rr->data);
//     dnsRR_free(rr->next);
//     free(rr);
// }



// Print the time stamp.
void print_ts(struct timeval * ts) {
    cout << (int)ts->tv_sec << ":" << (int)ts->tv_usec;
}

