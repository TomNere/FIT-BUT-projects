
#include <stdint.h> // Getopt
#include <pcap.h>   // No comment needed...
#include <string>
#include <sstream>      // std::stringstream
#include "structures.h"
#include "helpers.cpp"

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
        string name = "";
        cout << this->position << " "<< this->header->len << " " << this->idPos << endl;

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
                    if (endPos == 0)
                    {
                        endPos = pos + 1;
                    }
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
                    nameLen++;
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
            return "";
        }

        nameLen++;
        cout << "nameLen" << nameLen << endl;
        for (int i = 0; i < nameLen; i++)
        {
            name += '\0';
        }

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

        int index = name.find_first_of('\0');
        name.resize(index);

        LOGGING("Returning RR name: " << name);
        name.shrink_to_fit();
        return name;
    }

    void applyParser()
    {
        switch (this->type)
        {
            case 1:
                this->rrName = "A";
                this->parserA();
                break;
            case 28:
                this->rrName = "AAAA";
                this->parserAAAA();
                break;
            case 5:
                this->rrName = "CNAME";
                this->parserDOMAIN_NAME();
                break;
            case 15:
                this->rrName = "MX";
                this->parserMX();
                break;
            case 2:
                this->rrName = "NS";
                this->parserDOMAIN_NAME();
                break;
            case 6:
                this->rrName = "SOA";
                this->parserSOA();
                break;
            case 16:
                this->rrName = "TXT";
                this->parserTXT();
                break;
            case 99:
                this->rrName = "SPF";
                // TODO
            // DNSSEC
            case 48:
                this->rrName = "DNSKEY";
                this->parserDNSKEY();
                break;
            case 46:
                this->rrName = "RRSIG";
                this->parseRRSIG();
                break;
             case 47:
                this->rrName = "NSEC";
                this->parserNSEC();
                break;
            case 43:
                this->rrName = "DS";
                this->parserDS();
                break;
            default:
                this->rrName = "Unknown";
        }
    }

    /****************************************** RR parsers *****************************************************/

    // A (IPv4 address) format
    // A records are simply an IPv4 address, and are formatted as such
    void parserA()
    {
        if (rdlength != 4)
        {
            return;
        }

        stringstream ss;

        ss << (int)packet[this->position] << "." << (int)packet[this->position + 1] << "." << (int)packet[this->position + 2] << "." << (int)packet[this->position + 3];
        ss >> this->data;
        cout << "parserA data: " << this->data << endl;
    }

    // domain name like format
    // A DNS like name. This format is used for many record types
    void parserDOMAIN_NAME()
    {
        uint32_t pos = this->position;
        string name = this->readRRName();
        this->position = pos;
        if (name.empty())
        {
            LOGGING("Invalid domain name.")
        }
        this->data = name;
    }

    // Start of Authority format
    // Presented as a series of labeled SOA fields.
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

    // Mail Exchange record format
    // A standard dns name preceded by a preference number.
    void parserMX()
    {

        uint16_t pref = (this->packet[this->position] << 8) + this->packet[this->position + 1];
        string name;
        string buffer;
        uint32_t spos = this->position;

        this->position = this->position + 2;
        name = readRRName();
        if (name.empty()) 
            return;

        stringstream ss;
        ss << pref << "," << name;
        ss >> this->data; 
    }

    // IPv6 record format.  RFC 3596
    // A standard IPv6 address. No attempt is made to abbreviate the address.
    void parserAAAA()
    {
        char *buffer;
        uint16_t ipv6[8];
        int i;

        if (this->rdlength != 16)
        {
            return;
        }

        for (i=0; i < 8; i++) 
            ipv6[i] = (packet[this->position + i * 2] << 8) + packet[this->position + i * 2 + 1];

        stringstream ss;
        ss << hex << ipv6[0] << ":" << ipv6[1] << ":" << ipv6[2] << ":" << ipv6[3] << ":" << ipv6[4] << ":" << ipv6[5] << ":" << ipv6[6] << ":" << ipv6[7];
    
        ss >> this->data;
    }

    // dnssec Key format. RFC 4034
    // format: flags, proto, algorithm, key
    // All fields except the key are printed as decimal numbers.
    // The key is given in base64.
    void parserDNSKEY()
    {
        uint16_t flags = (packet[this->position] << 8) + packet[this->position + 1];
        uint8_t proto = packet[this->position + 2];
        uint8_t algorithm = packet[this->position + 3];
        char* key;

        key = b64encode(packet, this->position + 4, this->rdlength - 4);

        stringstream ss;
        ss << flags << "," << proto << "," << algorithm << "," << key;
        ss >> this->data;
    }

    // DNS SEC Signature. RFC 4304
    // format: tc,alg,labels,ottl,expiration,inception,tag signer signature
    // All fields except the signer and signature are given as decimal numbers
    // The signer is a standard DNS name
    // The signature is base64 encoded
    void parseRRSIG()
    {
        uint32_t o_pos = this->position;
        uint16_t tc = (packet[this->position] << 8) + packet[this->position + 1];
        uint8_t alg = packet[this->position + 2];
        uint8_t labels = packet[this->position + 3];
        u_int ottl, sig_exp, sig_inc;
        uint16_t key_tag = (packet[this->position + 16] << 8) + packet[this->position + 17];
        string signer;
        char* signature;

        this->position += 4;
        ottl = (packet[this->position] << 24) + (packet[this->position + 1] << 16) + (packet[this->position + 2] << 8) + packet[this->position + 3];
        this->position += 4;
        sig_exp = (packet[this->position] << 24) + (packet[this->position + 1] << 16) + (packet[this->position + 2] << 8) + packet[this->position + 3];
        this->position += 4; 
        sig_inc = (packet[this->position] << 24) + (packet[this->position + 1] << 16) + (packet[this->position + 2] << 8) + packet[this->position + 3];
        this->position += 6;

        uint16_t o_rdlength = this->rdlength;
        this->rdlength += o_pos;
        signer = readRRName();
        this->rdlength = o_rdlength;

        if (signer.empty())
        {
            return;
        }
        
        signature = b64encode(packet, this->position, o_pos + rdlength - this->position);
    
        stringstream ss;
        ss << tc << "," << alg << "," << labels << "," << ottl << "," << sig_exp << "," << sig_inc << "," << key_tag << "," << signer << "," << signature;
        ss >> this->data;
    }

    // NSEC format.  RFC 4034
    // Format: domain bitmap
    // domain is a DNS name, bitmap is hex escaped
    void parserNSEC()
    {
        string buffer, domain, bitmap;

        uint16_t oldRdlength = this->rdlength;
        this->rdlength += this->position;
        domain = readRRName();

        if (domain.empty())
        {
            return;
        } 
    
        bitmap = escape_data(packet, this->position, this->position + oldRdlength);
        stringstream ss;
        ss << domain << "," << bitmap;
        ss >> this->data;
    }

    //DS DNS SEC record.  RFC 4034
    //format: key_tag,algorithm,digest_type,digest
    //The keytag, algorithm, and digest type are given as base 10
    //The digest is base64 encoded
    void parserDS()
    {
        uint16_t key_tag = (packet[this->position] << 8) + packet[this->position + 1];
        uint8_t alg = packet[this->position + 2];
        uint8_t dig_type = packet[this->position + 3];
        char* digest = b64encode(packet ,this->position + 4, this->rdlength - 4);

        stringstream ss;
        ss << key_tag << "," << alg<< "," << dig_type<< "," << digest;
        ss >> this->data;
    }

    
    // This data is simply hex escaped
    // Non printable characters are given as a hex value (\\x30), for example
    void parserTXT()
    {
        this->data = escape_data(this->packet, this->position, this->position + this->rdlength);
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
        uint32_t SkipQuestion()
        {
            uint32_t start_pos = this->position; 

            string name = this->readRRName();
            if (name.empty() || (this->position + 2) >= this->header->len)
            {
                return 0;
            }

            return (this->position + 4);
        }

        // Parse an individual resource record, placing the acquired data in 'rr'.
        // 'packet', 'pos', and 'idPos' serve the same uses as in parse_rr_set.
        // Return 0 on error, the new 'pos' in the packet otherwise.
        uint32_t ParseRRAnswer()
        {
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
                ERR_RET("HERE");
                return 0;
            }

            this->type = (packet[this->position] << 8) + packet[this->position + 1];
            this->rdlength = (packet[this->position + 8] << 8) + packet[this->position + 9];

            this->cls = (packet[this->position + 2] << 8) + packet[this->position + 3];
            this->ttl = 0;
            for (int i = 0; i < 4; i++)
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

            cout << "returning pos: " << this->position << " rdlength: " << this->rdlength << endl;
            return this->position + this->rdlength;
        }
};