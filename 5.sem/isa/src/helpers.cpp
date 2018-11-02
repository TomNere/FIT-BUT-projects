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

char* readRRName(const uint8_t* packet, uint32_t* packetPos, uint32_t idPos, uint32_t len)
{
    uint32_t i, next, pos;
    pos = *packetPos;
    uint32_t endPos = 0;
    uint32_t nameLen = 0;
    uint32_t steps = 0;
    char* name;

    // Scan through the name, one character at a time. We need to look at 
    // each character to look for values we can't print in order to allocate
    // extra space for escaping them.  'next' is the next position to look
    // for a compression jump or name end.
    // It's possible that there are endless loops in the name. Our protection
    // against this is to make sure we don't read more bytes in this process
    // than twice the length of the data.  Names that take that many steps to 
    // read in should be impossible.
    next = pos;
    while (pos < len && !(next == pos && packet[pos] == 0) && steps < len * 2)
    {
        uint8_t c = packet[pos];
        steps++;
        if (next == pos)
        {
            // Handle message compression.  
            // If the length byte starts with the bits 11, then the rest of
            // this byte and the next form the offset from the dns proto start
            // to the start of the remainder of the name.
            if ((c & 0xc0) == 0xc0)
            {
                if (pos + 1 >= len)
                {
                    return 0;
                }
                if (endPos == 0) endPos = pos + 1;
                pos = idPos + ((c & 0x3f) << 8) + packet[pos+1];
                next = pos;
            } 
            else 
            {
                nameLen++;
                pos++;
                next = next + c + 1; 
            }
        } 
        else 
        {
            if (c >= '!' && c <= 'z' && c != '\\')
            {
                nameLen ++;
            }
            else nameLen += 4;
            pos++;
        }
    }
    if (endPos == 0)
    {
        endPos = pos;
    }

    // Due to the nature of DNS name compression, it's possible to get a
    // name that is infinitely long. Return an error in that case.
    // We use the len of the packet as the limit, because it shouldn't 
    // be possible for the name to be that long.
    if (steps >= 2*len || pos >= len)
    {
        return NULL;
    }

    nameLen++;

    name = (char*) malloc(sizeof(char)* nameLen ) ;
    pos = *packetPos;

    //Now actually assemble the name.
    //We've already made sure that we don't exceed the packet length, so
    // we don't need to make those checks anymore.
    // Non-printable and whitespace characters are replaced with a question
    // mark. They shouldn't be allowed under any circumstances anyway.
    // Other non-allowed characters are kept as is, as they appear sometimes
    // regardless.
    // This shouldn't interfere with IDNA (international
    // domain names), as those are ascii encoded.
    next = pos;
    i = 0;
    while (next != pos || packet[pos] != 0) 
    {
        if (pos == next) 
        {
            if ((packet[pos] & 0xc0) == 0xc0) 
            {
                pos = idPos + ((packet[pos] & 0x3f) << 8) + packet[pos+1];
                next = pos;
            } 
            else
            {
                // Add a period except for the first time.
                if (i != 0) name[i++] = '.';
                next = pos + packet[pos] + 1;
                pos++;
            }
        }
        else
        {
            uint8_t c = packet[pos];
            if (c >= '!' && c <= '~' && c != '\\')
            {
                name[i] = packet[pos];
                i++; pos++;
            } 
            else
            {
                name[i] = '\\';
                name[i+1] = 'x';
                name[i+2] = c/16 + 0x30;
                name[i+3] = c%16 + 0x30;
                if (name[i+2] > 0x39) name[i+2] += 0x27;
                if (name[i+3] > 0x39) name[i+3] += 0x27;
                i+=4; 
                pos++;
            }
        }
    }
    name[i] = 0;

    *packetPos = endPos + 1;
    LOGGING("Returning RR name: " << name);
    return name;
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

