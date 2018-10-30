#include "parsers.h"
#include "structures.h"
#include "rr_parsers.cpp"

using namespace std;

rrDataParser opts;

////////////////////////////////////////////////////////////////

// Parse the ethernet header
// Returns the payload position (-1 on error)
uint32_t ethParse(struct pcap_pkthdr* header, uint8_t* packet, uint16_t* ethType, int datalink)
{
    if (header->len < 14)
    {
        return 0;
    }

    // Skip mac adresses
    uint32_t pos = 12;

    // Skip the extra 2 byte field inserted in "Linux Cooked" captures
    if (datalink == DLT_LINUX_SLL)
    {
        pos += 2;
    }

    // Skip VLAN tagging if needed
    if (packet[pos] == VLAN_TAGGED && packet[pos + 1] == 0) 
    {
        pos += 4;
    }

    *ethType = (packet[pos] << 8) + packet[pos + 1];
    pos += 2;

    return pos;
}

// Parse MPLS. We don't care about the data in these headers, all we have
// to do is continue parsing them until the 'bottom of stack' flag is set.
//
// It is bottom-of-stack bit, to indicate one more label. Else the data is popped out of MPLS headers 
// and given to next level engine to Handle (it will typically contain a L2 or L3/IP header), 
// it is done while the programming that correct engine is associated.
uint32_t mplsParse(struct pcap_pkthdr* header, uint8_t* packet, uint16_t* ethType, uint32_t pos)
{
    // Bottom of stack flag.
    bool bottomFlag = false;
    while (bottomFlag == false)
    {
        if (header->len < (pos + 4))
        {
            return 0;
        }

        bottomFlag = (packet[pos + 2] == 1);
        pos += 4;
    }

    if (header->len < pos)
    {
        return 0;
    }

    // 'Guess' the next protocol. This can result in false positives, but generally not
    int ip_ver = packet[pos] >> 4;
    switch (ip_ver)
    {
        case IPv4:
            *ethType = IPv4Prot; 
            break;
        case IPv6:
            *ethType =IPv6Prot;
             break;
        default:
            *ethType = 0;
    }

    return pos;
}

// Parse the IPv4 header. May point p_packet to a new packet data array,
// which means zero is a valid return value. Sets p_packet to NULL on error.
// See RFC791
uint32_t ipv4Parse(uint32_t pos, struct pcap_pkthdr* header, uint8_t* packet, ipInfo* ip)
{
    uint32_t h_len;
    uint8_t frag_mf;
    uint16_t frag_offset;

    if (header->len - pos < 20)
    {
        LOGGING("Truncated Packet(ipv4). Skipping");
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

    // SKip fragmentation 
    if (frag_mf == 1 || frag_offset != 0) {
        
        return 0;
    } 

    // move the position up past the options section.
    return pos + 4*h_len;
}

// Parse the IPv6 header. May point p_packet to a new packet data array,
// which means zero is a valid return value. Sets p_packet to NULL on error.
// See RFC2460
uint32_t ipv6Parse(uint32_t pos, struct pcap_pkthdr* header, uint8_t* packet, ipInfo* ip)
{
    // In case the IP packet is a fragment.
    uint32_t header_len = 0;

    if (header->len < (pos + 40))
    {
        LOGGING("Truncated Packet(ipv6). Skipping");
        return 0;
    }

    ip->length = (packet[pos+4] << 8) + packet[pos+5];

    IPv6_MOVE(ip->src, packet + pos + 8);
    IPv6_MOVE(ip->dst, packet + pos + 24);

    // Jumbo grams will have a length of zero. We'll choose to ignore those,
    // and any other zero length packets.
    if (ip->length == 0)
    {
        LOGGING("Zero Length IP packet, possible Jumbo Payload. Skipping.");
        return 0;
    }

    uint8_t next_hdr = packet[pos+6];
    pos += 40;
   
    // We pretty much have no choice but to parse all extended sections,
    // since there is nothing to tell where the actual data is.
    bool done = false;

    while (done == false)
    {
        LOGGING("IPv6, next header: " << next_hdr);
        switch (next_hdr)
        {
            // Handle hop-by-hop, dest, and routing options.
            // Yay for consistent layouts.
            case IPPROTO_HOPOPTS:
            case IPPROTO_DSTOPTS:
            case IPPROTO_ROUTING:
                if (header->len < (pos + 16))
                {
                    LOGGING("Truncated Packet(ipv6). Skipping");
                    return 0;
                }

                next_hdr = packet[pos];
                // The headers are 16 bytes longer.
                header_len += 16;
                pos += packet[pos+1] + 1;
                break;
            case 51: // Authentication Header. See RFC4302
                if (header->len < (pos + 2))
                {
                    LOGGING("Truncated Packet(ipv6). Skipping");
                    return 0;
                }

                next_hdr = packet[pos];
                header_len += (packet[pos+1] + 2) * 4;
                pos += (packet[pos+1] + 2) * 4;

                if (header->len < pos)
                {
                    LOGGING("Truncated Packet(ipv6). Skipping");
                    return 0;
                } 
                break;
            case 50: // ESP Protocol. See RFC4303.
                // We don't support ESP.
                LOGGING("Unsuported ESP protocol. Skipping");
                return 0;
            case 135: // IPv6 Mobility See RFC 6275
                if (header->len < (pos + 2))
                {
                    LOGGING("Truncated Packet(ipv6). Skipping");
                    return 0;
                }  

                next_hdr = packet[pos];
                header_len += packet[pos+1] * 8;
                pos += packet[pos+1] * 8;
                if (header->len < pos)
                {
                    LOGGING("Truncated Packet(ipv6). Skipping");
                    return 0;
                } 
                break;
            case IPPROTO_FRAGMENT:
                // IP fragment, skipping
                return 0;
            case TCP:
            case UDP:
                done = true; 
                break;
            default:
                LOGGING("Unsupported IPv6 proto: " << next_hdr);
                return 0;
        }
    }

    // check for int overflow
    if (header_len > ip->length)
    {
        LOGGING("Malformed packet(ipv6). Skipping");
        return 0;
    }

    ip->proto = next_hdr;
    ip->length = ip->length - header_len;

    return pos;
}

// Parse the dns protocol in 'packet'
// See RFC1035
// See dns_parse.h for more info
uint32_t dnsParse(uint32_t pos, struct pcap_pkthdr* header, uint8_t* packet, dnsInfo* dns, uint8_t force)
{
    uint32_t idPos = pos;

    if (header->len - pos < 12)
    {
        LOGGING("Truncated Packet(dns). Skipping");
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
        LOGGING("Rcode > 5. Skipping");
        return 0;;
    }

    // Counts for each of the record types.
    dns->qdcount = (packet[pos+4] << 8) + packet[pos+5];
    dns->ancount = (packet[pos+6] << 8) + packet[pos+7];
    dns->nscount = (packet[pos+8] << 8) + packet[pos+9];
    dns->arcount = (packet[pos+10] << 8) + packet[pos+11];

    // Parse each type of records in turn
    pos = parseQuestions(pos + 12, idPos, header, packet, dns->qdcount, &(dns->queries));

    if (pos != 0)
    {
        pos = parseRRSet(pos, idPos, header, packet, dns->ancount, &(dns->answers));
    }
    else
    {
        dns->answers = NULL;
    }

    if (pos != 0 && force)
    {
        pos = parseRRSet(pos, idPos, header, packet, dns->nscount, &(dns->nameServers));
    }
    else
    {
        dns->nameServers = NULL;
    }

    if (pos != 0 && force)
    {
        pos = parseRRSet(pos, idPos, header, packet, dns->arcount, &(dns->additional));
    }
    else
    {
        dns->additional = NULL; 
    }

    return pos;
}

// Parse the questions section of the dns protocol
// pos - offset to the start of the questions section
// idPos - offset set to the id field. Needed to decompress dns data
// packet, header - the packet location and header data
// count - Number of question records to expect
// root - Pointer to where to store the question records
uint32_t parseQuestions(uint32_t pos, uint32_t idPos, struct pcap_pkthdr* header, uint8_t* packet, uint16_t count, dnsQuestion** root)
{
    uint32_t startPos = pos;
    dnsQuestion* last = NULL;
    dnsQuestion* current;
    uint16_t i;
    *root = NULL;

    for (i = 0; i < count; i++)
    {
        current = (dnsQuestion*) malloc(sizeof(dnsQuestion));
        current->next = NULL; 
        current->name = NULL;

        current->name = readRRName(packet, &pos, idPos, header->len);

        if (current->name == NULL || (pos + 2) >= header->len)
        {
            // Handle a bad DNS name.
            LOGGING("DNS question error. Skipping");
            current->type = 0;
            current->cls = 0;
            if (last == NULL)
            {
                *root = current;
            }
            else 
            {
                last->next = current;
            }
            return 0;
        }

        current->type = (packet[pos] << 8) + packet[pos+1];
        current->cls = (packet[pos+2] << 8) + packet[pos+3];

        // Add this question object to the list.
        if (last == NULL)
        {
            *root = current;
        }
        else 
        {
            last->next = current;
        }
        last = current;
        pos = pos + 4;
   }

    return pos;
}

// Parse a set of resource records in the dns protocol in 'packet', starting
// at 'pos'. The 'idPos' offset is necessary for putting together 
// compressed names. 'count' is the expected number of records of this type.
// 'root' is where to assign the parsed list of objects.
// Return 0 on error, the new 'pos' in the packet otherwise.
uint32_t parseRRSet(uint32_t pos, uint32_t idPos, struct pcap_pkthdr *header, uint8_t *packet, uint16_t count, dnsRR** root)
{
    dnsRR* last = NULL;
    dnsRR* current;
    uint16_t i;
    *root = NULL;

    for (i = 0; i < count; i++)
    {
        // Create and clear the data in a new dnsRR object.
        current = (dnsRR*) malloc(sizeof(dnsRR));
        current->next = NULL; 
        current->name = NULL; 
        current->data = NULL;

        pos = parseRR(pos, idPos, header, packet, current);

        // If a non-recoverable error occurs when parsing an rr, 
        // we can only return what we've got and give up.
        if (pos == 0)
        {
            if (last == NULL) *root = current;
            else last->next = current;
            return 0;
        }
        if (last == NULL) 
        {
            *root = current;
        }
        else 
        {
            last->next = current;
        }
        last = current;
    }
    return pos;
}

// Parse an individual resource record, placing the acquired data in 'rr'.
// 'packet', 'pos', and 'idPos' serve the same uses as in parse_rr_set.
// Return 0 on error, the new 'pos' in the packet otherwise.
uint32_t parseRR(uint32_t pos, uint32_t idPos, struct pcap_pkthdr* header, uint8_t* packet, dnsRR* rr)
{
    int i;
    uint32_t rr_start = pos;
    rrParserContainer * parser;
    rrParserContainer opts_cont = {0,0, opts};

    rr->name = NULL;
    rr->data = NULL;

    rr->name = readRRName(packet, &pos, idPos, header->len);
    
    // Handle a bad rr name.
    // We still want to print the rest of the escaped rr data.
    if (rr->name == NULL)
    {
        LOGGING("Bad rr name");
        return 0;
    }

    if ((header->len - pos) < 10)
    {
        return 0;
    }

    rr->type = (packet[pos] << 8) + packet[pos+1];
    rr->rdlength = (packet[pos+8] << 8) + packet[pos + 9];

    // Handle edns opt RR's differently.
    if (rr->type == 41)
    {
        rr->cls = 0;
        rr->ttl = 0;
        rr->rrName = "OPTS";
        parser = &opts_cont;
        // We'll leave the parsing of the special EDNS opt fields to
        // our opt rdata parser.  
        pos = pos + 2;
    }
    else
    {
        // The normal case.
        rr->cls = (packet[pos+2] << 8) + packet[pos+3];
        rr->ttl = 0;
        for (i=0; i<4; i++)
        {
            rr->ttl = (rr->ttl << 8) + packet[pos+4+i];
        }

        // Retrieve the correct parser function.
        parser = findParser(rr->cls, rr->type);
        rr->rrName = parser->name;
        pos = pos + 10;
    }

    LOGGING("Applying RR parser: " << parser->name);

    if (&default_rr_parser == parser) 
    {
        LOGGING("Missing parser for class :" << rr->cls << " type :" << rr->type);
    }

    // Make sure the data for the record is actually there.
    // If not, escape and print the raw data.
    if (header->len < (rr_start + 10 + rr->rdlength))
    {
        LOGGING("Truncated rr");
        return 0;
    }

    // Parse the resource record data.
    rr->data = parser->parser(packet, pos, idPos, rr->rdlength, header->len);
    
    // fprintf(stderr, "rr->name: %s\n", rr->name);
    // fprintf(stderr, "type %d, cls %d, ttl %d, len %d\n", rr->type, rr->cls, rr->ttl,
    //        rr->rdlength);
    // fprintf(stderr, "rr->data %s\n", rr->data);

    return pos + rr->rdlength;
}



// Parse the udp headers.
uint32_t udpParse(uint32_t pos, struct pcap_pkthdr* header, uint8_t* packet, transportInfo* udp)
{
    if (header->len - pos < 8)
    {
        fprintf(stderr, "Truncated Packet(udp)\n");
        return 0;
    }

    udp->srcport = (packet[pos] << 8) + packet[pos+1];
    udp->dstport = (packet[pos+2] << 8) + packet[pos+3];
    udp->length = (packet[pos+4] << 8) + packet[pos+5];
    udp->transport = UDP;
    
    return pos + 8;
}
