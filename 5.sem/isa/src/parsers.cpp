#include "parsers.h"
#include "structures.h"
#include "helpers.cpp"

using namespace std;

// This is for handling rr's with errors or an unhandled rtype.
rr_parser_container default_rr_parser;

// Add them to the list of data parsers in rtypes.c.
extern rr_parser_container rr_parsers[];

rr_data_parser opts;

struct ip_fragment* ip_fragment_head = NULL;

////////////////////////////////////////////////////////////////

// Parse the ethernet header
// Returns the payload position (-1 on error)
uint32_t ethParse(struct pcap_pkthdr* header, uint8_t* packet, uint16_t* eth_type, int datalink)
{

    if (header->len < 14)
    {
        return 0;
    }

    uint32_t pos = 12;

    // Skip the extra 2 byte field inserted in "Linux Cooked" captures
    if (datalink == DLT_LINUX_SLL)
    {
        pos += 2;
    }

    // Skip VLAN tagging 
    if (packet[pos] == 0x81 && packet[pos+1] == 0) 
    {
        pos += 4;
    }

    *eth_type = (packet[pos] << 8) + packet[pos+1];
    pos += 2;

    return pos;
}

// Parse MPLS. We don't care about the data in these headers, all we have
// to do is continue parsing them until the 'bottom of stack' flag is set.
uint32_t mplsParse(uint32_t pos, struct pcap_pkthdr* header, uint8_t* packet, uint16_t* eth_type)
{

    // Bottom of stack flag.
    uint8_t bos;
    do
    {
        if (header->len < (pos + 4))
        {
            return 0;
        }

        bos = packet[pos + 2] & 0x01;
        pos += 4;
    } while (bos == 0);

    if (header->len < pos)
    {
        return 0;
    }

    // 'Guess' the next protocol. This can result in false positives, but generally not
    uint8_t ip_ver = packet[pos] >> 4;
    switch (ip_ver)
    {
        case IPv4:
            *eth_type = 0x0800; 
            break;
        case IPv6:
            *eth_type = 0x86DD;
             break;
        default:
            *eth_type = 0;
    }

    return pos;
}

// Parse the IPv4 header. May point p_packet to a new packet data array,
// which means zero is a valid return value. Sets p_packet to NULL on error.
// See RFC791
uint32_t ipv4Parse(uint32_t pos, struct pcap_pkthdr* header, uint8_t* packet, ip_info* ip)
{
    uint32_t h_len;
    ip_fragment* frag = NULL;
    uint8_t frag_mf;
    uint16_t frag_offset;

    if (header->len - pos < 20)
    {
        LOGGING("Truncated Packet(ipv4). Skipping");
        packet = NULL;
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
        //                                                      frag_mf);)
        frag = (ip_fragment*) malloc(sizeof(ip_fragment));
        frag->start = frag_offset;
        // We don't try to deal with endianness here, since it 
        // won't matter as long as we're consistent.
        frag->islast = !frag_mf;
        frag->id = *((uint16_t *)(packet + pos + 4));
        frag->src = ip->src;
        frag->dst = ip->dst;
        frag->end = frag->start + ip->length;
        frag->data = (uint8_t*) malloc(sizeof(uint8_t) * ip->length);
        frag->next = frag->child = NULL;
        memcpy(frag->data, packet + pos + 4*h_len, ip->length);
        
        // Add the fragment to the list.
        // If this completed the packet, it is returned.
        frag = ipFragAdd(frag, ip_fragment_head);
        if (frag != NULL) {
            // Update the IP info on the reassembled data.
            header->len = ip->length = frag->end - frag->start;
            packet = frag->data;
            free(frag);
            return 0;
        }
        // Signals that there is no more work to do on this packet.
        packet = NULL;
        return 0;
    } 

    // move the position up past the options section.
    return pos + 4*h_len;
}

// Parse the IPv6 header. May point p_packet to a new packet data array,
// which means zero is a valid return value. Sets p_packet to NULL on error.
// See RFC2460
uint32_t ipv6Parse(uint32_t pos, struct pcap_pkthdr* header, uint8_t* packet, ip_info* ip)
{
    // In case the IP packet is a fragment.
    ip_fragment* frag = NULL;
    uint32_t header_len = 0;

    if (header->len < (pos + 40))
    {
        LOGGING("Truncated Packet(ipv6). Skipping");
        packet = NULL; 
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
        packet=NULL;
        return 0;
    }

    uint8_t next_hdr = packet[pos+6];
    // VERBOSE(print_packet(header->len, packet, pos, pos+40, 4);)
    // fprintf(stderr, "IPv6 src: %s, ", iptostr(&ip->src));
    // fprintf(stderr, "IPv6 dst: %s\n", iptostr(&ip->dst));
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
                    packet = NULL;
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
                    packet = NULL;
                    return 0;
                }

                next_hdr = packet[pos];
                header_len += (packet[pos+1] + 2) * 4;
                pos += (packet[pos+1] + 2) * 4;

                if (header->len < pos)
                {
                    LOGGING("Truncated Packet(ipv6). Skipping");
                    packet = NULL;
                    return 0;
                } 
                break;
            case 50: // ESP Protocol. See RFC4303.
                // We don't support ESP.
                LOGGING("Unsuported ESP protocol. Skipping");
                packet = NULL;
                return 0;
            case 135: // IPv6 Mobility See RFC 6275
                if (header->len < (pos + 2))
                {
                    LOGGING("Truncated Packet(ipv6). Skipping");
                    packet = NULL;
                    return 0;
                }  

                next_hdr = packet[pos];
                header_len += packet[pos+1] * 8;
                pos += packet[pos+1] * 8;
                if (header->len < pos)
                {
                    LOGGING("Truncated Packet(ipv6). Skipping");
                    packet = NULL;
                    return 0;
                } 
                break;
            case IPPROTO_FRAGMENT:
                // IP fragment.
                next_hdr = packet[pos];
                frag = (ip_fragment*) malloc(sizeof(ip_fragment));
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
            case TCP:
            case UDP:
                done = true; 
                break;
            default:
                LOGGING("Unsupported IPv6 proto: " << next_hdr);
                packet = NULL;
                return 0;
        }
    }

    // check for int overflow
    if (header_len > ip->length)
    {
        LOGGING("Malformed packet(ipv6). Skipping");
        packet = NULL;
        return 0;
    }

    ip->proto = next_hdr;
    ip->length = ip->length - header_len;

    // Handle fragments.
    if (frag != NULL)
    {
        frag->src = ip->src;
        frag->dst = ip->dst;
        frag->end = frag->start + ip->length;
        frag->next = frag->child = NULL;
        frag->data = (uint8_t*) malloc(sizeof(uint8_t) * ip->length);
        LOGGING("IPv6 fragment. offset: " << frag->start << " m: " << frag->islast);
        memcpy(frag->data, packet+pos, ip->length);
        // Add the fragment to the list.
        // If this completed the packet, it is returned.

        frag = ipFragAdd(frag, ip_fragment_head); 
        if (frag != NULL)
        {
            header->len = ip->length = frag->end - frag->start;
            packet = frag->data;
            free(frag);
            return 0;
        }

        // Signals that there is no more work to do on this packet.
        packet = NULL;
        return 0;
    }
    else
    {
        return pos;
    }
}

// Parse the dns protocol in 'packet'
// See RFC1035
// See dns_parse.h for more info
uint32_t dnsParse(uint32_t pos, struct pcap_pkthdr* header, uint8_t* packet, dns_info* dns, uint8_t force)
{
    uint32_t id_pos = pos;

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

    // Parse each type of records in turn
    pos = parseQuestions(pos + 12, id_pos, header, packet, dns->qdcount, &(dns->queries));

    if (pos != 0)
    {
        pos = parseRRSet(pos, id_pos, header, packet, dns->ancount, &(dns->answers));
    }
    else
    {
        dns->answers = NULL;
    }

    if (pos != 0 && force)
    {
        pos = parseRRSet(pos, id_pos, header, packet, dns->nscount, &(dns->name_servers));
    }
    else
    {
        dns->name_servers = NULL;
    }

    if (pos != 0 && force)
    {
        pos = parseRRSet(pos, id_pos, header, packet, dns->arcount, &(dns->additional));
    }
    else
    {
        dns->additional = NULL; 
    }

    return pos;
}

// Parse the questions section of the dns protocol
// pos - offset to the start of the questions section
// id_pos - offset set to the id field. Needed to decompress dns data
// packet, header - the packet location and header data
// count - Number of question records to expect
// root - Pointer to where to store the question records
uint32_t parseQuestions(uint32_t pos, uint32_t id_pos, struct pcap_pkthdr* header, uint8_t* packet, uint16_t count, dns_question** root)
{
    uint32_t start_pos = pos;
    dns_question* last = NULL;
    dns_question* current;
    uint16_t i;
    *root = NULL;

    for (i=0; i < count; i++)
    {
        current = (dns_question*) malloc(sizeof(dns_question));
        current->next = NULL; 
        current->name = NULL;

        current->name = readRRName(packet, &pos, id_pos, header->len);

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

        // VERBOSE(printf("question->name: %s\n", current->name);)
        // VERBOSE(printf("type %d, cls %d\n", current->type, current->cls);)
   }

    return pos;
}

// Parse a set of resource records in the dns protocol in 'packet', starting
// at 'pos'. The 'id_pos' offset is necessary for putting together 
// compressed names. 'count' is the expected number of records of this type.
// 'root' is where to assign the parsed list of objects.
// Return 0 on error, the new 'pos' in the packet otherwise.
uint32_t parseRRSet(uint32_t pos, uint32_t id_pos, struct pcap_pkthdr *header, uint8_t *packet, uint16_t count, dns_rr** root)
{
    dns_rr* last = NULL;
    dns_rr* current;
    uint16_t i;
    *root = NULL;

    for (i = 0; i < count; i++)
    {
        // Create and clear the data in a new dns_rr object.
        current = (dns_rr*) malloc(sizeof(dns_rr));
        current->next = NULL; 
        current->name = NULL; 
        current->data = NULL;

        pos = parseRR(pos, id_pos, header, packet, current);

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
// 'packet', 'pos', and 'id_pos' serve the same uses as in parse_rr_set.
// Return 0 on error, the new 'pos' in the packet otherwise.
uint32_t parseRR(uint32_t pos, uint32_t id_pos, struct pcap_pkthdr* header, uint8_t* packet, dns_rr* rr)
{
    int i;
    uint32_t rr_start = pos;
    rr_parser_container * parser;
    rr_parser_container opts_cont = {0,0, opts};

    rr->name = NULL;
    rr->data = NULL;

    rr->name = readRRName(packet, &pos, id_pos, header->len);
    
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
        rr->rr_name = "OPTS";
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
        rr->rr_name = parser->name;
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
    rr->data = parser->parser(packet, pos, id_pos, rr->rdlength, header->len);
    
    // fprintf(stderr, "rr->name: %s\n", rr->name);
    // fprintf(stderr, "type %d, cls %d, ttl %d, len %d\n", rr->type, rr->cls, rr->ttl,
    //        rr->rdlength);
    // fprintf(stderr, "rr->data %s\n", rr->data);

    return pos + rr->rdlength;
}

// Find the parser that corresponds to the given cls and rtype.
rr_parser_container* findParser(uint16_t cls, uint16_t rtype) {

    unsigned int i=0, pcount = count_parsers();
    rr_parser_container * found = NULL;
   
    // Re-arrange the order of the parsers according to how often things are 
    // seen every REORDER_LIMIT packets.
    if (PACKETS_SEEN > REORDER_LIMIT) {
        PACKETS_SEEN = 0;
        sort_parsers();
    } 
    PACKETS_SEEN++;

    while (i < pcount && found == NULL) {
        rr_parser_container pc = rr_parsers[i];
        if ((pc.rtype == rtype || pc.rtype == 0) &&
            (pc.cls == cls || pc.cls == 0)) {
            rr_parsers[i].count++;
            found = &rr_parsers[i];
            break;
        }
        i++;
    }

    if (found == NULL) 
        found = &default_rr_parser;
    
    found->count++;
    return found;
}

#define A_DOC "A (IPv4 address) format\n"\
"A records are simply an IPv4 address, and are formatted as such."
char * A(const uint8_t * packet, uint32_t pos, uint32_t i,
         uint16_t rdlength, uint32_t plen) {
    char * data = (char *)malloc(sizeof(char)*16);

    if (rdlength != 4) {
        free(data);
        return ("Bad A record: ");
    }
    
    sprintf(data, "%d.%d.%d.%d", packet[pos], packet[pos+1],
                                 packet[pos+2], packet[pos+3]);

    return data;
}

#define D_DOC "domain name like format\n"\
"A DNS like name. This format is used for many record types."
char * domain_name(const uint8_t * packet, uint32_t pos, uint32_t id_pos,
                   uint16_t rdlength, uint32_t plen) {
    char * name = readRRName(packet, &pos, id_pos, plen);
    if (name == NULL) 
        name = "Bad DNS name";

    return name;
}

#define SOA_DOC "Start of Authority format\n"\
"Presented as a series of labeled SOA fields."
char * soa(const uint8_t * packet, uint32_t pos, uint32_t id_pos,
                 uint16_t rdlength, uint32_t plen) {
    char * mname;
    char * rname;
    char * buffer;
    uint32_t serial, refresh, retry, expire, minimum;
    const char * format = "mname: %s, rname: %s, serial: %d, "
                          "refresh: %d, retry: %d, expire: %d, min: %d";

    mname = readRRName(packet, &pos, id_pos, plen);
    if (mname == NULL) return mk_error("Bad SOA: ", packet, pos, rdlength);
    rname = readRRName(packet, &pos, id_pos, plen);
    if (rname == NULL) return mk_error("Bad SOA: ", packet, pos, rdlength);

    int i;
    serial = refresh = retry = expire = minimum = 0;
    for (i = 0; i < 4; i++) {
        serial  <<= 8; serial  |= packet[pos+(i+(4*0))];
        refresh <<= 8; refresh |= packet[pos+(i+(4*1))];
        retry   <<= 8; retry   |= packet[pos+(i+(4*2))];
        expire  <<= 8; expire  |= packet[pos+(i+(4*3))];
        minimum <<= 8; minimum |= packet[pos+(i+(4*4))];
    }
    
    // let snprintf() measure the formatted string
    int len = snprintf(0, 0, format, mname, rname, serial, refresh, retry,
                       expire, minimum);
    buffer = (char*) malloc(len + 1);
    sprintf(buffer, format, mname, rname, serial, refresh, retry, expire,
            minimum);
    free(mname);
    free(rname);
    return buffer;
}

#define MX_DOC "Mail Exchange record format\n"\
"A standard dns name preceded by a preference number."
char * mx(const uint8_t * packet, uint32_t pos, uint32_t id_pos,
                uint16_t rdlength, uint32_t plen) {

    uint16_t pref = (packet[pos] << 8) + packet[pos+1];
    char * name;
    char * buffer;
    uint32_t spos = pos;

    pos = pos + 2;
    name = readRRName(packet, &pos, id_pos, plen);
    if (name == NULL) 
        return mk_error("Bad MX: ", packet, spos, rdlength);

    buffer = (char*)  malloc(sizeof(char)*(5 + 1 + strlen(name) + 1));
    sprintf(buffer, "%d,%s", pref, name);
    free(name);
    return buffer;
}

#define OPTS_DOC "EDNS option record format\n"\
"These records contain a size field for warning about extra large DNS \n"\
"packets, an extended rcode, and an optional set of dynamic fields.\n"\
"The size and extended rcode are printed, but the dynamic fields are \n"\
"simply escaped. Note that the associated format function is non-standard,\n"\
"as EDNS records modify the basic resourse record protocol (there is no \n"\
"class field, for instance. RFC 2671"
char * opts(const uint8_t * packet, uint32_t pos, uint32_t id_pos,
                  uint16_t rdlength, uint32_t plen) {
    uint16_t payload_size = (packet[pos] << 8) + packet[pos+1];
    char *buffer;
    const char * base_format = "size:%d,rcode:0x%02x%02x%02x%02x,%s";
    //char *rdata = escape_data(packet, pos+6, pos + 6 + rdlength);

    // buffer = (char*) malloc(sizeof(char) * (strlen(base_format) - 20 + 5 + 8 +
    //                                 strlen(rdata) + 1)); 
    // sprintf(buffer, base_format, payload_size, packet[pos+2], packet[pos+3],
    //                              packet[pos+4], packet[pos+5], rdata); 
    // free(rdata);
    return buffer;
}

#define SRV_DOC "Service record format. RFC 2782\n"\
"Service records are used to identify various network services and ports.\n"\
"The format is: 'priority,weight,port target'\n"\
"The target is a somewhat standard DNS name."
char * srv(const uint8_t * packet, uint32_t pos, uint32_t id_pos,
                 uint16_t rdlength, uint32_t plen) {
    uint16_t priority = (packet[pos] << 8) + packet[pos+1];
    uint16_t weight = (packet[pos+2] << 8) + packet[pos+3];
    uint16_t port = (packet[pos+4] << 8) + packet[pos+5];
    char *target, *buffer;
    pos = pos + 6;
    // Don't read beyond the end of the rr.
    target = readRRName(packet, &pos, id_pos, pos+rdlength-6);
    if (target == NULL) 
        return mk_error("Bad SRV", packet, pos, rdlength);
    
    buffer = (char*) malloc(sizeof(char) * ((3*5+3) + strlen(target)));
    sprintf(buffer, "%d,%d,%d %s", priority, weight, port, target);
    free(target);
    return buffer;
}

#define AAAA_DOC "IPv6 record format.  RFC 3596\n"\
"A standard IPv6 address. No attempt is made to abbreviate the address."
char * AAAA(const uint8_t * packet, uint32_t pos, uint32_t id_pos,
                  uint16_t rdlength, uint32_t plen) {
    char *buffer;
    uint16_t ipv6[8];
    int i;

    if (rdlength != 16) { 
        return mk_error("Bad AAAA record", packet, pos, rdlength);
    }

    for (i=0; i < 8; i++) 
        ipv6[i] = (packet[pos+i*2] << 8) + packet[pos+i*2+1];
    buffer = (char*)  malloc(sizeof(char) * (4*8 + 7 + 1));
    sprintf(buffer, "%x:%x:%x:%x:%x:%x:%x:%x", ipv6[0], ipv6[1], ipv6[2],
                                               ipv6[3], ipv6[4], ipv6[5],
                                               ipv6[6], ipv6[7]); 
    return buffer;
}

#define KEY_DOC "dnssec Key format. RFC 4034\n"\
"format: flags, proto, algorithm, key\n"\
"All fields except the key are printed as decimal numbers.\n"\
"The key is given in base64. "
char * dnskey(const uint8_t * packet, uint32_t pos, uint32_t id_pos,
                    uint16_t rdlength, uint32_t plen) {
    uint16_t flags = (packet[pos] << 8) + packet[pos+1];
    uint8_t proto = packet[pos+2];
    uint8_t algorithm = packet[pos+3];
    char *buffer, *key;

    key = b64encode(packet, pos+4, rdlength-4);
    buffer = (char*) malloc(sizeof(char) * (1 + strlen(key) + 18));
    sprintf(buffer, "%d,%d,%d,%s", flags, proto, algorithm, key);
    free(key);
    return buffer;
}

#define RRSIG_DOC "DNS SEC Signature. RFC 4304\n"\
"format: tc,alg,labels,ottl,expiration,inception,tag signer signature\n"\
"All fields except the signer and signature are given as decimal numbers.\n"\
"The signer is a standard DNS name.\n"\
"The signature is base64 encoded."
char * rrsig(const uint8_t * packet, uint32_t pos, uint32_t id_pos,
                   uint16_t rdlength, uint32_t plen) {
    uint32_t o_pos = pos;
    uint16_t tc = (packet[pos] << 8) + packet[pos+1];
    uint8_t alg = packet[pos+2];
    uint8_t labels = packet[pos+3];
    u_int ottl, sig_exp, sig_inc;
    uint16_t key_tag = (packet[pos+16] << 8) + packet[pos+17];
    char *signer, *signature, *buffer;
    pos = pos + 4;
    ottl = (packet[pos] << 24) + (packet[pos+1] << 16) + 
           (packet[pos+2] << 8) + packet[pos+3];
    pos = pos + 4;
    sig_exp = (packet[pos] << 24) + (packet[pos+1] << 16) + 
              (packet[pos+2] << 8) + packet[pos+3];
    pos = pos + 4; 
    sig_inc = (packet[pos] << 24) + (packet[pos+1] << 16) + 
              (packet[pos+2] << 8) + packet[pos+3];
    pos = pos + 6;
    signer = readRRName(packet, &pos, id_pos, o_pos+rdlength);
    if (signer == NULL) 
        return mk_error("Bad Signer name", packet, pos, rdlength);
        
    signature = b64encode(packet, pos, o_pos+rdlength-pos);
    buffer = (char*) malloc(sizeof(char) * (2*5 + // 2 16 bit ints
                                    3*10 + // 3 32 bit ints
                                    2*3 + // 2 8 bit ints
                                    8 + // 8 separator chars
                                    strlen(signer) +
                                    strlen(signature) + 1));
    sprintf(buffer, "%d,%d,%d,%d,%d,%d,%d,%s,%s", tc, alg, labels, ottl, 
                    sig_exp, sig_inc, key_tag, signer, signature);
    free(signer);
    free(signature);
    return buffer;
}

#define NSEC_DOC "NSEC format.  RFC 4034\n"\
"Format: domain bitmap\n"\
"domain is a DNS name, bitmap is hex escaped."
char * nsec(const uint8_t * packet, uint32_t pos, uint32_t id_pos,
                  uint16_t rdlength, uint32_t plen) {

    char *buffer, *domain, *bitmap;

    domain = readRRName(packet, &pos, id_pos, pos+rdlength);
    if (domain == NULL) 
        return mk_error("Bad NSEC domain", packet, pos, rdlength);
    
    // bitmap = escape_data(packet, pos, pos+rdlength);
    // buffer = (char*) malloc(sizeof(char) * (strlen(domain)+strlen(bitmap)+2));
    // sprintf(buffer, "%s,%s", domain, bitmap);
    // free(domain);
    // free(bitmap);
    return buffer;

}

#define DS_DOC "DS DNS SEC record.  RFC 4034\n"\
"format: key_tag,algorithm,digest_type,digest\n"\
"The keytag, algorithm, and digest type are given as base 10.\n"\
"The digest is base64 encoded."
char * ds(const uint8_t * packet, uint32_t pos, uint32_t id_pos,                          uint16_t rdlength, uint32_t plen) {
    uint16_t key_tag = (packet[pos] << 8) + packet[pos+1];
    uint8_t alg = packet[pos+2];
    uint8_t dig_type = packet[pos+3];
    char * digest = b64encode(packet,pos+4,rdlength-4);
    char * buffer;

    buffer = (char*) malloc(sizeof(char) * (strlen(digest) + 15));
    sprintf(buffer,"%d,%d,%d,%s", key_tag, alg, dig_type, digest);
    free(digest);
    return buffer;
}

// Add parser functions here, they should be prototyped in rtypes.h and
// then defined below.
// Some of the rtypes below use the escape parser.  This isn't
// because we don't know how to parse them, it's simply because that's
// the right parser for them anyway.
rr_parser_container rr_parsers[] =
{
    {1, 1, A, "A", A_DOC, 0},
    {0, 2, domain_name, "NS", D_DOC, 0},
    {0, 5, domain_name, "CNAME", D_DOC, 0},
    {0, 6, soa, "SOA", SOA_DOC, 0},
    {0, 12, domain_name, "PTR", D_DOC, 0},
    {1, 33, srv, "SRV", SRV_DOC, 0}, 
    {1, 28, AAAA, "AAAA", AAAA_DOC, 0},
    {0, 15, mx, "MX", MX_DOC, 0},
    {0, 46, rrsig, "RRSIG", RRSIG_DOC, 0},
    {0, 16, domain_name, "TEXT", NULL_DOC, 0}, 
    {0, 47, nsec, "NSEC", NSEC_DOC, 0},
    {0, 43, ds, "DS", DS_DOC, 0},
    {0, 10, domain_name, "NULL", NULL_DOC, 0}, 
    {0, 48, dnskey, "DNSKEY", KEY_DOC, 0},
    {0, 255, domain_name, "ANY", NULL_DOC, 0}
};

// Parse the udp headers.
uint32_t udpParse(uint32_t pos, struct pcap_pkthdr *header, 
                   uint8_t *packet, transport_info * udp) {
    if (header->len - pos < 8) {
        fprintf(stderr, "Truncated Packet(udp)\n");
        return 0;
    }

    udp->srcport = (packet[pos] << 8) + packet[pos+1];
    udp->dstport = (packet[pos+2] << 8) + packet[pos+3];
    udp->length = (packet[pos+4] << 8) + packet[pos+5];
    udp->transport = UDP;
    fprintf(stderr, "udp\n");
    fprintf(stderr, "srcport: %d, dstport: %d, len: %d\n", udp->srcport, udp->dstport, udp->length);
    //SHOW_RAW(print_packet(header->len, packet, pos, pos, 4);)
    return pos + 8;
}



// We occasionally sort the parsers according to how they're used in order
// to speed up lookups.
void sort_parsers() {
    int m,n;
    int change = 1;
    int pcount = count_parsers();
    rr_parser_container tmp;
    for (m = 0; m < pcount - 1 && change == 1; m++) {
        change = 0;
        for (n = 0; n < pcount - 1; n++) {
            if (rr_parsers[n].count < rr_parsers[n+1].count) {
                tmp = rr_parsers[n];
                rr_parsers[n] = rr_parsers[n+1];
                rr_parsers[n+1] = tmp;
                change = 1;
            }
        }
    }
    // Reset the counts
    for (m = 0; m < pcount - 1; m++) {
        rr_parsers[m].count = 0;
    }
}



void print_parsers() {
    int i;
    printf("What follows is a list of handled DNS classes and resource \n"
           "record types. \n"
           " - The class # may be listed as 'any', though anything \n"
           "   other than the internet class is rarely seen. \n"
           " - Parsers for records other than those in RFC 1035 should \n"
           "   have their RFC listed. \n"
           " - Unhandled resource records are simply string escaped.\n"
           " - Some resource records share parsers and documentation.\n\n"
           "class, rtype, name: documentation\n");
    for (i=0; i < count_parsers(); i++) {
        rr_parser_container cont = rr_parsers[i];
        if (cont.cls == 0) printf("any,");
        else printf("%d,", cont.cls);

        printf(" %d, %s: %s\n\n", cont.rtype, cont.name, cont.doc);
    }
}

inline int count_parsers()
{
    return sizeof(rr_parsers)/sizeof(rr_parser_container);
}





