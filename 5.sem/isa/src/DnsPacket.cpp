#include "DnsData.cpp"
#include "structures.h"

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

class DnsPacket
{
    uint8_t packet;
    struct pcap_pkthdr header;
    uint32_t position;
    int protType;
    uint32_t transProt;
    DnsData dns;

    uint32_t getTransProt()
    {
        struct ether_header *eptr;
        struct ip* myIp;
        struct ip6_hdr* myIpv6;

        // read the Ethernet header
        eptr = (struct ether_header*) packet;
        this->position += sizeof(struct ether_header);


        // parse ethernet type 
        switch (ntohs(eptr->ether_type))
        {               
            // see /usr/include/net/ethernet.h for types
            case ETHERTYPE_IP:
                LOGGING("Ethernet type is IPv4");
                myIp = (struct ip*) (packet + this->position);              // skip Ethernet header
                this->position += sizeof(struct ip);
                this->protType = myIp->ip_p;
                break;

            case ETHERTYPE_IPV6:
                LOGGING("Ethernet type is IPv6");
                myIpv6 = (struct ip6_hdr*) (packet + this->position);
                this->position += sizeof(struct ip6_hdr);
                this->protType = myIpv6->ip6_ctlun.ip6_un1.ip6_un1_nxt;
                break;
            default:
                LOGGING("Invalid ethernet type for DNS");
                return 0;
        }

        switch (protType)
        {
            case UDP:
                LOGGING("Transport protocol UDP");
                this->position += sizeof(struct udphdr);
                return UDP;
            case TCP: 
                LOGGING("Transport protocol TCP");
                this->position += sizeof(struct tcphdr);
                return TCP;
            default: 
                LOGGING("Invalid transport protocol for DNS");
                return 0;
        }
    }

    // Parse the dns protocol in 'packet'
    // See RFC1035
    // See dns_parse.h for more info
    void dnsParse()
    {
        uint32_t idPos = this->position;

        if (this->header.len - this->position < 12)
        {
            LOGGING("Truncated Packet(dns). Skipping");
            return;
        }

        //uint8_t* new_packet = &(this->packet);

        dns.id = ((&packet)[this->position] << 8) + (&packet)[this->position + 1];
        dns.qr = (&packet)[this->position + 2] >> 7;
        dns.AA = ((&packet)[this->position + 2] & 0x04) >> 2;
        dns.TC = ((&packet)[this->position + 2] & 0x02) >> 1;
        dns.rcode = (&packet)[this->position + 3] & 0x0f;

        // rcodes > 5 indicate various protocol errors and redefine most of the 
        // remaining fields. Parsing this would hurt more than help. 
        if (dns.rcode > 5) {
            LOGGING("Rcode > 5. Skipping");
            return;
        }

        // Counts for each of the record types.
        int qdCount = ((&packet)[this->position + 4] << 8) + (&packet)[this->position + 5];
        dns.ancount = ((&packet)[this->position + 6] << 8) + (&packet)[this->position + 7];

        // Skip questions
        this->position = this->position + 12 + (qdCount * 4);

        // Parse answer records
        this->parseRRSet(idPos);
    }

    // Parse a set of resource records in the dns protocol in 'packet', starting
    // at 'pos'. The 'idPos' offset is necessary for putting together 
    // compressed names. 'count' is the expected number of records of this type.
    // 'root' is where to assign the parsed list of objects.
    // Return 0 on error, the new 'pos' in the packet otherwise.
    void parseRRSet(uint32_t idPos)
    {
        for (int i = 0; i < this->dns.ancount; i++)
        {
            // Create and clear the data in a new dnsRR object.
            DnsRR current;
            current.name = ""; 
            current.data = "";

            this->position = current.ParseRR(this->position, idPos, &(this->header), &(this->packet));

            // If a non-recoverable error occurs when parsing an rr, 
            // we can only return what we've got and give up.
            if (this->position == 0)
            {
                LOGGING("Error occured when RR parsing")
                return;
            }
            this->dns.answers.push_front(current);
        }
    }

    public:
        DnsPacket(uint8_t p, struct pcap_pkthdr h)
        {
            this->packet = p;
            this->header.ts = h.ts;
            this->header.caplen = h.caplen;
            this->header.len = h.len;
            this->position = 0;
        }

        void Parse()
        {
            this->getTransProt();

            switch (this->protType)
            {
                case UDP:
                    this->dnsParse();

                    if (this->position != 0)
                    {
                        printSummary(&ip, &dns, &header);
                    }
                    break;
                case TCP:
                    // TODO
                    LOGGING("TCP not implemented yet");
                    break;
                default:
                    return;
            }
        }
}
