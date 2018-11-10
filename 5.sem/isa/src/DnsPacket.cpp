#include "structures.h"
#include "DnsRR.cpp"

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/if_ether.h>
#include <pcap.h>

#include <arpa/inet.h>
#include <netinet/ether.h> 

using namespace std;

/*********************************************** Consts ********************************************/

#define UDP 0x11
#define TCP 0x06

// Class holding (maybe) DNS packet
class DnsPacket
{
    /*********************************************** PRIVATE Variables ********************************************/
    
    uint8_t* packet;
    struct pcap_pkthdr header;
    uint32_t position;
    uint16_t ancount;

    /*********************************************** PRIVATE Methods ********************************************/

    // Parse ethernet and transport protocol
    // Return true if success
    bool getTransProt()
    {
        struct ether_header* eptr;
        struct ip* myIp;
        struct ip6_hdr* myIpv6;
        uint8_t transProtType;

        // Read the ethernet header
        eptr = (struct ether_header*) packet;
        this->position += sizeof(struct ether_header);

        // Check ethernet type 
        switch (ntohs(eptr->ether_type))
        {
            // see /usr/include/net/ethernet.h for types (ip, ip6_header...)
            case ETHERTYPE_IP:
                myIp = (struct ip*)(packet + this->position);              
                this->position += sizeof(struct ip);                // skip Ethernet IP header
                transProtType = myIp->ip_p;
                break;
            case ETHERTYPE_IPV6:
                myIpv6 = (struct ip6_hdr*)(packet + this->position);
                this->position += sizeof(struct ip6_hdr);           // skip Ethernet IPv6 header
                transProtType = myIpv6->ip6_ctlun.ip6_un1.ip6_un1_nxt;
                break;
            default:
                return false;       // Unsupported
        }

        // Skip position according to transport protocol type
        switch (transProtType)
        {
            case UDP:
                this->position += sizeof(struct udphdr);
                break;
            case TCP: 
                this->position += sizeof(struct tcphdr);
            default:
                return false;       // Unsupported
        }

        return true;
    }

    // Parse the dns protocol in 'packet'
    // See RFC1035
    // See dns_parse.h for more info
    void dnsParse()
    {
        //  The header contains the following fields:
        //
        //                                 1  1  1  1  1  1
        //   0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
        // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        // |                      ID                       |
        // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        // |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
        // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        // |                    QDCOUNT                    |
        // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        // |                    ANCOUNT                    |
        // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        // |                    NSCOUNT                    |
        // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        // |                    ARCOUNT                    |
        // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

        // Check if DNS header is 12 bytes long
        // 2 byte ID, 2 bytes of flags, and 4 x 2 bytes of counts.
        if (this->header.len - this->position < 12)
        {
            return;     // Skip this packet...
        }

        // Store ID position for future use when name is compressed
        uint32_t idPos = this->position;

        // Check if flags are valid for DNS packet
        // we don't care about ID (by design), qr is after 2 byte ID
        // need bitshift because QR is one bit
        uint8_t qr = packet[this->position + 2] >> 7;
        if (qr == 0)
        {
            return;     // Message is query, skip
        }
        
        // Check rcode for errors
        // need & because rcode has size 4 bits
        uint8_t rcode = packet[this->position + 3] & 0b00001111;
        if (rcode > 5)
        {
            return;     // > 5 signalize errors
        }


        // Question count 
        uint16_t qdcount = packet[this->position + 5];
        if (qdcount != 1)
        {
            return;     // question count other than 1 in DNS packet is very weird
        }

        // Answer count, other counts are useless for us
        this->ancount = packet[this->position + 7];

        this->position += 12;   // Set possition after header

        // Parse answer records
        this->parseRRSet(idPos);
    }

    // Skip question and parse answers
    void parseRRSet(uint32_t idPos)
    {
        // Skip question section because we don't need it's data
        this->SkipQuestion();

        // Parse answers
        for (int i = 0; i < this->ancount; i++)
        {
            // Create and clear the data in a new dnsRR object.
            DnsRR current(this->position, idPos, &(this->header), this->packet);
            current.name = ""; 
            current.data = "";

            this->position = current.ParseRR();

            // If a non-recoverable error occurs when parsing an rr, 
            // we can only return what we've got and give up.
            if (this->position == 0)
            {
                LOGGING("Error occured when RR answer parsing");
                return;
            }
            this->Answers.push_front(current);
        }
    }

    // Skip Question section
    void SkipQuestion()
    {
        //  Question section contains the following fields:
        //                                    1  1  1  1  1  1
        //      0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
        //    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        //    |                                               |
        //    /                     QNAME                     /
        //    /                                               /
        //    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        //    |                     QTYPE                     |
        //    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        //    |                     QCLASS                    |
        //    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        //
        // Qname contains labels, there's no compression so we can read bytes until "0" label occures
        while(this->packet[this->position++] != 0)
        {
            ;
        }
        // Skip 2 byte long qtype and 2 byte long qclass
        this->position += 4);
    }

    /****************************************** PUBLIC Variables and Methods ****************************************/

    public:
        list<DnsRR> Answers;

        // Initialize packet, header and position in constructor
        DnsPacket(const uint8_t* p, const struct pcap_pkthdr* h)
        {
            this->packet = (uint8_t*) p;
            this->header.ts = h->ts;
            this->header.caplen = h->caplen;
            this->header.len = h->len;
            this->position = 0;
        }

        // Try to parse packet
        void Parse()
        {
            if (this->getTransProt())
            {
                this->dnsParse();
            }
        }
};
