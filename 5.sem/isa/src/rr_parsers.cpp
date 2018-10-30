#include "rr_parsers.h"
#include "helpers.cpp"

using namespace std;

// This is for handling rr's with errors or an unhandled rtype.
rrParserContainer default_rr_parser;

// Add them to the list of data parsers in rtypes.c.
extern rrParserContainer rr_parsers[];

// Find the parser that corresponds to the given cls and rtype.
rrParserContainer* findParser(uint16_t cls, uint16_t rtype) {

    unsigned int i=0;
    rrParserContainer * found = NULL;

    while (i < PARSERS_COUNT && found == NULL) {
        rrParserContainer pc = rr_parsers[i];
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
char * domain_name(const uint8_t * packet, uint32_t pos, uint32_t idPos,
                   uint16_t rdlength, uint32_t plen) {
    char * name = readRRName(packet, &pos, idPos, plen);
    if (name == NULL) 
        name = "Bad DNS name";

    return name;
}

#define SOA_DOC "Start of Authority format\n"\
"Presented as a series of labeled SOA fields."
char * soa(const uint8_t * packet, uint32_t pos, uint32_t idPos,
                 uint16_t rdlength, uint32_t plen) {
    char * mname;
    char * rname;
    char * buffer;
    uint32_t serial, refresh, retry, expire, minimum;
    const char * format = "mname: %s, rname: %s, serial: %d, "
                          "refresh: %d, retry: %d, expire: %d, min: %d";

    mname = readRRName(packet, &pos, idPos, plen);
    if (mname == NULL) return mk_error("Bad SOA: ", packet, pos, rdlength);
    rname = readRRName(packet, &pos, idPos, plen);
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
char * mx(const uint8_t * packet, uint32_t pos, uint32_t idPos,
                uint16_t rdlength, uint32_t plen) {

    uint16_t pref = (packet[pos] << 8) + packet[pos+1];
    char * name;
    char * buffer;
    uint32_t spos = pos;

    pos = pos + 2;
    name = readRRName(packet, &pos, idPos, plen);
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
char * opts(const uint8_t * packet, uint32_t pos, uint32_t idPos,
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

#define AAAA_DOC "IPv6 record format.  RFC 3596\n"\
"A standard IPv6 address. No attempt is made to abbreviate the address."
char * AAAA(const uint8_t * packet, uint32_t pos, uint32_t idPos,
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
char * dnskey(const uint8_t * packet, uint32_t pos, uint32_t idPos,
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
char * rrsig(const uint8_t * packet, uint32_t pos, uint32_t idPos,
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
    signer = readRRName(packet, &pos, idPos, o_pos+rdlength);
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
char * nsec(const uint8_t * packet, uint32_t pos, uint32_t idPos,
                  uint16_t rdlength, uint32_t plen) {

    char *buffer, *domain, *bitmap;

    domain = readRRName(packet, &pos, idPos, pos+rdlength);
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
char * ds(const uint8_t * packet, uint32_t pos, uint32_t idPos, uint16_t rdlength, uint32_t plen) {
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
rrParserContainer rr_parsers[] =
{
    {1, 1, A, "A", A_DOC, 0},
    {0, 2, domain_name, "NS", D_DOC, 0},
    {0, 5, domain_name, "CNAME", D_DOC, 0},
    {0, 6, soa, "SOA", SOA_DOC, 0},
    {1, 28, AAAA, "AAAA", AAAA_DOC, 0},
    {0, 15, mx, "MX", MX_DOC, 0},
    {0, 46, rrsig, "RRSIG", RRSIG_DOC, 0},
    {0, 16, domain_name, "TEXT", NULL_DOC, 0}, 
    {0, 47, nsec, "NSEC", NSEC_DOC, 0},
    {0, 43, ds, "DS", DS_DOC, 0},
    {0, 48, dnskey, "DNSKEY", KEY_DOC, 0}
};
