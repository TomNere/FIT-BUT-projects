#include <iostream>		// No comment needed...
#include <unistd.h>		// Getopt
#include <signal.h>     // Signal shandling
#include <pcap.h>		// No comment needed...
#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <net/ethernet.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

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

// This method is called in pcap_loop for each packet
void pcapHandler(u_char *userData, const struct pcap_pkthdr* pkthdr, const u_char* packet)
{
	const struct ip* ipHeader;
	char sourceIP[INET_ADDRSTRLEN];
    char destIP[INET_ADDRSTRLEN];
	u_int sourcePort, destPort;

	const struct ether_header* ethernetHeader = (struct ether_header*)packet;
    if (ntohs(ethernetHeader->ether_type) == ETHERTYPE_IP) 
	{

        ipHeader = (struct ip*)(packet + sizeof(struct ether_header));
        inet_ntop(AF_INET, &(ipHeader->ip_src), sourceIP, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &(ipHeader->ip_dst), destIP, INET_ADDRSTRLEN);

        
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
