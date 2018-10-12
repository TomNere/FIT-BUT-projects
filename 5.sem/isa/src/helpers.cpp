#include "dns-export.h"

using namespace std;

// Move IPv4 addr at pointer P into ip object D, and set it's type.
#define IPv4_MOVE(D, P) D.addr.v4.s_addr = *(in_addr_t *)(P); \
                        D.vers = IPv4;
// Move IPv6 addr at pointer P into ip object D, and set it's type.
#define IPv6_MOVE(D, P) memcpy(D.addr.v6.s6_addr, P, 16); D.vers = IPv6;

unsigned int PACKETS_SEEN = 0;
#define REORDER_LIMIT 100000

#define NULL_DOC "This data is simply hex escaped. \n"\
"Non printable characters are given as a hex value (\\x30), for example."
char * escape(const uint8_t * packet, uint32_t pos, uint32_t i,
              uint16_t rdlength, uint32_t plen) {
    return escape_data(packet, pos, pos + rdlength);
}


// Compare two IP addresses.
#define IP_CMP(ipA, ipB) ((ipA.vers == ipB.vers) &&\
                          (ipA.vers == IPv4 ? \
                            ipA.addr.v4.s_addr == ipB.addr.v4.s_addr : \
                ((ipA.addr.v6.s6_addr32[0] == ipB.addr.v6.s6_addr32[0]) && \
                 (ipA.addr.v6.s6_addr32[1] == ipB.addr.v6.s6_addr32[1]) && \
                 (ipA.addr.v6.s6_addr32[2] == ipB.addr.v6.s6_addr32[2]) && \
                 (ipA.addr.v6.s6_addr32[3] == ipB.addr.v6.s6_addr32[3])) \
                 ))

char * mk_error(const char * msg, const uint8_t * packet, uint32_t pos,
                uint16_t rdlength) {
    char * tmp = escape_data(packet, pos, pos+rdlength);
    size_t len = strlen(tmp) + strlen(msg) + 1;
    char * buffer = (char*) malloc(sizeof(char)*len);
    sprintf(buffer, "%s%s", msg, tmp);
    free(tmp);
    return buffer;
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

#define A_DOC "A (IPv4 address) format\n"\
"A records are simply an IPv4 address, and are formatted as such."
char * A(const uint8_t * packet, uint32_t pos, uint32_t i,
         uint16_t rdlength, uint32_t plen) {
    char * data = (char *)malloc(sizeof(char)*16);

    if (rdlength != 4) {
        free(data);
        return mk_error("Bad A record: ", packet, pos, rdlength);
    }
    
    sprintf(data, "%d.%d.%d.%d", packet[pos], packet[pos+1],
                                 packet[pos+2], packet[pos+3]);

    return data;
}

#define D_DOC "domain name like format\n"\
"A DNS like name. This format is used for many record types."
char * domain_name(const uint8_t * packet, uint32_t pos, uint32_t id_pos,
                   uint16_t rdlength, uint32_t plen) {
    char * name = read_rr_name(packet, &pos, id_pos, plen);
    if (name == NULL) 
        name = mk_error("Bad DNS name: ", packet, pos, rdlength);

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

    mname = read_rr_name(packet, &pos, id_pos, plen);
    if (mname == NULL) return mk_error("Bad SOA: ", packet, pos, rdlength);
    rname = read_rr_name(packet, &pos, id_pos, plen);
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
    name = read_rr_name(packet, &pos, id_pos, plen);
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
    char *rdata = escape_data(packet, pos+6, pos + 6 + rdlength);

    buffer = (char*) malloc(sizeof(char) * (strlen(base_format) - 20 + 5 + 8 +
                                    strlen(rdata) + 1)); 
    sprintf(buffer, base_format, payload_size, packet[pos+2], packet[pos+3],
                                 packet[pos+4], packet[pos+5], rdata); 
    free(rdata);
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
    target = read_rr_name(packet, &pos, id_pos, pos+rdlength-6);
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
    signer = read_rr_name(packet, &pos, id_pos, o_pos+rdlength);
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

    domain = read_rr_name(packet, &pos, id_pos, pos+rdlength);
    if (domain == NULL) 
        return mk_error("Bad NSEC domain", packet, pos, rdlength);
    
    bitmap = escape_data(packet, pos, pos+rdlength);
    buffer = (char*) malloc(sizeof(char) * (strlen(domain)+strlen(bitmap)+2));
    sprintf(buffer, "%s,%s", domain, bitmap);
    free(domain);
    free(bitmap);
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
rr_parser_container rr_parsers[] = {{1, 1, A, "A", A_DOC, 0},
                                    {0, 2, domain_name, "NS", D_DOC, 0},
                                    {0, 5, domain_name, "CNAME", D_DOC, 0},
                                    {0, 6, soa, "SOA", SOA_DOC, 0},
                                    {0, 12, domain_name, "PTR", D_DOC, 0},
                                    {1, 33, srv, "SRV", SRV_DOC, 0}, 
                                    {1, 28, AAAA, "AAAA", AAAA_DOC, 0},
                                    {0, 15, mx, "MX", MX_DOC, 0},
                                    {0, 46, rrsig, "RRSIG", RRSIG_DOC, 0},
                                    {0, 16, escape, "TEXT", NULL_DOC, 0}, 
                                    {0, 47, nsec, "NSEC", NSEC_DOC, 0},
                                    {0, 43, ds, "DS", DS_DOC, 0},
                                    {0, 10, escape, "NULL", NULL_DOC, 0}, 
                                    {0, 48, dnskey, "DNSKEY", KEY_DOC, 0},
                                    {0, 255, escape, "ANY", NULL_DOC, 0}
                                   };


inline int count_parsers() {
    return sizeof(rr_parsers)/sizeof(rr_parser_container);
}

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


char * escape_data(const uint8_t * packet, uint32_t start, uint32_t end) { 
    int i,o;
    uint8_t c;
    unsigned int length=1;

    char * outstr;

    for (i=start; i<end; i++) {
        c = packet[i];
        if (c < 0x20 || c == 0x5c || c >= 0x7f) length += 4;
        else length += 1;
    }

    outstr = (char *)malloc(sizeof(char)*length);
    // If the malloc failed then fail.
    if (outstr == 0) return (char *)0;

    o=0;
    for (i=start; i<end; i++) {
        c = packet[i];
        if (c < 0x20 || c == 0x5c || c >= 0x7f) {
            outstr[o] = '\\';
            outstr[o+1] = 'x';
            outstr[o+2] = c/16 + 0x30;
            outstr[o+3] = c%16 + 0x30;
            if (outstr[o+2] > 0x39) outstr[o+2] += 0x27;
            if (outstr[o+3] > 0x39) outstr[o+3] += 0x27;
            o += 4;
        } else {
            outstr[o] = c;
            o++;
        }
    }
    outstr[o] = 0;
    return outstr;
}

char * read_rr_name(const uint8_t * packet, uint32_t * packet_p, 
                    uint32_t id_pos, uint32_t len) {
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
    while (pos < len && !(next == pos && packet[pos] == 0)
           && steps < len*2) {
        uint8_t c = packet[pos];
        steps++;
        if (next == pos) {
            // Handle message compression.  
            // If the length byte starts with the bits 11, then the rest of
            // this byte and the next form the offset from the dns proto start
            // to the start of the remainder of the name.
            if ((c & 0xc0) == 0xc0) {
                if (pos + 1 >= len) return 0;
                if (end_pos == 0) end_pos = pos + 1;
                pos = id_pos + ((c & 0x3f) << 8) + packet[pos+1];
                next = pos;
            } else {
                name_len++;
                pos++;
                next = next + c + 1; 
            }
        } else {
            if (c >= '!' && c <= 'z' && c != '\\') name_len++;
            else name_len += 4;
            pos++;
        }
    }
    if (end_pos == 0) end_pos = pos;

    // Due to the nature of DNS name compression, it's possible to get a
    // name that is infinitely long. Return an error in that case.
    // We use the len of the packet as the limit, because it shouldn't 
    // be possible for the name to be that long.
    if (steps >= 2*len || pos >= len) return NULL;

    name_len++;

    name = (char *)malloc(sizeof(char) * name_len);
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
    while (next != pos || packet[pos] != 0) {
        if (pos == next) {
            if ((packet[pos] & 0xc0) == 0xc0) {
                pos = id_pos + ((packet[pos] & 0x3f) << 8) + packet[pos+1];
                next = pos;
            } else {
                // Add a period except for the first time.
                if (i != 0) name[i++] = '.';
                next = pos + packet[pos] + 1;
                pos++;
            }
        } else {
            uint8_t c = packet[pos];
            if (c >= '!' && c <= '~' && c != '\\') {
                name[i] = packet[pos];
                i++; pos++;
            } else {
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

    return name;
}

// Parse the questions section of the dns protocol.
// pos - offset to the start of the questions section.
// id_pos - offset set to the id field. Needed to decompress dns data.
// packet, header - the packet location and header data.
// count - Number of question records to expect.
// root - Pointer to where to store the question records.
uint32_t parseQuestions(uint32_t pos, uint32_t id_pos, 
                         struct pcap_pkthdr *header,
                         uint8_t *packet, uint16_t count, 
                         dns_question ** root) {
    uint32_t start_pos = pos; 
    dns_question * last = NULL;
    dns_question * current;
    uint16_t i;
    *root = NULL;

    for (i=0; i < count; i++) {
        current = (dns_question*) malloc(sizeof(dns_question));
        current->next = NULL; current->name = NULL;

        current->name = read_rr_name(packet, &pos, id_pos, header->len);
        if (current->name == NULL || (pos + 2) >= header->len) {
            // Handle a bad DNS name.
            fprintf(stderr, "DNS question error\n");
            char * buffer = escape_data(packet, start_pos, header->len);
            const char * msg = "Bad DNS question: ";
            current->name = (char*) malloc(sizeof(char) * (strlen(buffer) +
                                                   strlen(msg) + 1));
            sprintf(current->name, "%s%s", msg, buffer);
            free(buffer);
            current->type = 0;
            current->cls = 0;
            if (last == NULL) *root = current;
            else last->next = current;
            return 0;
        }
        current->type = (packet[pos] << 8) + packet[pos+1];
        current->cls = (packet[pos+2] << 8) + packet[pos+3];

        // Add this question object to the list.
        if (last == NULL) *root = current;
        else last->next = current;
        last = current;
        pos = pos + 4;

        // VERBOSE(printf("question->name: %s\n", current->name);)
        // VERBOSE(printf("type %d, cls %d\n", current->type, current->cls);)
   }

    return pos;
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

// Parse an individual resource record, placing the acquired data in 'rr'.
// 'packet', 'pos', and 'id_pos' serve the same uses as in parse_rr_set.
// Return 0 on error, the new 'pos' in the packet otherwise.
uint32_t parseRR(uint32_t pos, uint32_t id_pos, struct pcap_pkthdr *header, 
                  uint8_t *packet, dns_rr* rr, bool flag) {
    int i;
    uint32_t rr_start = pos;
    rr_parser_container * parser;
    rr_parser_container opts_cont = {0,0, opts};

    rr->name = NULL;
    rr->data = NULL;

    rr->name = read_rr_name(packet, &pos, id_pos, header->len);
    // Handle a bad rr name.
    // We still want to print the rest of the escaped rr data.
    if (rr->name == NULL) {
        const char * msg = "Bad rr name: ";
        rr->name = (char*) malloc(sizeof(char) * (strlen(msg) + 1));
        sprintf(rr->name, "%s", "Bad rr name");
        rr->type = 0;
        rr->rr_name = NULL;
        rr->cls = 0;
        rr->ttl = 0;
        rr->data = escape_data(packet, pos, header->len);
        return 0;
    }

    if ((header->len - pos) < 10 ) return 0;

    rr->type = (packet[pos] << 8) + packet[pos+1];
    rr->rdlength = (packet[pos+8] << 8) + packet[pos + 9];
    // Handle edns opt RR's differently.
    if (rr->type == 41) {
        rr->cls = 0;
        rr->ttl = 0;
        rr->rr_name = "OPTS";
        parser = &opts_cont;
        // We'll leave the parsing of the special EDNS opt fields to
        // our opt rdata parser.  
        pos = pos + 2;
    } else {
        // The normal case.
        rr->cls = (packet[pos+2] << 8) + packet[pos+3];
        rr->ttl = 0;
        for (i=0; i<4; i++)
            rr->ttl = (rr->ttl << 8) + packet[pos+4+i];
        // Retrieve the correct parser function.
        parser = findParser(rr->cls, rr->type);
        rr->rr_name = parser->name;
        pos = pos + 10;
    }

    fprintf(stderr, "Applying RR parser: %s\n", parser->name);

    // if (conf->MISSING_TYPE_WARNINGS && &default_rr_parser == parser) 
    //     fprintf(stderr, "Missing parser for class %d, type %d\n", 
    //                     rr->cls, rr->type);

    // Make sure the data for the record is actually there.
    // If not, escape and print the raw data.
    if (header->len < (rr_start + 10 + rr->rdlength)) {
        char * buffer;
        const char * msg = "Truncated rr: ";
        rr->data = escape_data(packet, rr_start, header->len);
        buffer = (char*) malloc(sizeof(char) * (strlen(rr->data) + strlen(msg) + 1));
        sprintf(buffer, "%s%s", msg, rr->data);
        free(rr->data);
        rr->data = buffer;
        return 0;
    }
    // Parse the resource record data.
    rr->data = parser->parser(packet, pos, id_pos, rr->rdlength, 
                              header->len);
    
    fprintf(stderr, "rr->name: %s\n", rr->name);
    fprintf(stderr, "type %d, cls %d, ttl %d, len %d\n", rr->type, rr->cls, rr->ttl,
           rr->rdlength);
    fprintf(stderr, "rr->data %s\n", rr->data);

    return pos + rr->rdlength;
}

// Parse a set of resource records in the dns protocol in 'packet', starting
// at 'pos'. The 'id_pos' offset is necessary for putting together 
// compressed names. 'count' is the expected number of records of this type.
// 'root' is where to assign the parsed list of objects.
// Return 0 on error, the new 'pos' in the packet otherwise.
uint32_t parse_rr_set(uint32_t pos, uint32_t id_pos, 
                         struct pcap_pkthdr *header,
                         uint8_t *packet, uint16_t count, 
                         dns_rr ** root) {
    dns_rr * last = NULL;
    dns_rr * current;
    uint16_t i;
    *root = NULL; 
    for (i=0; i < count; i++) {
        // Create and clear the data in a new dns_rr object.
        current = (dns_rr*) malloc(sizeof(dns_rr));
        current->next = NULL; current->name = NULL; current->data = NULL;

        pos = parseRR(pos, id_pos, header, packet, current, true);
        // If a non-recoverable error occurs when parsing an rr, 
        // we can only return what we've got and give up.
        if (pos == 0) {
            if (last == NULL) *root = current;
            else last->next = current;
            return 0;
        }
        if (last == NULL) *root = current;
        else last->next = current;
        last = current;
    }
    return pos;
}


// Parse the dns protocol in 'packet'. 
// See RFC1035
// See dns_parse.h for more info.
uint32_t dns_parse(uint32_t pos, struct pcap_pkthdr *header, 
                   uint8_t *packet, dns_info * dns, uint8_t force) {

    uint32_t id_pos = pos;

    if (header->len - pos < 12) {
        char * msg = escape_data(packet, id_pos, header->len);
        fprintf(stderr, "Truncated Packet(dns): %s\n", msg); 
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
    pos = parseQuestions(pos+12, id_pos, header, packet, 
                          dns->qdcount, &(dns->queries));
    if (pos != 0) 
        pos = parse_rr_set(pos, id_pos, header, packet, 
                           dns->ancount, &(dns->answers));
    else dns->answers = NULL;
    if (pos != 0 && force) {
        pos = parse_rr_set(pos, id_pos, header, packet, 
                           dns->nscount, &(dns->name_servers));
    } else dns->name_servers = NULL;
    if (pos != 0 && force) {
        pos = parse_rr_set(pos, id_pos, header, packet, 
                           dns->arcount, &(dns->additional));
    } else dns->additional = NULL;
    return pos;
}

// Print the time stamp.
void print_ts(struct timeval * ts) {
    cout << (int)ts->tv_sec << ":" << (int)ts->tv_usec;
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

// Output the DNS data.
void printSummary(ipInfo * ip, transport_info * trns, dns_info * dns,
                   struct pcap_pkthdr * header) {
    char proto;

    uint32_t dnslength;
    dns_question *qnext;

    print_ts(&(header->ts));

    // Print the transport protocol indicator.
    if (ip->proto == 17) {
        proto = 'u';
    } else if (ip->proto == 6) {
        proto = 't';
    } else {
        return;
    }
    dnslength = trns->length;

    // Print the IP addresses and the basic query information.
    printf(",%s,", iptostr(&ip->src));
    printf("%s,%d,%c,%c,%s", iptostr(&ip->dst),
           dnslength, proto, dns->qr ? 'r':'q', dns->AA?"AA":"NA");

    // if (conf->COUNTS) {
    //     printf(",%u?,%u!,%u$,%u+", dns->qdcount, dns->ancount, 
    //                                dns->nscount, dns->arcount);
    // }

    // Go through the list of queries, and print each one.
    qnext = dns->queries;
    while (qnext != NULL) {
        printf("%c? ", '\t');
            rr_parser_container * parser; 
            parser = findParser(qnext->cls, qnext->type);
            if (parser->name == NULL) 
                printf("%s UNKNOWN(%s,%d)", qnext->name, parser->name, qnext->type);
            else 
                printf("%s %s", qnext->name, parser->name);
        
        qnext = qnext->next; 
    }

    // Print it resource record type in turn (for those enabled).
    printRRSection(dns->answers, "!";
    // if (conf->NS_ENABLED) 
    //     printRRSection(dns->name_servers, "$");
    // if (conf->AD_ENABLED) 
    //     printRRSection(dns->additional, "+");
    //printf("%c%s\n", conf->SEP, conf->RECORD_SEP);
    
    dns_question_free(dns->queries);
    dns_rr_free(dns->answers);
    dns_rr_free(dns->name_servers);
    dns_rr_free(dns->additional);
    fflush(stdout); 
    fflush(stderr);
}