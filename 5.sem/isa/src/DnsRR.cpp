
#include <stdint.h> // Getopt
#include <pcap.h>   // No comment needed...
#include <string>
#include <sstream>      // std::stringstream
#include "structures.h"

using namespace std;

// Holds the information for a dns resource record.
class DnsRR
{
    string readRRName()
    {
        uint32_t i, next, pos;
        pos = this->position;
        uint32_t endPos = 0;
        uint32_t nameLen = 0;
        uint32_t steps = 0;
        string name;

        // Scan through the name, one character at a time. We need to look at 
        // each character to look for values we can't print in order to allocate
        // extra space for escaping them.  'next' is the next position to look
        // for a compression jump or name end.
        // It's possible that there are endless loops in the name. Our protection
        // against this is to make sure we don't read more bytes in this process
        // than twice the length of the data.  Names that take that many steps to 
        // read in should be impossible.
        next = pos;
        while (pos < this->header->len && !(next == pos && this->packet[pos] == 0) && steps < this->header->len * 2)
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
                    if (pos + 1 >= this->header->len)
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
        if (steps >= 2 * this->header->len || pos >= this->header->len)
        {
            return NULL;
        }

        nameLen++;

        name = (char*) malloc(sizeof(char)* nameLen ) ;
        pos = this->position;

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

        this->position = endPos + 1;
        LOGGING("Returning RR name: " << name);
        return name;
    }

    void applyParser()
    {
        switch (this->type)
        {
            case 1:
                this->rrName = "A";
            case 28:
                this->rrName = "AAAA";
            case 5:
                this->rrName = "CNAME";
            case 15:
                this->rrName = "MX";
            case 2:
                this->rrName = "NS";
            case 6:
                this->rrName = "SOA";
            case 16:
                this->rrName = "TXT";
            case 99:
                this->rrName = "SPF";
            // DNSSEC
            case 48:
                this->rrName = "DNSKEY";
            case 46:
                this->rrName = "RRSIG";
             case 47:
                this->rrName = "NSEC";
            case 43:
                this->rrName = "DS";
            default:
                this->rrName = "Unknown";
        }
    }

    /****************************************** RR parsers *****************************************************/

    void parserA()
    {
        if (rdlength != 4)
        {
            return;
        }

        stringstream ss;

        ss << packet[this->position] << " " << packet[this->position + 1] << " " << packet[this->position + 2] << " " << packet[this->position + 3];
        ss >> this->data;
    }


    void parserDOMAIN_NAME()
    {
        string name = this->readRRName();
        if (name.empty())
        {
            LOGGING("Invalid domain name.")
        }
        this->data = name;
    }

    void parserSOA()
    {
        string mname;
        string rname;
        string buffer;
        uint32_t serial, refresh, retry, expire, minimum;
        const char * format = "mname: %s, rname: %s, serial: %d, "
                          "refresh: %d, retry: %d, expire: %d, min: %d";

        mname = readRRName();
        if (mname.empty())
        {
            LOGGING("Invalid SOA mname");
            return;
        }

        rname = readRRName();
        if (rname.empty())
        {
            LOGGING("Invalid SOA rname");
            return;
        }

        int i;
        serial = refresh = retry = expire = minimum = 0;
        for (i = 0; i < 4; i++)
        {
            serial  <<= 8; serial  |= packet[this->position + (i + (4 * 0))];
            refresh <<= 8; refresh |= packet[this->position + (i + (4 * 1))];
            retry   <<= 8; retry   |= packet[this->position + (i + (4 * 2))];
            expire  <<= 8; expire  |= packet[this->position + (i + (4 * 3))];
            minimum <<= 8; minimum |= packet[this->position + (i + (4 * 4))];
        }
    
        stringstream ss;
        ss << mname << " " << rname << " " << serial << " " << refresh << " " << retry << " " << expire << " " << minimum; 

        ss >> this->data; 
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



    public:
        string name;
        uint16_t type;
        uint16_t cls;
        string rrName;
        uint16_t ttl;
        uint16_t rdlength;
        uint16_t dataLen;
        string data;

        uint32_t position;
        uint32_t idPos;
        struct pcap_pkthdr* header;
        uint8_t* packet;

        DnsRR(uint32_t p, uint32_t idP, struct pcap_pkthdr* h, uint8_t* pt)
        {
            this->position = p;
            this->idPos = idP;
            this->header = h;
            this->packet = pt;
        }

        // Parse an individual resource record, placing the acquired data in 'rr'.
        // 'packet', 'pos', and 'idPos' serve the same uses as in parse_rr_set.
        // Return 0 on error, the new 'pos' in the packet otherwise.
        uint32_t ParseRR()
        {
            int i;
            uint32_t rr_start = this->position;

            this->name = "";
            this->data = "";

            this->name = this->readRRName();
    
            // Handle a bad rr name.
            // We still want to print the rest of the escaped rr data.
            if (this->name.empty())
            {
                LOGGING("Bad rr name");
                return 0;
            }

            if ((header->len - this->position) < 10)
            {
                return 0;
            }

            this->type = (packet[this->position] << 8) + packet[this->position + 1];
            this->rdlength = (packet[this->position + 8] << 8) + packet[this->position + 9];

            this->cls = (packet[this->position + 2] << 8) + packet[this->position + 3];
            this->ttl = 0;
            for (i=0; i<4; i++)
            {
                this->ttl = (this->ttl << 8) + packet[this->position + 4 + i];
            }

            // Make sure the data for the record is actually there.
            // If not, escape and print the raw data.
            if (header->len < (rr_start + 10 + this->rdlength))
            {
                LOGGING("Truncated rr");
                return 0;
            }

            LOGGING("Applying RR parser for type: " << this->type);

            this->position = this->position + 10;
            // Parse the resource record data.

            this->applyParser();
    
            // fprintf(stderr, "rr->name: %s\n", rr->name);
            // fprintf(stderr, "type %d, cls %d, ttl %d, len %d\n", rr->type, rr->cls, rr->ttl,
            //        rr->rdlength);
            // fprintf(stderr, "rr->data %s\n", rr->data);

            return this->position + this->rdlength;
        }
};