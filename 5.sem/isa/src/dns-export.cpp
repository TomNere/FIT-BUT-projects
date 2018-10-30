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
uint32_t packetCount = 0;
uint32_t datalink;



/******************************************************** CLASSES ************************************************************/

class DnsRecord
{
	string domain;
	uint32_t rrType;
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

        DnsRecord(string d, uint32_t rt, string ra, uint32_t c)
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
rawParameters parseArg(int argc, char const *argv[])
{
    rawParameters params;
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
void pcapHandler(unsigned char* useless, const struct pcap_pkthdr* origHeader, const uint8_t* origPacket)
{
    LOGGING("Handling packet " << packetCount++);

    uint32_t currentPos;
    uint16_t ethType;
    ipInfo ip;

    uint8_t* packet = (uint8_t*) origPacket;    // Const is useless

    struct pcap_pkthdr header;
    header.ts = origHeader->ts;
    header.caplen = origHeader->caplen;
    header.len = origHeader->len;
    
    // Parse ethernet frame
    currentPos = ethParse(&header, packet, &ethType, datalink);
    if (currentPos == 0)
    {
        LOGGING("Truncated packet found when ethernet header parsing. Skipping packet");
        return;
    }

    // MPLS parsing is simple, but leaves us to guess the next protocol.
    // We make our guess in the MPLS parser, and set the ethtype accordingly.

    if (ethType == MPLS_UNI)
    {
        currentPos = mplsParse(&header, packet, &ethType, currentPos);

        if (currentPos == 0)
        {
            LOGGING("Truncated packet found when mpls parsing. Skipping packet");
            return;
        }
    }  

    // IP v4 and v6 parsing. These may replace the packet byte array with 
    // one from reconstructed packet fragments. Zero is a reasonable return
    // value, so they set the packet pointer to NULL on failure.
    if (ethType == IPv4Prot)
    {
        currentPos = ipv4Parse(currentPos, &header, packet, &ip);
    }
    else if (ethType == 0x86DD)
    {
        currentPos = ipv6Parse(currentPos, &header, packet, &ip);
    }
    else
    {
        LOGGING("Unsupported EtherType: " << ethType);
        return;
    }

    // Fragmented or truncated packet
    if (currentPos == 0)
    {
        return;
    }

    // Transport layer parsing
    if (ip.proto == UDP)
    {
        // Parse the udp and this single bit of DNS, and output it
        transportInfo udp;
        currentPos = udpParse(currentPos, &header, packet, &udp);

        if (currentPos == 0)
        {
            return;
        }

        dnsInfo dns;
        currentPos = dnsParse(currentPos, &header, packet, &dns, !FORCE);
        printSummary(&ip, &udp, &dns, &header);
    }
    else if (ip.proto == TCP)
    {
        // Hand the tcp packet over for later reconstruction.
        // tcp_parse(currentPos, &header, packet, &ip, conf); 
    }
    else
    {
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

    uint32_t ret2 = pcap_compile(pcapFile, &fp, str_filter.c_str(), 0, 0xffffffff); // create the filter
    uint32_t ret3 = pcap_setfilter(pcapFile, &fp);                                  // attach the filter to the handle

    if (pcap_loop(pcapFile, -1, pcapHandler, NULL) < 0)
    {
        ERR_RET("Error occured when reading pcap file.");
    }
}

/*********************************************************** MAIN ***************************************************************/

// Main function
int main(int argc, char const *argv[])
{
    // Handle SIUGSR1
    signal(SIGUSR1, handleSig);

    // Parse arguments
    rawParameters params = parseArg(argc, argv);

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
void printSummary(ipInfo * ip, transportInfo * trns, dnsInfo * dns,
                   struct pcap_pkthdr * header) {
    char proto;

    uint32_t dnslength;
    dnsQuestion *qnext;

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
            rrParserContainer * parser; 
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
    //     printRRSection(dns->nameServers, "$");
    // if (conf->AD_ENABLED) 
    //     printRRSection(dns->additional, "+");
    //printf("%c%s\n", conf->SEP, conf->RECORD_SEP);
    
    dnsQuestion_free(dns->queries);
    dnsRR_free(dns->answers);
    dnsRR_free(dns->nameServers);
    dnsRR_free(dns->additional);
    fflush(stdout); 
    fflush(stderr);
}

