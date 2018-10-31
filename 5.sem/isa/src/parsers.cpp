#include "parsers.h"
#include "structures.h"
#include "rr_parsers.cpp"

using namespace std;

rrDataParser opts;

////////////////////////////////////////////////////////////////

uint32_t getTransProt(uint32_t* currentPos, const uint8_t* packet)
{
    struct ether_header *eptr;
    struct ip* myIp;
    struct ip6_hdr* myIpv6;

    // read the Ethernet header
    eptr = (struct ether_header*) packet;
    *currentPos += sizeof(struct ether_header);

    int protType;

    // parse ethernet type 
    switch (ntohs(eptr->ether_type))
    {               
        // see /usr/include/net/ethernet.h for types
        case ETHERTYPE_IP:
            LOGGING("Ethernet type is IPv4");
            myIp = (struct ip*) (packet + *currentPos);              // skip Ethernet header
            *currentPos += sizeof(struct ip);
            protType = myIp->ip_p;
            break;

        case ETHERTYPE_IPV6:
            LOGGING("Ethernet type is IPv6");
            myIpv6 = (struct ip6_hdr*) (packet + *currentPos);
            *currentPos += sizeof(struct ip6_hdr);
            protType = myIpv6->ip6_ctlun.ip6_un1.ip6_un1_nxt;
            break;
        default:
            LOGGING("Invalid ethernet type for DNS");
            return 0;
    }

    switch (protType)
    {
        case UDP:
            LOGGING("Transport protocol UDP");
            *currentPos += sizeof(struct udphdr);
            return UDP;
        case TCP: 
            LOGGING("Transport protocol TCP");
            *currentPos += sizeof(struct tcphdr);
            return TCP;
        default: 
            LOGGING("Invalid transport protocol for DNS");
            return 0;
    }
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
        return 0;
    }

    // Counts for each of the record types.
    int qdCount = (packet[pos+4] << 8) + packet[pos+5];
    dns->ancount = (packet[pos+6] << 8) + packet[pos+7];

    // Skip questions
    pos = pos + 12 + (qdCount * 4);

    // Parse answer records
    pos = parseRRSet(pos, idPos, header, packet, dns->ancount, &(dns->answers));

    return pos;
}

// Parse a set of resource records in the dns protocol in 'packet', starting
// at 'pos'. The 'idPos' offset is necessary for putting together 
// compressed names. 'count' is the expected number of records of this type.
// 'root' is where to assign the parsed list of objects.
// Return 0 on error, the new 'pos' in the packet otherwise.
uint32_t parseRRSet(uint32_t pos, uint32_t idPos, struct pcap_pkthdr* header, uint8_t* packet, uint16_t count, list<dnsRR*>* dnsRRList)
{
    for (int i = 0; i < count; i++)
    {
        // Create and clear the data in a new dnsRR object.
        dnsRR* current = (dnsRR*) malloc(sizeof(dnsRR));
        current->next = NULL; 
        current->name = NULL; 
        current->data = NULL;

        pos = parseRR(pos, idPos, header, packet, current);

        // If a non-recoverable error occurs when parsing an rr, 
        // we can only return what we've got and give up.
        if (pos == 0)
        {
            LOGGING("Error occured when RR parsing")
            return 0;
        }
        (*dnsRRList).push_front(current);
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
    rrParserContainer* parser;
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

