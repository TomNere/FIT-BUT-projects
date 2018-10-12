#include <iostream> // No comment needed...
#include <unistd.h> // Getopt
#include <signal.h> // Signal shandling
#include <pcap.h>   // No comment needed...
#include <arpa/inet.h>
#include <string.h>
#include <list>
#include <iterator>
#include <sstream>
#include "helpers.cpp"

using namespace std;
int packetCount = 0;
int datalink;

struct ip_fragment* ipFragmentHead = NULL;

/******************************************************** CLASSES ************************************************************/

class DnsRecord
{
	string domain;
	int rrType;
	string rrAnswer;
	uint count;

    string getRRType()
    {
        switch (rrType)
        {
            case 1:
                return "A";
            case 28:
                return "AAAA";
            case 5:
                return "CNAME";
            case 15:
                return "MX";
            case 2:
                return "NS";
            case 6:
                return "SOA";
            case 16:
                return "TXT";
            case 99:
                return "SPF";
            // DNSSEC
            case 32769:
                return "DLV";
            case 48:
                return "DNSKEY";
            case 43:
                return "DS";
            case 25:
                return "KEY";
            case 47:
                return "NSEC";
            case 50:
                return "NSEC3";
            case 46:
                return "RRSIG";
            case 24:
                return "SIG";
            case 32768:
                return "TA";
            case 250:
                return "TSIG";
            case 30:
                return "NXT";
            default:
                return "Unknown";
        }
}
        

    public:

        DnsRecord(string d, int rt, string ra, int c)
        {
            domain = d;
            rrType = rt;
            rrAnswer = ra;
            count = c;
        }

        bool IsEqual(DnsRecord)
        {
            return true;
        }

        void AddRecord()
        {
            count++;
        }

        string GetString()
        {
            string str;
            stringstream ss;
            ss << "rrtype: " << getRRType() << " , count: " << count;
            return ss.str();
        }
};

class RecordCollection
{
    list<DnsRecord> innerList;

    void AddRecord(DnsRecord record)
    {
        list <DnsRecord> :: iterator it; 
        
        for(it = innerList.begin(); it != innerList.end(); it++) 
        {
            if ((*it).IsEqual(record))
            {
                (*it).AddRecord();
                return;
            }
            innerList.push_front(record);
        }
    }

};

/******************************************************** CONST & MACROS *******************************************************/

#define ERR_RET(message)     \
    cerr << message << " Terminating." << endl; \
    exit(EXIT_FAILURE);

#define LOGGING(message) \
    cerr << "LOG: ";     \
    message << endl;

const string help = "Invalid parameters!\n\n"
                    "Usage: dns-export [-r file.pcap] [-i interface] [-s syslog-server] [-t seconds] \n";

/*****************************************************************************************************************************/

/********************************************************** METHODS **********************************************************/

// Write stats to stdout
void writeStats()
{
}

// Signal handler
// Write stats end terminate
void handleSig(int sigNum)
{
    writeStats();
}

// Argument parser
raw_parameters parseArg(int argc, char const *argv[])
{
    raw_parameters params;
    params.file = params.interface = params.syslogServer = params.time = "";
    int option;

    while ((option = getopt(argc, (char **)argv, "r:i:s:t:")) != -1)
    {
        switch (option)
        {
        case 'r':
            if (params.file.compare(""))
            {
                ERR_RET(help);
            }

            params.file = string(optarg);
            break;
        case 'i':
            if (params.interface.compare(""))
            {
                ERR_RET(help);
            }

            params.interface = string(optarg);
            break;
        case 's':
            if (params.syslogServer.compare(""))
            {
                ERR_RET(help);
            }

            params.syslogServer = string(optarg);
            break;
        case 't':
            if (params.time.compare(""))
            {
                ERR_RET(help);
            }

            params.time = string(optarg);
            break;
        default:
            ERR_RET(help);
        }
    }

    return params;
}

// Parse the IPv4 header. May point p_packet to a new packet data array,
// which means zero is a valid return value. Sets p_packet to NULL on error.
// See RFC791
uint32_t ipv4Parse(uint32_t pos, struct pcap_pkthdr *header, 
                    uint8_t ** p_packet, ipInfo * ip) {

    uint32_t h_len;
    ip_fragment* frag = NULL;
    uint8_t frag_mf;
    uint16_t frag_offset;

    // For convenience and code consistency, dereference the packet **.
    uint8_t* packet = *p_packet;

    if (header-> len - pos < 20) {
        fprintf(stderr, "Truncated Packet(ipv4)\n");
        *p_packet = NULL;
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
        frag = ip_frag_add(frag, ipFragmentHead);
        if (frag != NULL) {
            // Update the IP info on the reassembled data.
            header->len = ip->length = frag->end - frag->start;
            *p_packet = frag->data;
            free(frag);
            return 0;
        }
        // Signals that there is no more work to do on this packet.
        *p_packet = NULL;
        return 0;
    } 

    // move the position up past the options section.
    return pos + 4*h_len;
}


// Parse the IPv6 header. May point p_packet to a new packet data array,
// which means zero is a valid return value. Sets p_packet to NULL on error.
// See RFC2460
uint32_t ipv6Parse(uint32_t pos, struct pcap_pkthdr *header,
                    uint8_t ** p_packet, ipInfo * ip) {

    // For convenience and code consistency, dereference the packet **.
    uint8_t * packet = *p_packet;

    // In case the IP packet is a fragment.
    ip_fragment * frag = NULL;
    uint32_t header_len = 0;

    if (header->len < (pos + 40)) {
        fprintf(stderr, "Truncated Packet(ipv6)\n");
        *p_packet=NULL; return 0;
    }
    ip->length = (packet[pos+4] << 8) + packet[pos+5];
    IPv6_MOVE(ip->src, packet + pos + 8);
    IPv6_MOVE(ip->dst, packet + pos + 24);

    // Jumbo grams will have a length of zero. We'll choose to ignore those,
    // and any other zero length packets.
    if (ip->length == 0) {
        fprintf(stderr, "Zero Length IP packet, possible Jumbo Payload.\n");
        *p_packet=NULL; return 0;
    }

    uint8_t next_hdr = packet[pos+6];
    //VERBOSE(print_packet(header->len, packet, pos, pos+40, 4);)
    fprintf(stderr, "IPv6 src: %s, ", iptostr(&ip->src));
    fprintf(stderr, "IPv6 dst: %s\n", iptostr(&ip->dst));
    pos += 40;
   
    // We pretty much have no choice but to parse all extended sections,
    // since there is nothing to tell where the actual data is.
    uint8_t done = 0;
    while (done == 0) {
        fprintf(stderr, "IPv6, next header: %u\n", next_hdr);
        switch (next_hdr) {
            // Handle hop-by-hop, dest, and routing options.
            // Yay for consistent layouts.
            case IPPROTO_HOPOPTS:
            case IPPROTO_DSTOPTS:
            case IPPROTO_ROUTING:
                if (header->len < (pos + 16)) {
                    fprintf(stderr, "Truncated Packet(ipv6)\n");
                    *p_packet = NULL; return 0;
                }
                next_hdr = packet[pos];
                // The headers are 16 bytes longer.
                header_len += 16;
                pos += packet[pos+1] + 1;
                break;
            case 51: // Authentication Header. See RFC4302
                if (header->len < (pos + 2)) {
                    fprintf(stderr, "Truncated Packet(ipv6)\n");
                    *p_packet = NULL; return 0;
                } 
                next_hdr = packet[pos];
                header_len += (packet[pos+1] + 2) * 4;
                pos += (packet[pos+1] + 2) * 4;
                if (header->len < pos) {
                    fprintf(stderr, "Truncated Packet(ipv6)\n");
                    *p_packet = NULL; return 0;
                } 
                break;
            case 50: // ESP Protocol. See RFC4303.
                // We don't support ESP.
                fprintf(stderr, "Unsupported protocol: IPv6 ESP.\n");
                if (frag != NULL) free(frag);
                *p_packet = NULL; return 0;
            case 135: // IPv6 Mobility See RFC 6275
                if (header->len < (pos + 2)) {
                    fprintf(stderr, "Truncated Packet(ipv6)\n");
                    *p_packet = NULL; return 0;
                }  
                next_hdr = packet[pos];
                header_len += packet[pos+1] * 8;
                pos += packet[pos+1] * 8;
                if (header->len < pos) {
                    fprintf(stderr, "Truncated Packet(ipv6)\n");
                    *p_packet = NULL; return 0;
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
                done = 1; 
                break;
            default:
                fprintf(stderr, "Unsupported IPv6 proto(%u).\n", next_hdr);
                *p_packet = NULL; return 0;
        }
    }

    // check for int overflow
    if (header_len > ip->length) {
      fprintf(stderr, "Malformed packet(ipv6)\n");
      *p_packet = NULL;
      return 0;
    }

    ip->proto = next_hdr;
    ip->length = ip->length - header_len;

    // Handle fragments.
    if (frag != NULL) {
        frag->src = ip->src;
        frag->dst = ip->dst;
        frag->end = frag->start + ip->length;
        frag->next = frag->child = NULL;
        frag->data = (uint8_t*) malloc(sizeof(uint8_t) * ip->length);
        fprintf(stderr, "IPv6 fragment. offset: %d, m:%u\n", frag->start,
                                                            frag->islast);
        memcpy(frag->data, packet+pos, ip->length);
        // Add the fragment to the list.
        // If this completed the packet, it is returned.
        frag = ip_frag_add(frag, ipFragmentHead); 
        if (frag != NULL) {
            header->len = ip->length = frag->end - frag->start;
            *p_packet = frag->data;
            free(frag);
            return 0;
        }
        // Signals that there is no more work to do on this packet.
        *p_packet = NULL;
        return 0;
    } else {
        return pos;
    }

}

/*********************************************************** INTERFACE ***************************************************************/

void logInterface(string strInterface, string strLog, string strTime)
{
    int time;
    if (strTime.compare(""))
    {
        time = 60; // Default value
    }
    else
    {
        // Check for valid time
        if (strTime.find_first_not_of("0123456789") == string::npos)
        {
            time = stoi(strTime, NULL, 10);
        }
        else
        {
            ERR_RET("Time must be number in seconds!");
        }
    }

    pcap_t *pcapInterface;
    //if ((pcapInterface = pcap_open_live(strInterface.c_str(), 1, 1000, 1000))
}

/*********************************************************** PCAP FILE ***************************************************************/

// This method is called in pcap_loop for each packet
void pcapHandler(unsigned char *useless, const struct pcap_pkthdr *pkthdr, const uint8_t* origPacket)
{
    LOGGING(cerr << "Handling packet " << packetCount++);

    int pos;
    uint16_t ethType;
    ipInfo ip;

    // The way we handle IP fragments means we may have to replace
    // the original data and correct the header info, so a const won't work.
    uint8_t* packet = (uint8_t*) origPacket;
    struct pcap_pkthdr header;
    header.ts = pkthdr->ts;
    header.caplen = pkthdr->caplen;
    header.len = pkthdr->len;
    
    // Parse the ethernet frame. Errors are typically handled in the parser
    // functions. The functions generally return 0 on error.
    pos = ethParse(&header, packet, &ethType, datalink);
    if (pos == 0) 
        return;

    // MPLS parsing is simple, but leaves us to guess the next protocol.
    // We make our guess in the MPLS parser, and set the ethtype accordingly.
    if (ethType == 0x8847) {
        pos = mplsParse(pos, &header, packet, &ethType);
    }  

    // IP v4 and v6 parsing. These may replace the packet byte array with 
    // one from reconstructed packet fragments. Zero is a reasonable return
    // value, so they set the packet pointer to NULL on failure.
    if (ethType == 0x0800) {
        pos = ipv4Parse(pos, &header, &packet, &ip);
    } else if (ethType == 0x86DD) {
        pos = ipv6Parse(pos, &header, &packet, &ip);
    } else {
        fprintf(stderr, "Unsupported EtherType: %04x\n", ethType);
        return;
    }
    if (packet == NULL) return;

        // Transport layer parsing. 
    if (ip.proto == 17) {
        // Parse the udp and this single bit of DNS, and output it.
        dns_info dns;
        transport_info udp;
        pos = udpParse(pos, &header, packet, &udp);
        if ( pos == 0 ) return;
        // Only do deduplication if DEDUPS > 0.
        // if (conf->DEDUPS != 0 ) {
        //     if (dedup(pos, &header, packet, &ip, &udp, conf) == 1) {
        //         // A duplicate packet.
        //         return;
        //     }
        // }

        pos = dns_parse(pos, &header, packet, &dns, !FORCE);

        printSummary(&ip, &udp, &dns, &header);
    } else if (ip.proto == 6) {
        // Hand the tcp packet over for later reconstruction.
        //tcp_parse(pos, &header, packet, &ip, conf); 
    } else {
        fprintf(stderr, "Unsupported Transport Protocol(%d)\n", ip.proto);
        return;
    }
   
    if (packet != origPacket) {
        // Free data from artificially constructed packets.
        free(packet);
    }

    // Expire tcp sessions, and output them if possible.
    fprintf(stderr, "Expiring TCP.\n");
    //tcp_expire(conf, &header.ts);
}

void logFile(string strFile, string strLog)
{
    LOGGING(cerr << "Logging from file");

    char errBuf[PCAP_ERRBUF_SIZE];
    pcap_t *pcapFile;
    string str_filter = "udp port 53";
    struct bpf_program fp; /* compiled filter program (expression) */
    bpf_u_int32 mask;      /* subnet mask */
    bpf_u_int32 net;       /* ip */

    if ((pcapFile = pcap_open_offline(strFile.c_str(), errBuf)) == NULL)
    {
        ERR_RET("Unable to open pcap file.");
    }

    datalink = pcap_datalink(pcapFile);

    int ret2 = pcap_compile(pcapFile, &fp, str_filter.c_str(), 0, 0xffffffff); // create the filter
    int ret3 = pcap_setfilter(pcapFile, &fp);                                  // attach the filter to the handle

    if (pcap_loop(pcapFile, -1, pcapHandler, NULL) < 0)
    {
        ERR_RET("Error occured when reading pcap file.");
    }
}

/*********************************************************** MAIN ***************************************************************/

// Main function
int main(int argc, char const *argv[])
{
    // Handle SIUSR1
    signal(SIGUSR1, handleSig);

    // Parse arguments
    raw_parameters params = parseArg(argc, argv);

    if (params.interface.compare(""))
    {
        logInterface(params.interface, params.syslogServer, params.time);
    }

    if (params.file.compare(""))
    {
        logFile(params.file, params.syslogServer);
    }

    return 0;
}
