#include <iostream>		// No comment needed...
#include <unistd.h>		// Getopt
#include <signal.h>     // Signal shandling
#include <pcap.h>		// No comment needed...
#include <arpa/inet.h>
#include "dns-export.h"

using namespace std;

/*************************** CONST *******************************************************************************************/

#define ERR_RET(message) cerr << message << endl; exit(EXIT_FAILURE);

const string help = "Invalid parameters!\n\n"
                    "Usage: dns-export [-r file.pcap] [-i interface] [-s syslog-server] [-t seconds] \n";

/*****************************************************************************************************************************/

// Structure representing parameters of running program in raw string format
typedef struct raw_parameters
{
	string file;
	string interface;
	string syslogServer;
	string time;
} raw_parameters;

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

	while ((option = getopt(argc, (char**)argv, "r:i:s:t:")) != -1) {
		switch (option) {
		case 'r':
			if (params.file.compare(""))
				ERR_RET(help);

			params.file = string(optarg);
			break;
		case 'i':
			if (params.interface.compare(""))
				ERR_RET(help);

			params.interface = string(optarg);
			break;
		case 's':
			if (params.syslogServer.compare(""))
				ERR_RET(help);

			params.syslogServer = string(optarg);
			break;
		case 't':
			if (params.time.compare(""))
				ERR_RET(help);

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
		time = 60;			// Default value
	else
	{
		// Check for valid time
		if (strTime.find_first_not_of("0123456789") == string::npos)
        	time = stoi(strTime, NULL, 10);
    	else
        	ERR_RET("Time must be number in seconds!");
	}

	pcap_t* pcapInterface;
	//if ((pcapInterface = pcap_open_live(strInterface.c_str(), 1, 1000, 1000))



}

/*********************************************************** PCAP FILE ***************************************************************/

// Parse the ethernet headers, and return the payload position (0 on error).
uint32_t eth_parse(struct pcap_pkthdr* header, uint8_t* packet, uint16_t* ethtype) {

    if (header->len < 14)
        ERR_RET("Truncated Packet(eth)");

    uint32_t pos = 14;	// Header skip
    
    // Skip VLAN tagging
    if (packet[pos] == 0x81 && packet[pos+1] == 0)
		pos = pos + 4;

    *ethtype = (packet[pos] << 8) + packet[pos+1];
    pos = pos + 2;

    // SHOW_RAW(
    //     printf("\neth ");
    //     print_packet(header->len, packet, 0, pos, 18);
    // )
    // VERBOSE(
    //     printf("dstmac: %02x:%02x:%02x:%02x:%02x:%02x, "
    //            "srcmac: %02x:%02x:%02x:%02x:%02x:%02x\n",
    //            eth->dstmac[0],eth->dstmac[1],eth->dstmac[2],
    //            eth->dstmac[3],eth->dstmac[4],eth->dstmac[5],
    //            eth->srcmac[0],eth->srcmac[1],eth->srcmac[2],
    //            eth->srcmac[3],eth->srcmac[4],eth->srcmac[5]);
    // )
    return pos;
}

// This method is called in pcap_loop for each packet
void pcapHandler(uint8_t * args, const struct pcap_pkthdr *orig_header, const uint8_t *orig_packet)
{
	int pos;
    uint16_t ethtype;
    ip_info ip;
    //config* conf = (config *) args;

    // The way we handle IP fragments means we may have to replace
    // the original data and correct the header info, so a const won't work.
    uint8_t * packet = (uint8_t *) orig_packet;
    struct pcap_pkthdr header;
    header.ts = orig_header->ts;
    header.caplen = orig_header->caplen;
    header.len = orig_header->len;
    
    //cerr << "\nPacket %llu.%llu\n", 
      //             (uint64_t)header.ts.tv_sec, 
        //           (uint64_t)header.ts.tv_usec);)
    
    // Parse the ethernet frame. Errors are typically handled in the parser
    // functions. The functions generally return 0 on error.
    pos = eth_parse(&header, packet, &ethtype);
    if (pos == 0) return;

    // MPLS parsing is simple, but leaves us to guess the next protocol.
    // We make our guess in the MPLS parser, and set the ethtype accordingly.
    if (ethtype == 0x8847) {
        pos = mpls_parse(pos, &header, packet, ethtype);
    } 

    // IP v4 and v6 parsing. These may replace the packet byte array with 
    // one from reconstructed packet fragments. Zero is a reasonable return
    // value, so they set the packet pointer to NULL on failure.
    if (eth.ethtype == 0x0800) {
        pos = ipv4_parse(pos, &header, &packet, &ip, conf);
    } else if (eth.ethtype == 0x86DD) {
        pos = ipv6_parse(pos, &header, &packet, &ip, conf);
    } else {
        fprintf(stderr, "Unsupported EtherType: %04x\n", eth.ethtype);
        return;
    }
    if (packet == NULL) return;

    // Transport layer parsing. 
    if (ip.proto == 17) {
        // Parse the udp and this single bit of DNS, and output it.
        dns_info dns;
        transport_info udp;
        pos = udp_parse(pos, &header, packet, &udp, conf);
        if ( pos == 0 ) return;
        // Only do deduplication if DEDUPS > 0.
        if (conf->DEDUPS != 0 ) {
            if (dedup(pos, &header, packet, &ip, &udp, conf) == 1) {
                // A duplicate packet.
                return;
            }
        }
        pos = dns_parse(pos, &header, packet, &dns, conf, !FORCE);
        print_summary(&ip, &udp, &dns, &header, conf);
    } else if (ip.proto == 6) {
        // Hand the tcp packet over for later reconstruction.
        tcp_parse(pos, &header, packet, &ip, conf); 
    } else {
        fprintf(stderr, "Unsupported Transport Protocol(%d)\n", ip.proto);
        return;
    }
   
    if (packet != orig_packet) {
        // Free data from artificially constructed packets.
        free(packet);
    }

    // Expire tcp sessions, and output them if possible.
    DBG(printf("Expiring TCP.\n");)
    tcp_expire(conf, &header.ts);
}

}


void logFile(string strFile, string strLog)
{
	char errBuf[PCAP_ERRBUF_SIZE];
	pcap_t* pcapFile;
	if ((pcapFile = pcap_open_offline(strFile.c_str(), errBuf)) == NULL)
		ERR_RET("Unable to open pcap file. Terminating.");

	if (pcap_loop(pcapFile, 0, pcapHandler, NULL) < 0)
		ERR_RET("Error occured when reading pcap file. Terminating.");

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
