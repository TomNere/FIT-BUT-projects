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
    uint16_t rdLength;

    uint32_t position;
    uint32_t idPosition;
    struct pcap_pkthdr header;
    uint8_t* packet;

    /*********************************************** PRIVATE Methods ********************************************/

    // Return domain name
    string readDomainName(uint32_t* position)
    {
        uint32_t pos = *position;
        string domainName = "";
        
        while (this->packet[pos] != 0)
        {
            uint8_t ch = this->packet[pos];
            
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

                domainName += label;
                *position = pos + 2;
                return domainName;
            }
            else
            {
                int length = ch & 0b00111111; // First two bits are offset , length is max 63
                for (int i = 0; i < length; i++)
                {
                    uint8_t ch = this->packet[++pos];
                    if (isprint(ch))
                    {
                        domainName += ch;
                    }
                    else
                    {
                        domainName += escapeChar(ch);
                    }
                }
                domainName += ".";
                pos++;
            }
        }

        if (domainName.size() > 0)
        {
            domainName = domainName.substr(0, domainName.size()-1);   // Remove last '.'
        }
        
        *position = pos + 1;
        
        return domainName;
    }

    // Set typeStr and call appropriate parser
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
                // RFC says this answer never arrive but....
                this->typeStr  = "SPF";
                this->parserTXT();
                break;
            /***************************** DNSSEC *****************************/
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
    void parserA()
    {
        // IPv4 must be 32-bit
        if (rdLength != 4)
        {
            return;
        }

        stringstream ss;

        ss << (int)this->packet[this->position] << "." << (int)this->packet[this->position + 1] << "." 
           << (int)this->packet[this->position + 2] << "." << (int)this->packet[this->position + 3];
        this->data = ss.str();
    }

    // Parser for AAAA record(IPv6 address)
    void parserAAAA()
    {
        // IPv6 must be 128 bit
        if (this->rdLength != 16)
        {
            return;
        }

        stringstream ss;
        ss << hex;

        for (int i = 0; i < 14; i += 2)
        {
            ss << ntohs(*(uint16_t*)(this->packet + this->position + i)) << ":";
        }
        
        ss << ntohs(*(uint16_t*)(this->packet + this->position + 14));   // last number without ':'
        this->data = ss.str();
    }

    // Parser for records containing domain name (CNAME, NS...)
    void parserDOMAIN_NAME()
    {
        uint32_t tmp = this->position;          // Use tmp to not change position in packet (we've got rdata length...)
        this->data= this->readDomainName(&tmp);
    }

    // Parser for MX record (domain name with preference)
    // MX records cause type A additional section processing for the host specified by EXCHANGE
    
    void parserMX()
    {
        //  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        //  |                  PREFERENCE                   |
        //  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        //  /                   EXCHANGE                    /
        //  /                                               /
        //  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

        // A 16 bit integer which specifies the preference given to
        // this RR among others at the same owner.  Lower values are prefered
        uint16_t preference = ntohs(*(uint16_t*)(this->packet + this->position));

        uint32_t tmp = this->position + 2;
        string domainName = readDomainName(&tmp);

        stringstream ss;
        ss << '"' << preference << " " << domainName << '"';
        this->data = ss.str();
    }

    // Parser for SOA record
    
    void parserSOA()
    {
        //  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        //  /                     MNAME                     /
        //  /                                               /
        //  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        //  /                     RNAME                     /
        //  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        //  |                    SERIAL                     |
        //  |                                               |
        //  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        //  |                    REFRESH                    |
        //  |                                               |
        //  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        //  |                     RETRY                     |
        //  |                                               |
        //  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        //  |                    EXPIRE                     |
        //  |                                               |
        //  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        //  |                    MINIMUM                    |
        //  |                                               |
        //  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

        uint32_t position = this->position;

        // The <domain-name> of the name server that was the
        // original or primary source of data for this zone.
        string mName = readDomainName(&position);

        // A <domain-name> which specifies the mailbox of the
        // person responsible for this zone.
        string rName = readDomainName(&position);

        // The unsigned 32 bit version number of the original copy
        // of the zone.  Zone transfers preserve this value.  This
        // value wraps and should be compared using sequence space
        // arithmetic.
        uint32_t serial = ntohl(*(uint32_t*)(this->packet + position));

        // A 32 bit time interval before the zone should be
        // refreshed.
        uint32_t refresh = ntohl(*(uint32_t*)(this->packet + position + 4));

        // A 32 bit time interval that should elapse before a
        // failed refresh should be retried.
        uint32_t retry = ntohl(*(uint32_t*)(this->packet + position + 8));

        // A 32 bit time value that specifies the upper limit on
        // the time interval that can elapse before the zone is no
        // longer authoritative.
        uint32_t expire = ntohl(*(uint32_t*)(this->packet + position + 12));
        
        // The unsigned 32 bit minimum TTL field that should be
        // exported with any RR from this zone.
        uint32_t minimum = ntohl(*(uint32_t*)(this->packet + position + 16));

        stringstream ss;
        ss << '"' << mName << " " << rName << " " << serial << " " << refresh 
           << " " << retry << " " << expire << " " << minimum << '"';
        this->data = ss.str();
    }

    // Parser for TXT/SPF record (One or more <character-string>s.)
    // TXT could contain SPF but we don't care
    void parserTXT()
    {
        for (int i = 0; i < this->rdLength; i++)
        {
            uint8_t ch = this->packet[this->position + i];
            if (isprint(ch))
            {
                this->data += ch;
            }
            else
            {
                this->data += escapeChar(ch);
            }
        }
    }

    /********************************************** DNSSEC *****************************************/

    // Parser for DNSKEY
    // DNSSEC uses public key cryptography to sign and authenticate DNS
    // resource record sets (RRsets).
    void parserDNSKEY()
    {
        //                      1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
        //  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        // |              Flags            |    Protocol   |   Algorithm   |
        // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        // /                                                               /
        // /                            Public Key                         /
        // /                                                               /
        // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

        // flags (zone key etc.)
        uint16_t flags = ntohs(*(uint16_t*)(this->packet + this->position));

        // protocol must be 3, else invalid 
        uint8_t protocol = packet[this->position + 2];
        if (protocol != 3)
        {
            return;
        }

        // identifies the public key's cryptographic
        // algorithm and determines the format of the Public Key field
        uint8_t algorithm = packet[this->position + 3];

        string publicKey = base64_encode(packet + this->position + 4, this->rdLength - 4);

        stringstream ss;
        ss << '"' << flags << " " << protocol << " " << algorithm << " " << publicKey << '"';
        this->data = ss.str();
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

        uint16_t o_rdlength = this->rdLength;
        this->rdLength += o_pos;
        signer = readDomainName(&(this->position));
        this->rdLength = o_rdlength;

        if (signer.empty())
        {
            return;
        }
        
        signature = b64encode(packet, this->position, o_pos + rdLength - this->position);
    
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

        uint16_t oldRdlength = this->rdLength;
        this->rdLength += this->position;
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
        char* digest = b64encode(packet ,this->position + 4, this->rdLength - 4);

        stringstream ss;
        ss << key_tag << "," << alg<< "," << dig_type<< "," << digest;
        ss >> this->data;
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
            this->rdLength = packet[this->position + 9];    // data length

            // this->cls = (packet[this->position + 2] << 8) + packet[this->position + 3];
            // this->ttl = 0;
            // for (int i = 0; i < 4; i++)
            // {
            //     this->ttl = (this->ttl << 8) + packet[this->position + 4 + i];
            // }

            // Check if data are here
            if (header.len < (initPos + 10 + this->rdLength))
            {
                return 0;
            }

            // Set position to rdata
            this->position = this->position + 10;
            // Parse the resource record data.

            this->applyParser();

            return this->position + this->rdLength;
        }
};