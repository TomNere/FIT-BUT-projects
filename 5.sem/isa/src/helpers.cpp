#include "dns-export.h"

using namespace std;

// Move IPv4 addr at pointer P into ip object D, and set it's type.
#define IPv4_MOVE(D, P) D.addr.v4.s_addr = *(in_addr_t *)(P); \
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

// Parse the ethernet headers, and return the payload position (0 on error).
uint32_t ethParse(struct pcap_pkthdr *header, uint8_t *packet,
                   uint16_t* ethType, int datalink) {
    uint32_t pos = 0;

    if (header->len < 14) {
        fprintf(stderr, "Truncated Packet(eth)\n");
        return 0;
    }

    while (pos < 6) {
        pos++;
    }
    pos = pos + 6;
   
    // Skip the extra 2 byte field inserted in "Linux Cooked" captures.
    if (datalink == DLT_LINUX_SLL) {
        pos = pos + 2;
    }

    // Skip VLAN tagging 
    if (packet[pos] == 0x81 && packet[pos+1] == 0) 
    {
        pos = pos + 4;
    }

    *ethType = (packet[pos] << 8) + packet[pos+1];
    pos = pos + 2;

    return pos;
}

// Parse MPLS. We don't care about the data in these headers, all we have
// to do is continue parsing them until the 'bottom of stack' flag is set.
uint32_t mplsParse(uint32_t pos, struct pcap_pkthdr *header,
                    uint8_t *packet, uint16_t* ethType) {
    // Bottom of stack flag.
    uint8_t bos;
    do {
        // Deal with truncated MPLS.
        if (header->len < (pos + 4)) {
            fprintf(stderr, "Truncated Packet(mpls)\n");
            return 0;
        }

        bos = packet[pos + 2] & 0x01;
        pos += 4;
    } while (bos == 0);

    if (header->len < pos) {
        fprintf(stderr, "Truncated Packet(post mpls)\n");
        return 0;
    }


    // 'Guess' the next protocol. This can result in false positives, but
    // generally not.
    uint8_t ip_ver = packet[pos] >> 4;
    switch (ip_ver) {
        case IPv4:
            *ethType = 0x0800; 
            break;
        case IPv6:
            *ethType = 0x86DD;
             break;
        default:
            *ethType = 0;
    }

    return pos;
}

// Add this ip fragment to the our list of fragments. If we complete
// a fragmented packet, return it. 
// Limitations - Duplicate packets may end up in the list of fragments.
//             - We aren't going to expire fragments, and we aren't going
//                to save/load them like with TCP streams either. This may
//                mean lost data.
ip_fragment* ip_frag_add(ip_fragment* thiss, ip_fragment* ip_fragment_head) {
    ip_fragment ** curr = &ip_fragment_head;
    ip_fragment ** found = NULL;

    //DBG(printf("Adding fragment at %p\n", this);)

    // Find the matching fragment list.
    while (*curr != NULL) {
        if ((*curr)->id == thiss->id && 
            IP_CMP((*curr)->src, thiss->src) &&
            IP_CMP((*curr)->dst, thiss->dst)) {
            found = curr;
            cerr << "Match found. " << *found << endl;
            break;
        }
        curr = &(*curr)->next;
    }

    // At this point curr will be the head of our matched chain of fragments, 
    // and found will be the same. We'll use found as our pointer into this
    // chain, and curr to remember where it starts.
    // 'found' could also be NULL, meaning no match was found.

    // If there wasn't a matching list, then we're done.
    if (found == NULL) {
        cerr << "No matching fragments.\n";
        thiss->next = ip_fragment_head;
        ip_fragment_head = thiss;
        return NULL;
    }

    while (*found != NULL) {
        cerr << "*found: " << (*found)->start << (*found)->end << "this: "<<thiss->start << thiss->end << endl;
        if ((*found)->start >= thiss->end) {
            cerr << "It goes in front of "<< *found << endl;
            // It goes before, so put it there.
            thiss->child = *found;
            thiss->next = (*found)->next;
            *found = thiss;
            break;
        } else if ((*found)->child == NULL && 
                    (*found)->end <= thiss->start) {
            cerr << "It goes at the end. " << *found;
           // We've reached the end of the line, and that's where it
            // goes, so put it there.
            (*found)->child = thiss;
            break;
        }
        cerr << "What: " << *found;
        found = &((*found)->child);
    }
    cerr << "What: " << *found;

    // We found no place for the fragment, which means it's a duplicate
    // (or the chain is screwed up...)
    if (*found == NULL) {
        cerr << "No place for fragment: " << *found;
        free(thiss);
        return NULL;
    }

    // Now we try to collapse the list.
    found = curr;
    while ((*found != NULL) && (*found)->child != NULL) {
        ip_fragment * child = (*found)->child;
        if ((*found)->end == child->start) {
            fprintf(stderr, "Merging frag at offset %u-%u with %u-%u\n", 
                        (*found)->start, (*found)->end,
                        child->start, child->end);
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
        } else {
            found = &(*found)->child;
        }
    }

    fprintf(stderr, "*curr, start: %u, end: %u, islast: %u\n", 
                (*curr)->start, (*curr)->end, (*curr)->islast);
    // Check to see if we completely collapsed it.
    // *curr is the pointer to the first fragment.
    if ((*curr)->islast != 0) {
        ip_fragment * ret = *curr;
        // Remove this from the fragment list.
        *curr = (*curr)->next;
        fprintf(stderr, "Returning reassembled fragments.\n");
        return ret;
    }
    // This is what happens when we don't complete a packet.
    return NULL;
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
