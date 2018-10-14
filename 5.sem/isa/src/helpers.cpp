#include "dns-export.h"
#include "helpers.h"
#include "structures.h"

using namespace std;

unsigned int PACKETS_SEEN = 0;
char IP_STR_BUFF[INET6_ADDRSTRLEN];

// Add this ip fragment to the our list of fragments. If we complete
// a fragmented packet, return it. 
// Limitations - Duplicate packets may end up in the list of fragments.
//             - We aren't going to expire fragments, and we aren't going
//                to save/load them like with TCP streams either. This may
//                mean lost data.
ip_fragment* ipFragAdd(ip_fragment* frag, ip_fragment* ip_fragment_head)
{
    ip_fragment** curr = &ip_fragment_head;
    ip_fragment** found = NULL;

    LOGGING("Adding fragment at " << frag);

    // Find the matching fragment list.
    while (*curr != NULL)
    {
        if ((*curr)->id == frag->id && IP_CMP((*curr)->src, frag->src) && IP_CMP((*curr)->dst, frag->dst))
        {
            found = curr;
            LOGGING("Match found. " << *found);
            break;
        }
        curr = &(*curr)->next;
    }

    // At this point curr will be the head of our matched chain of fragments, 
    // and found will be the same. We'll use found as our pointer into this
    // chain, and curr to remember where it starts.
    // 'found' could also be NULL, meaning no match was found.

    // If there wasn't a matching list, then we're done.
    if (found == NULL)
    {
        LOGGING("No matching fragments");
        frag->next = ip_fragment_head;
        ip_fragment_head = frag;
        return NULL;
    }

    while (*found != NULL)
    {
        cerr << "*found: " << (*found)->start << (*found)->end << "this: "<<frag->start << frag->end << endl;
        if ((*found)->start >= frag->end)
        {
            cerr << "It goes in front of "<< *found << endl;
            // It goes before, so put it there.
            frag->child = *found;
            frag->next = (*found)->next;
            *found = frag;
            break;
        } 
        else if ((*found)->child == NULL && (*found)->end <= frag->start)
        {
            cerr << "It goes at the end. " << *found;
            // We've reached the end of the line, and that's where it
            // goes, so put it there.
            (*found)->child = frag;
            break;
        }
        cerr << "What: " << *found;
        found = &((*found)->child);
    }
    cerr << "What: " << *found;

    // We found no place for the fragment, which means it's a duplicate
    // (or the chain is screwed up...)
    if (*found == NULL)
    {
        LOGGING("No place for fragment: " << *found);
        free(frag);
        return NULL;
    }

    // Now we try to collapse the list.
    found = curr;
    while ((*found != NULL) && (*found)->child != NULL)
    {
        ip_fragment* child = (*found)->child;
        if ((*found)->end == child->start)
        {
            fprintf(stderr, "Merging frag at offset %u-%u with %u-%u\n", (*found)->start, (*found)->end, child->start, child->end);
            uint32_t child_len = child->end - child->start;
            uint32_t fnd_len = (*found)->end - (*found)->start;
            uint8_t * buff = (uint8_t*) malloc(sizeof(uint8_t) * (fnd_len + child_len));
            memcpy(buff, (*found)->data, fnd_len);
            memcpy(buff + fnd_len, child->data, child_len);
            (*found)->end = (*found)->end + child_len;
            (*found)->islast = child->islast;
            (*found)->child = child->child;
            // Free the old data and the child, and make the combined buffer
            // the new data for the merged fragment.
            free((*found)->data);
            free(child->data);
            free(child);
            (*found)->data = buff;
        } 
        else
        {
            found = &(*found)->child;
        }
    }

    fprintf(stderr, "*curr, start: %u, end: %u, islast: %u\n", (*curr)->start, (*curr)->end, (*curr)->islast);
    // Check to see if we completely collapsed it.
    // *curr is the pointer to the first fragment.
    if ((*curr)->islast != 0)
    {
        ip_fragment * ret = *curr;
        // Remove this from the fragment list.
        *curr = (*curr)->next;
        fprintf(stderr, "Returning reassembled fragments.\n");
        return ret;
    }
    // This is what happens when we don't complete a packet.
    return NULL;
}

char* mk_error(const char * msg, const uint8_t * packet, uint32_t pos,
                uint16_t rdlength) {
    return "MK error";
}

char * b64encode(const uint8_t * data, uint32_t pos, uint16_t length) {
    char * out;
    uint32_t end_pos = pos + length;
    uint32_t op = 0;

    // We allocate a little extra here sometimes, but in this application
    // these strings are almost immediately de-allocated anyway.
    out = (char*) malloc(sizeof(char) * ((length/3 + 1)*4 + 1));

    while (pos + 2 < end_pos) {
        out[op] = cb64[ data[pos] >> 2 ];
        out[op+1] = cb64[ ((data[pos] & 0x3) << 4) | 
                          ((data[pos+1] & 0xf0) >> 4) ];
        out[op+2] = cb64[ ((data[pos+1] & 0xf) << 2) | 
                          ((data[pos+2] & 0xc0) >> 6) ];
        out[op+3] = cb64[ data[pos+2] & 0x3f ];

        op = op + 4;
        pos = pos + 3;
    }

    if ((end_pos - pos) == 2) {
        out[op] = cb64[ data[pos] >> 2 ];
        out[op+1] = cb64[ ((data[pos] & 0x3) << 4) | 
                          ((data[pos+1] & 0xf0) >> 4) ];
        out[op+2] = cb64[ ((data[pos+1] & 0xf) << 2) ];
        out[op+3] = '=';
        op = op + 4;
    } else if ((end_pos - pos) == 1) {
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
char * iptostr(ip_addr * ip) {
    if (ip->vers == IPv4) {
        inet_ntop(AF_INET, (const void *) &(ip->addr.v4),
                  IP_STR_BUFF, INET6_ADDRSTRLEN);
    } else { // IPv6
        inet_ntop(AF_INET6, (const void *) &(ip->addr.v6),
                  IP_STR_BUFF, INET6_ADDRSTRLEN);
    }
    return IP_STR_BUFF;
}

char* readRRName(const uint8_t* packet, uint32_t* packet_p, uint32_t id_pos, uint32_t len)
{
    uint32_t i, next, pos=*packet_p;
    uint32_t end_pos = 0;
    uint32_t name_len=0;
    uint32_t steps = 0;
    char * name;

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
                if (end_pos == 0) end_pos = pos + 1;
                pos = id_pos + ((c & 0x3f) << 8) + packet[pos+1];
                next = pos;
            } 
            else 
            {
                name_len++;
                pos++;
                next = next + c + 1; 
            }
        } 
        else 
        {
            if (c >= '!' && c <= 'z' && c != '\\')
            {
                name_len++;
            }
            else name_len += 4;
            pos++;
        }
    }
    if (end_pos == 0)
    {
        end_pos = pos;
    }

    // Due to the nature of DNS name compression, it's possible to get a
    // name that is infinitely long. Return an error in that case.
    // We use the len of the packet as the limit, because it shouldn't 
    // be possible for the name to be that long.
    if (steps >= 2*len || pos >= len)
    {
        return NULL;
    }

    name_len++;

    name = (char*) malloc(sizeof(char)* name_len);
    pos = *packet_p;

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
                pos = id_pos + ((packet[pos] & 0x3f) << 8) + packet[pos+1];
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

    *packet_p = end_pos + 1;
    LOGGING("Returning RR name: " << name);
    return name;
}

// Print all resource records in the given section.
void printRRSection(dns_rr * next, char * name) {
    int skip;
    int i;
    while (next != NULL) {
        // Print the rr seperator and rr section name.
        cout << '\t' << name;
        skip = 0;
        // Search the excludes list to see if we should not print this
        // rtype.
        
            string name, data;
            name = (next->name == NULL) ? "*empty*" : next->name;
            data = (next->data == NULL) ? "*empty*" : next->data;
            cout << "RECORD 1. output\n";
            
                if (next->rr_name == NULL) 
                    // Handle bad records.
                    cout <<  name <<" UNKNOWN(" << next->type << next->cls << data;
                else
                    // Print the string rtype name with the rest of the record.
                    cout << name << " " << next->rr_name << " " << data;

                cout << "RECORD 2. output";
                // The -r option case. 
                // Print the rtype and class number with the record.
                cout << name << " " << " " << next->type << " " << next->cls << " " << data;
        
        next = next->next; 
    }
}

// Free a dns_question struct.
void dns_question_free(dns_question * question) {
    if (question == NULL) return;
    if (question->name != NULL) free(question->name);
    dns_question_free(question->next);
    free(question);
}

// Free a dns_rr struct.
void dns_rr_free(dns_rr * rr) {
    if (rr == NULL) return;
    if (rr->name != NULL) free(rr->name);
    if (rr->data != NULL) free(rr->data);
    dns_rr_free(rr->next);
    free(rr);
}



// Print the time stamp.
void print_ts(struct timeval * ts) {
    cout << (int)ts->tv_sec << ":" << (int)ts->tv_usec;
}

