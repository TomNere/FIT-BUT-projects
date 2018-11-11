
#include <stdint.h> // Getopt
#include <pcap.h>   // No comment needed...
#include <string>
#include <sstream>      // std::stringstream
#include "structures.h"
#include "helpers.cpp"

using namespace std;

// Holds the information for a dns resource record
// Only used for answers in our case
class DnsRR
{
    /*********************************************** PRIVATE Variables ********************************************/

    uint16_t cls;
    uint16_t ttl;
    uint16_t rdlength;

    uint32_t position;
    uint32_t idPosition;
    struct pcap_pkthdr header;
    uint8_t* packet;

    /*********************************************** PRIVATE Methods ********************************************/

    // Return domain name
    string readDomainName(uint32_t* position)
    {
        uint32_t pos;
        pos = *position;
        string name = "";
        
        while (packet[pos] != 0)
        {
            uint8_t ch = packet[pos];
            
            if ((ch >> 6) == 0b11)          // Check for compression (first two bits enabled)
            {
                uint16_t offset = 0;

                // The pointer takes the form of a two octet sequence:
                //
                //+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
                //| 1  1|                OFFSET                   |
                //+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--

                offset = ((ch & 0b00111111) << 8) | packet[pos + 1]; // First two bits are offset , length is max 63
                uint32_t tmp = this->idPosition + offset;

                string label = this->readDomainName(&tmp);      // Recursion

                name += label;
                *position = pos + 2;
                return name;
            }
            else
            {
                int length = ch & 0b00111111; // First two bits are offset , length is max 63
                for (int i = 0; i < length; i++)
                {
                    char c = packet[++pos];
                    if (c >= '!' && c <= '~' && c != '\\')
                    {
                        name += c;
                    } 
                    else
                    {
                        name += '\\';
                        name += 'x';
                        char c1 = c/16 + 0x30;
                        char c2 = c%16 + 0x30;
                        if (c1 > 0x39) c1 += 0x27;
                        if (c2 > 0x39) c2 += 0x27;
                        name += c1;
                        name += c2;
                    }

                }
                name += ".";
                pos++;
            }
        }

        if (name.size() > 0)
        {
            name = name.substr(0, name.size()-1);   // Remove last '.'
        }
        
        *position = pos + 1;
        
        return name;
    }

    void applyParser()
    {
        switch (this->type)
        {
            case 1:
                this->typeStr  = "A";
                this->parserA();
                break;
            case 28:
                this->typeStr  = "AAAA";
                this->parserAAAA();
                break;
            case 5:
                this->typeStr  = "CNAME";
                this->parserDOMAIN_NAME();
                break;
            case 15:
                this->typeStr  = "MX";
                this->parserMX();
                break;
            case 2:
                this->typeStr  = "NS";
                this->parserDOMAIN_NAME();
                break;
            case 6:
                this->typeStr  = "SOA";
                this->parserSOA();
                break;
            case 16:
                this->typeStr  = "TXT";
                this->parserTXT();
                break;
            case 99:
                this->typeStr  = "SPF";
                // TODO
            // DNSSEC
            case 48:
                this->typeStr  = "DNSKEY";
                this->parserDNSKEY();
                break;
            case 46:
                this->typeStr  = "RRSIG";
                this->parseRRSIG();
                break;
             case 47:
                this->typeStr  = "NSEC";
                this->parserNSEC();
                break;
            case 43:
                this->typeStr  = "DS";
                this->parserDS();
                break;
            default:
                this->typeStr  = "Unknown";
        }
    }

    /****************************************** RR parsers *****************************************************/

    // Parser for A record(IPv4 address)
    //  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    //  |                    ADDRESS                    |
    //  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    void parserA()
    {
        // must be 32-bit
        if (rdlength != 4)
        {
            return;
        }

        stringstream ss;

        ss << (int)packet[this->position] << "." << (int)packet[this->position + 1] << "." << (int)packet[this->position + 2] << "." << (int)packet[this->position + 3];
        ss >> this->data;
    }

    // Parser for AAAA record(IPv6 address)
    void parserAAAA()
    {
        uint16_t ipv6[8];
        int i;

        // Must be 128 bit
        if (this->rdlength != 16)
        {
            return;
        }

        stringstream ss;
        ss ;
        for (i = 0; i < 16; i += 2)
        {
            uint16_t number = *(uint16_t*)packet + this->position + i;
            ss << hex << number << ":";
        }
        
        ss << packet[this->position + 17];  // last number without ':'

        //ss << hex << (uint16_t)packet[this->position + 1] << ":";

        //ss << hex << ipv6[0] << ":" << ipv6[1] << ":" << ipv6[2] << ":" << ipv6[3] << ":" << ipv6[4] << ":" << ipv6[5] << ":" << ipv6[6] << ":" << ipv6[7];
    
        ss >> this->data;
    }

    // Parser for domain name 
    // A DNS like name. This format is used for many record types
    void parserDOMAIN_NAME()
    {
        uint32_t tmp = this->position;
        string name = this->readDomainName(&tmp);
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

        mname = readDomainName(&(this->position));
        if (mname.empty())
        {
            LOGGING("Invalid SOA mname");
            return;
        }

        rname = readDomainName(&(this->position));
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
        name = readDomainName(&(this->position));
        if (name.empty()) 
            return;

        stringstream ss;
        ss << pref << "," << name;
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
        signer = readDomainName(&(this->position));
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
        domain = readDomainName(&(this->position));

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

    /****************************************** PUBLIC Variables and Methods ****************************************/

    public:
        string domainName;  // Domain name
        uint16_t type;      // RR type
        string typeStr;     // RR type but string
        string data;        // data

        DnsRR(uint32_t p, uint32_t idP, struct pcap_pkthdr h, uint8_t* pt)
        {
            this->position = p;
            this->idPosition = idP;
            this->header = h;
            this->packet = pt;
        }

        // Parse resource records
        // Return 0 if error occures
        uint32_t Parse()
        {
            uint32_t initPos = this->position;     // Store initial position

            //                                  1  1  1  1  1  1
            //    0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
            //  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            //  |                                               |
            //  /                                               /
            //  /                      NAME                     /
            //  |                                               |
            //  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            //  |                      TYPE                     |
            //  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            //  |                     CLASS                     |
            //  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            //  |                      TTL                      |
            //  |                                               |
            //  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            //  |                   RDLENGTH                    |
            //  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--|
            //  /                     RDATA                     /
            //  /                                               /
            //  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

            this->domainName = this->readDomainName(&(this->position));
    
            // Unable to get domain name - error
            if (this->domainName.empty())
            {
                return 0;
            }

            // Check if type, class, ttl etc. are here
            if ((header.len - this->position) < 10)
            {
                return 0;
            }

            this->type = packet[this->position + 1];        // RR type
            this->rdlength = packet[this->position + 9];    // data length

            // this->cls = (packet[this->position + 2] << 8) + packet[this->position + 3];
            // this->ttl = 0;
            // for (int i = 0; i < 4; i++)
            // {
            //     this->ttl = (this->ttl << 8) + packet[this->position + 4 + i];
            // }

            // Check if data are here
            if (header.len < (initPos + 10 + this->rdlength))
            {
                return 0;
            }

            // Set position to rdata
            this->position = this->position + 10;
            // Parse the resource record data.

            this->applyParser();

            return this->position + this->rdlength;
        }
};