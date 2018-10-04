#include <iostream>		// No comment needed...
#include <unistd.h>		// Getopt
#include <signal.h>     // Signal shandling
#include <pcap.h>		// No comment needed...
#include <arpa/inet.h>
#include "dns-export.h"
#include <string.h>

using namespace std;

/*************************** CONST *******************************************************************************************/

#define ERR_RET(message) cerr << message << endl; exit(EXIT_FAILURE);

#define IPv4_MOVE(D, P) D.addr.v4.s_addr = *(in_addr_t *)(P); \
                        D.vers = 0x04;
// Move IPv6 addr at pointer P into ip object D, and set it's type.
#define IPv6_MOVE(D, P) memcpy(D.addr.v6.s6_addr, P, 16); D.vers = 0x06;

const string help = "Invalid parameters!\n\n"
                    "Usage: dns-export [-r file.pcap] [-i interface] [-s syslog-server] [-t seconds] \n";

/*****************************************************************************************************************************/

// Structure representing parameters of running program in raw string format
typedef struct raw_parameters
{
	string file;
	string interface;
	string syslogServer;
	string time;
} raw_parameters;

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

// Transport information.
typedef struct {
    uint16_t srcport;
    uint16_t dstport;
    // Length of the payload.
    uint16_t length;
    uint8_t transport; 
} transport_info;
/********************************************************** METHODS **********************************************************/

// Write stats to stdout
void writeStats()
{

}

// Signal handler 
// Write stats end terminate
void handleSig(int sigNum)
{
	writeStats();
}


// Argument parser
raw_parameters parseArg(int argc, char const *argv[])
{
	raw_parameters params;
	params.file = params.interface = params.syslogServer = params.time = "";
	int option;

	while ((option = getopt(argc, (char**)argv, "r:i:s:t:")) != -1) {
		switch (option) {
		case 'r':
			if (params.file.compare(""))
				ERR_RET(help);

			params.file = string(optarg);
			break;
		case 'i':
			if (params.interface.compare(""))
				ERR_RET(help);

			params.interface = string(optarg);
			break;
		case 's':
			if (params.syslogServer.compare(""))
				ERR_RET(help);

			params.syslogServer = string(optarg);
			break;
		case 't':
			if (params.time.compare(""))
				ERR_RET(help);

			params.time = string(optarg);
			break;
		default:
			ERR_RET(help);
		}
	}

	return params;
}

/*********************************************************** INTERFACE ***************************************************************/

void logInterface(string strInterface, string strLog, string strTime)
{
	int time;
	if (strTime.compare(""))
		time = 60;			// Default value
	else
	{
		// Check for valid time
		if (strTime.find_first_not_of("0123456789") == string::npos)
        	time = stoi(strTime, NULL, 10);
    	else
        	ERR_RET("Time must be number in seconds!");
	}

	pcap_t* pcapInterface;
	//if ((pcapInterface = pcap_open_live(strInterface.c_str(), 1, 1000, 1000))



}

/*********************************************************** PCAP FILE ***************************************************************/

// Parse the ethernet headers, and return the payload position (0 on error).
uint32_t eth_parse(struct pcap_pkthdr* header, uint8_t* packet, uint16_t* ethtype) {

    if (header->len < 14)
        ERR_RET("Truncated Packet(eth)");

    uint32_t pos = 14;	// Header skip
    
    // Skip VLAN tagging
    if (packet[pos] == 0x81 && packet[pos+1] == 0)
		pos = pos + 4;

    *ethtype = (packet[pos] << 8) + packet[pos+1];
    pos = pos + 2;

    // SHOW_RAW(
    //     printf("\neth ");
    //     print_packet(header->len, packet, 0, pos, 18);
    // )
    // VERBOSE(
    //     printf("dstmac: %02x:%02x:%02x:%02x:%02x:%02x, "
    //            "srcmac: %02x:%02x:%02x:%02x:%02x:%02x\n",
    //            eth->dstmac[0],eth->dstmac[1],eth->dstmac[2],
    //            eth->dstmac[3],eth->dstmac[4],eth->dstmac[5],
    //            eth->srcmac[0],eth->srcmac[1],eth->srcmac[2],
    //            eth->srcmac[3],eth->srcmac[4],eth->srcmac[5]);
    // )
    return pos;
}

// Parse MPLS. We don't care about the data in these headers, all we have
// to do is continue parsing them until the 'bottom of stack' flag is set.
uint32_t mpls_parse(uint32_t pos, struct pcap_pkthdr *header, uint8_t *packet, uint16_t* ethtype) {
    // Bottom of stack flag.
    uint8_t bos;
    do {
        //VERBOSE(printf("MPLS Layer.\n");)
        // Deal with truncated MPLS.
        if (header->len < (pos + 4)) {
            ERR_RET("Truncated Packet(mpls)");
        }

        bos = packet[pos + 2] & 0x01;
        pos += 4;
        //DBG(printf("MPLS layer. \n");)
    } while (bos == 0);

    if (header->len < pos) {
        fprintf(stderr, "Truncated Packet(post mpls)\n");
        return 0;
    }


    // 'Guess' the next protocol. This can result in false positives, but
    // generally not.
    uint8_t ip_ver = packet[pos] >> 4;
    switch (ip_ver) {
        case 0x04:
            *ethtype = 0x0800; 
            break;
        case 0x06:
            *ethtype = 0x86DD; 
            break;
        default:
            *ethtype = 0;
    }

    return pos;
}

/*
// Add this ip fragment to the our list of fragments. If we complete
// a fragmented packet, return it. 
// Limitations - Duplicate packets may end up in the list of fragments.
//             - We aren't going to expire fragments, and we aren't going
//                to save/load them like with TCP streams either. This may
//                mean lost data.
ip_fragment * ip_frag_add(ip_fragment * thiss) {
    ip_fragment ** curr = &(conf->ip_fragment_head);
    ip_fragment ** found = NULL;

    //DBG(printf("Adding fragment at %p\n", this);)

    // Find the matching fragment list.
    while (*curr != NULL) {
        if ((*curr)->id == thiss->id && 
            IP_CMP((*curr)->src, this->src) &&
            IP_CMP((*curr)->dst, this->dst)) {
            found = curr;
            DBG(printf("Match found. %p\n", *found);)
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
        DBG(printf("No matching fragments.\n");)
        this->next = conf->ip_fragment_head;
        conf->ip_fragment_head = this;
        return NULL;
    }

    while (*found != NULL) {
        DBG(printf("*found: %u-%u, this: %u-%u\n",
                   (*found)->start, (*found)->end,
                   this->start, this->end);)
        if ((*found)->start >= this->end) {
            DBG(printf("It goes in front of %p\n", *found);)
            // It goes before, so put it there.
            this->child = *found;
            this->next = (*found)->next;
            *found = this;
            break;
        } else if ((*found)->child == NULL && 
                    (*found)->end <= this->start) {
            DBG(printf("It goes at the end. %p\n", *found);)
           // We've reached the end of the line, and that's where it
            // goes, so put it there.
            (*found)->child = this;
            break;
        }
        DBG(printf("What: %p\n", *found);)
        found = &((*found)->child);
    }
    DBG(printf("What: %p\n", *found);)

    // We found no place for the fragment, which means it's a duplicate
    // (or the chain is screwed up...)
    if (*found == NULL) {
        DBG(printf("No place for fragment: %p\n", *found);)
        free(this);
        return NULL;
    }

    // Now we try to collapse the list.
    found = curr;
    while ((*found != NULL) && (*found)->child != NULL) {
        ip_fragment * child = (*found)->child;
        if ((*found)->end == child->start) {
            DBG(printf("Merging frag at offset %u-%u with %u-%u\n", 
                        (*found)->start, (*found)->end,
                        child->start, child->end);)
            uint32_t child_len = child->end - child->start;
            uint32_t fnd_len = (*found)->end - (*found)->start;
            uint8_t * buff = malloc(sizeof(uint8_t) * (fnd_len + child_len));
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

    DBG(printf("*curr, start: %u, end: %u, islast: %u\n", 
                (*curr)->start, (*curr)->end, (*curr)->islast);)
    // Check to see if we completely collapsed it.
    // *curr is the pointer to the first fragment.
    if ((*curr)->islast != 0) {
        ip_fragment * ret = *curr;
        // Remove this from the fragment list.
        *curr = (*curr)->next;
        DBG(printf("Returning reassembled fragments.\n");)
        return ret;
    }
    // This is what happens when we don't complete a packet.
    return NULL;
}
*/
// Parse the IPv4 header. May point p_packet to a new packet data array,
// which means zero is a valid return value. Sets p_packet to NULL on error.
// See RFC791
uint32_t ipv4_parse(uint32_t pos, struct pcap_pkthdr *header, 
                    uint8_t ** p_packet, ip_info * ip) {

    uint32_t h_len;
    ip_fragment * frag = NULL;
    uint8_t frag_mf;
    uint16_t frag_offset;

    // For convenience and code consistency, dereference the packet **.
    uint8_t * packet = *p_packet;

    if (header-> len - pos < 20) {
        fprintf(stderr, "Truncated Packet(ipv4)\n");
        *p_packet = NULL;
        return 0;
    }

    h_len = packet[pos] & 0x0f;
    ip->length = (packet[pos+2] << 8) + packet[pos+3] - h_len*4;
    ip->proto = packet[pos+9];

    IPv4_MOVE(ip->src, packet + pos + 12);
    IPv4_MOVE(ip->dst, packet + pos + 16);

    // Set if NOT the last fragment.
    frag_mf = (packet[pos+6] & 0x20) >> 5;
    // Offset for this data in the fragment.
    frag_offset = ((packet[pos+6] & 0x1f) << 11) + (packet[pos+7] << 3);

    // SHOW_RAW(
    //     printf("\nipv4\n");
    //     print_packet(header->len, packet, pos, pos + 4*h_len, 4);
    // )
    // VERBOSE(
    //     printf("version: %d, length: %d, proto: %d\n", 
    //             IPv4, ip->length, ip->proto);
    //     printf("src ip: %s, ", iptostr(&ip->src));
    //     printf("dst ip: %s\n", iptostr(&ip->dst));
    // )

    if (frag_mf == 1 || frag_offset != 0) {
        //VERBOSE(printf("Fragmented IPv4, offset: %u, mf:%u\n", frag_offset,
          //                                                     frag_mf);)
        frag = (ip_fragment *) malloc(sizeof(ip_fragment));
        frag->start = frag_offset;
        // We don't try to deal with endianness here, since it 
        // won't matter as long as we're consistent.
        frag->islast = !frag_mf;
        frag->id = *((uint16_t *)(packet + pos + 4));
        frag->src = ip->src;
        frag->dst = ip->dst;
        frag->end = frag->start + ip->length;
        frag->data = (uint8_t *) malloc(sizeof(uint8_t) * ip->length);
        frag->next = frag->child = NULL;
        memcpy(frag->data, packet + pos + 4*h_len, ip->length);
        // Add the fragment to the list.
        // If this completed the packet, it is returned.
        /*
        frag = ip_frag_add(frag, conf); 
        if (frag != NULL) {
            // Update the IP info on the reassembled data.
            header->len = ip->length = frag->end - frag->start;
            *p_packet = frag->data;
            free(frag);
            return 0;
        }
        */
        // Signals that there is no more work to do on this packet.
        *p_packet = NULL;
        return 0;
    } 

    // move the position up past the options section.
    return pos + 4*h_len;

}

// Parse the IPv6 header. May point p_packet to a new packet data array,
// which means zero is a valid return value. Sets p_packet to NULL on error.
// See RFC2460
uint32_t ipv6_parse(uint32_t pos, struct pcap_pkthdr *header,
                    uint8_t ** p_packet, ip_info * ip) {

    // For convenience and code consistency, dereference the packet **.
    uint8_t * packet = *p_packet;

    // In case the IP packet is a fragment.
    ip_fragment * frag = NULL;
    uint32_t header_len = 0;

    if (header->len < (pos + 40)) {
        fprintf(stderr, "Truncated Packet(ipv6)\n");
        *p_packet=NULL; return 0;
    }
    ip->length = (packet[pos+4] << 8) + packet[pos+5];
    IPv6_MOVE(ip->src, packet + pos + 8);
    IPv6_MOVE(ip->dst, packet + pos + 24);

    // Jumbo grams will have a length of zero. We'll choose to ignore those,
    // and any other zero length packets.
    if (ip->length == 0) {
        fprintf(stderr, "Zero Length IP packet, possible Jumbo Payload.\n");
        *p_packet=NULL; 
        return 0;
    }

    uint8_t next_hdr = packet[pos+6];
    // VERBOSE(print_packet(header->len, packet, pos, pos+40, 4);)
    // VERBOSE(printf("IPv6 src: %s, ", iptostr(&ip->src));)
    // VERBOSE(printf("IPv6 dst: %s\n", iptostr(&ip->dst));)
    pos += 40;
   
    // We pretty much have no choice but to parse all extended sections,
    // since there is nothing to tell where the actual data is.
    uint8_t done = 0;
    while (done == 0) {
        //VERBOSE(printf("IPv6, next header: %u\n", next_hdr);)
        switch (next_hdr) {
            // Handle hop-by-hop, dest, and routing options.
            // Yay for consistent layouts.
            case IPPROTO_HOPOPTS:
            case IPPROTO_DSTOPTS:
            case IPPROTO_ROUTING:
                if (header->len < (pos + 16)) {
                    fprintf(stderr, "Truncated Packet(ipv6)\n");
                    *p_packet = NULL; return 0;
                }
                next_hdr = packet[pos];
                // The headers are 16 bytes longer.
                header_len += 16;
                pos += packet[pos+1] + 1;
                break;
            case 51: // Authentication Header. See RFC4302
                if (header->len < (pos + 2)) {
                    fprintf(stderr, "Truncated Packet(ipv6)\n");
                    *p_packet = NULL; return 0;
                } 
                next_hdr = packet[pos];
                header_len += (packet[pos+1] + 2) * 4;
                pos += (packet[pos+1] + 2) * 4;
                if (header->len < pos) {
                    fprintf(stderr, "Truncated Packet(ipv6)\n");
                    *p_packet = NULL; return 0;
                } 
                break;
            case 50: // ESP Protocol. See RFC4303.
                // We don't support ESP.
                fprintf(stderr, "Unsupported protocol: IPv6 ESP.\n");
                if (frag != NULL) free(frag);
                *p_packet = NULL; return 0;
            case 135: // IPv6 Mobility See RFC 6275
                if (header->len < (pos + 2)) {
                    fprintf(stderr, "Truncated Packet(ipv6)\n");
                    *p_packet = NULL; return 0;
                }  
                next_hdr = packet[pos];
                header_len += packet[pos+1] * 8;
                pos += packet[pos+1] * 8;
                if (header->len < pos) {
                    fprintf(stderr, "Truncated Packet(ipv6)\n");
                    *p_packet = NULL; return 0;
                } 
                break;
            case IPPROTO_FRAGMENT:
                // IP fragment.
                next_hdr = packet[pos];
                frag = (ip_fragment *) malloc(sizeof(ip_fragment));
                // Get the offset of the data for this fragment.
                frag->start = (packet[pos+2] << 8) + (packet[pos+3] & 0xf4);
                frag->islast = !(packet[pos+3] & 0x01);
                // We don't try to deal with endianness here, since it 
                // won't matter as long as we're consistent.
                frag->id = *(uint32_t *)(packet+pos+4);
                // The headers are 8 bytes longer.
                header_len += 8;
                pos += 8;
                break;
            case 0x06:
            case 0x11:
                done = 1; 
                break;
            default:
                fprintf(stderr, "Unsupported IPv6 proto(%u).\n", next_hdr);
                *p_packet = NULL; return 0;
        }
    }

    // check for int overflow
    if (header_len > ip->length) {
      fprintf(stderr, "Malformed packet(ipv6)\n");
      *p_packet = NULL;
      return 0;
    }

    ip->proto = next_hdr;
    ip->length = ip->length - header_len;

    // Handle fragments.
    if (frag != NULL) {
        frag->src = ip->src;
        frag->dst = ip->dst;
        frag->end = frag->start + ip->length;
        frag->next = frag->child = NULL;
        frag->data = (uint8_t *) malloc(sizeof(uint8_t) * ip->length);
        //VERBOSE(printf("IPv6 fragment. offset: %d, m:%u\n", frag->start,
        //                                                    frag->islast);)
        memcpy(frag->data, packet+pos, ip->length);
        // Add the fragment to the list.
        // If this completed the packet, it is returned.
        /*
        frag = ip_frag_add(frag, conf); 
        if (frag != NULL) {
            header->len = ip->length = frag->end - frag->start;
            *p_packet = frag->data;
            free(frag);
            return 0;
        }
        */
        // Signals that there is no more work to do on this packet.
        *p_packet = NULL;
        return 0;
    } else {
        return pos;
    }

}

// Parse the dns protocol in 'packet'. 
// See RFC1035
// See dns_parse.h for more info.
uint32_t dns_parse(uint32_t pos, struct pcap_pkthdr *header, 
                   uint8_t *packet, dns_info * dns, uint8_t force) {

    uint32_t id_pos = pos;

    if (header->len - pos < 12) {
       // char * msg = escape_data(packet, id_pos, header->len);
        fprintf(stderr, "Truncated Packet(dns): \n"); 
        return 0;
    }

    dns->id = (packet[pos] << 8) + packet[pos+1];
    dns->qr = packet[pos+2] >> 7;
    dns->AA = (packet[pos+2] & 0x04) >> 2;
    dns->TC = (packet[pos+2] & 0x02) >> 1;
    dns->rcode = packet[pos + 3] & 0x0f;
    // rcodes > 5 indicate various protocol errors and redefine most of the 
    // remaining fields. Parsing this would hurt more than help. 
    if (dns->rcode > 5) {
        dns->qdcount = dns->ancount = dns->nscount = dns->arcount = 0;
        dns->queries = NULL;
        dns->answers = NULL;
        dns->name_servers = NULL;
        dns->additional = NULL;
        return pos + 12;
    }

    // Counts for each of the record types.
    dns->qdcount = (packet[pos+4] << 8) + packet[pos+5];
    dns->ancount = (packet[pos+6] << 8) + packet[pos+7];
    dns->nscount = (packet[pos+8] << 8) + packet[pos+9];
    dns->arcount = (packet[pos+10] << 8) + packet[pos+11];

    // SHOW_RAW(
    //     printf("dns\n");
    //     print_packet(header->len, packet, pos, header->len, 8);
    // )
    // VERBOSE(
    //     printf("DNS id:%d, qr:%d, AA:%d, TC:%d, rcode:%d\n", 
    //            dns->id, dns->qr, dns->AA, dns->TC, dns->rcode);
    //     printf("DNS qdcount:%d, ancount:%d, nscount:%d, arcount:%d\n",
    //            dns->qdcount, dns->ancount, dns->nscount, dns->arcount);
    // )

    // Parse each type of records in turn.
    pos = parse_questions(pos+12, id_pos, header, packet, 
                          dns->qdcount, &(dns->queries));
    if (pos != 0) 
        pos = parse_rr_set(pos, id_pos, header, packet, 
                           dns->ancount, &(dns->answers), conf);
    else dns->answers = NULL;
    if (pos != 0 && 
        (conf->NS_ENABLED || conf->AD_ENABLED || force)) {
        pos = parse_rr_set(pos, id_pos, header, packet, 
                           dns->nscount, &(dns->name_servers), conf);
    } else dns->name_servers = NULL;
    if (pos != 0 && (conf->AD_ENABLED || force)) {
        pos = parse_rr_set(pos, id_pos, header, packet, 
                           dns->arcount, &(dns->additional), conf);
    } else dns->additional = NULL;
    return pos;
}

// Parse the udp headers.
uint32_t udp_parse(uint32_t pos, struct pcap_pkthdr *header, 
                   uint8_t *packet, transport_info * udp) {
    if (header->len - pos < 8) {
        fprintf(stderr, "Truncated Packet(udp)\n");
        return 0;
    }

    udp->srcport = (packet[pos] << 8) + packet[pos+1];
    udp->dstport = (packet[pos+2] << 8) + packet[pos+3];
    udp->length = (packet[pos+4] << 8) + packet[pos+5];
    udp->transport = 0x11;
   // VERBOSE(printf("udp\n");)
    //VERBOSE(printf("srcport: %d, dstport: %d, len: %d\n", udp->srcport, udp->dstport, udp->length);)
   // SHOW_RAW(print_packet(header->len, packet, pos, pos, 4);)
    return pos + 8;
}

// This method is called in pcap_loop for each packet
void pcapHandler(uint8_t * args, const struct pcap_pkthdr *orig_header, const uint8_t *orig_packet)
{
	int pos;
    uint16_t ethtype;
    ip_info ip;
    //config* conf = (config *) args;

    // The way we handle IP fragments means we may have to replace
    // the original data and correct the header info, so a const won't work.
    uint8_t * packet = (uint8_t *) orig_packet;
    struct pcap_pkthdr header;
    header.ts = orig_header->ts;
    header.caplen = orig_header->caplen;
    header.len = orig_header->len;
    
    //cerr << "\nPacket %llu.%llu\n", 
      //             (uint64_t)header.ts.tv_sec, 
        //           (uint64_t)header.ts.tv_usec);)
    
    // Parse the ethernet frame. Errors are typically handled in the parser
    // functions. The functions generally return 0 on error.
    pos = eth_parse(&header, packet, &ethtype);
    if (pos == 0) return;

    // MPLS parsing is simple, but leaves us to guess the next protocol.
    // We make our guess in the MPLS parser, and set the ethtype accordingly.
    if (ethtype == 0x8847) {
        pos = mpls_parse(pos, &header, packet, &ethtype);
    } 

    // IP v4 and v6 parsing. These may replace the packet byte array with 
    // one from reconstructed packet fragments. Zero is a reasonable return
    // value, so they set the packet pointer to NULL on failure.
    if (ethtype == 0x0800) {
        pos = ipv4_parse(pos, &header, &packet, &ip);
    } else if (ethtype == 0x86DD) {
        pos = ipv6_parse(pos, &header, &packet, &ip);
    } else {
        fprintf(stderr, "Unsupported EtherType: %04x\n", ethtype);
        return;
    }
    if (packet == NULL) return;

    // Transport layer parsing. 
    if (ip.proto == 17) {
        // Parse the udp and this single bit of DNS, and output it.
        dns_info dns;
        transport_info udp;
        pos = udp_parse(pos, &header, packet, &udp);
        if ( pos == 0 ) return;
        /*
        // Only do deduplication if DEDUPS > 0.
        if (conf->DEDUPS != 0 ) {
            if (dedup(pos, &header, packet, &ip, &udp, conf) == 1) {
                // A duplicate packet.
                return;
            }
        }
        */
        pos = dns_parse(pos, &header, packet, &dns, !FORCE);
        print_summary(&ip, &udp, &dns, &header, conf);
    } else if (ip.proto == 6) {
        // Hand the tcp packet over for later reconstruction.
        tcp_parse(pos, &header, packet, &ip, conf); 
    } else {
        fprintf(stderr, "Unsupported Transport Protocol(%d)\n", ip.proto);
        return;
    }
   
    if (packet != orig_packet) {
        // Free data from artificially constructed packets.
        free(packet);
    }

    // Expire tcp sessions, and output them if possible.
    DBG(printf("Expiring TCP.\n");)
    tcp_expire(conf, &header.ts);
}

}


void logFile(string strFile, string strLog)
{
	char errBuf[PCAP_ERRBUF_SIZE];
	pcap_t* pcapFile;
	if ((pcapFile = pcap_open_offline(strFile.c_str(), errBuf)) == NULL)
		ERR_RET("Unable to open pcap file. Terminating.");

	if (pcap_loop(pcapFile, 0, pcapHandler, NULL) < 0)
		ERR_RET("Error occured when reading pcap file. Terminating.");

}


/*********************************************************** MAIN ***************************************************************/

// Main function
int main(int argc, char const *argv[])
{
    // Handle SIUSR1 
    signal(SIGUSR1, handleSig);

	// Parse arguments
	raw_parameters params = parseArg(argc, argv);

	if (params.interface.compare(""))
	{
		logInterface(params.interface, params.syslogServer, params.time);
	}

	if (params.file.compare(""))
	{
		logFile(params.file, params.syslogServer);
	}


	return 0;
}
