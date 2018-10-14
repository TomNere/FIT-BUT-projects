#include <iostream> // No comment needed...
#include <unistd.h> // Getopt
#include <signal.h> // Signal shandling
#include <pcap.h>   // No comment needed...
#include <arpa/inet.h>
#include <string.h>
#include <list>
#include <iterator>
#include <sstream>
#include "parsers.cpp"

using namespace std;
int packetCount = 0;
int datalink;



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
void pcapHandler(unsigned char *useless, const struct pcap_pkthdr *orig_header, const uint8_t* orig_packet)
{
    LOGGING("Handling packet " << packetCount++);

    int pos;
    uint16_t eth_type;
    ip_info ip;

    uint8_t* packet = (uint8_t*) orig_packet;    // Const is useless

    struct pcap_pkthdr header;
    header.ts = orig_header->ts;
    header.caplen = orig_header->caplen;
    header.len = orig_header->len;
    
    // Parse the ethernet frame
    pos = ethParse(&header, packet, &eth_type, datalink);
    if (pos == 0)
    {
        LOGGING("Truncated packet found when eth parsing. Skipping packet");
        return;
    }

    // MPLS parsing is simple, but leaves us to guess the next protocol.
    // We make our guess in the MPLS parser, and set the ethtype accordingly.

    // 0x8847 is MPLS unicast protocol
    if (eth_type == 0x8847)
    {
        pos = mplsParse(pos, &header, packet, &eth_type);

        if (pos == 0)
        {
            LOGGING("Truncated packet found when mpls parsing. Skipping packet");
            return;
        }
    }  

    // IP v4 and v6 parsing. These may replace the packet byte array with 
    // one from reconstructed packet fragments. Zero is a reasonable return
    // value, so they set the packet pointer to NULL on failure.
    if (eth_type == 0x0800)
    {
        pos = ipv4Parse(pos, &header, packet, &ip);
    }
    else if (eth_type == 0x86DD)
    {
        pos = ipv6Parse(pos, &header, packet, &ip);
    }
    else
    {
        LOGGING("Unsupported EtherType: " << eth_type);
        return;
    }

    if (packet == NULL)
    {
        return;
    }

    // Transport layer parsing
    if (ip.proto == 17)
    {
        // Parse the udp and this single bit of DNS, and output it
        dns_info dns;
        transport_info udp;
        pos = udpParse(pos, &header, packet, &udp);

        if (pos == 0)
        {
            return;
        }

        pos = dnsParse(pos, &header, packet, &dns, !FORCE);
        printSummary(&ip, &udp, &dns, &header);
    }
    else if (ip.proto == 6)
    {
        // Hand the tcp packet over for later reconstruction.
        // tcp_parse(pos, &header, packet, &ip, conf); 
    }
    else
    {
        fprintf(stderr, "Unsupported Transport Protocol(%d)\n", ip.proto);
        return;
    }
   
    if (packet != orig_packet) {
        // Free data from artificially constructed packets.
        free(packet);
    }

    // Expire tcp sessions, and output them if possible.
    fprintf(stderr, "Expiring TCP.\n");
    //tcp_expire(conf, &header.ts);
    cout << "//////////////////////////////////////////////\n\n";
}

void logFile(string strFile, string strLog)
{
    LOGGING("Logging from file");

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







// Output the DNS data.
void printSummary(ip_info * ip, transport_info * trns, dns_info * dns,
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
    printRRSection(dns->answers, "!");
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

