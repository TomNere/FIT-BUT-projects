#include <iostream>		// No comment needed...
#include <unistd.h>		// Getopt
#include <signal.h>     // Signal shandling
#include <pcap.h>		// No comment needed...
#include <arpa/inet.h>
#include "dns-export.h"
#include <string.h>

using namespace std;
int count = 0;
/*************************** CONST *******************************************************************************************/

#define ERR_RET(message) cerr << message << endl; exit(EXIT_FAILURE);

#define IPv4_MOVE(D, P) D.addr.v4.s_addr = *(in_addr_t *)(P); \
                        D.vers = 0x04;
// Move IPv6 addr at pointer P into ip object D, and set it's type.
#define IPv6_MOVE(D, P) memcpy(D.addr.v6.s6_addr, P, 16); D.vers = 0x06;

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

// IP fragment information.
typedef struct ip_fragment {
    uint32_t id;
    ip_addr src;
    ip_addr dst;
    uint32_t start;
    uint32_t end;
    uint8_t * data;
    uint8_t islast; 
    struct ip_fragment * next;
    struct ip_fragment * child; 
} ip_fragment;

// Holds the information for a dns question.
typedef struct dns_question {
    char * name;
    uint16_t type;
    uint16_t cls;
    struct dns_question * next;
} dns_question;

// Holds the information for a dns resource record.
typedef struct dns_rr {
    char * name;
    uint16_t type;
    uint16_t cls;
    const char * rr_name;
    uint16_t ttl;
    uint16_t rdlength;
    uint16_t data_len;
    char * data;
    struct dns_rr * next;
} dns_rr;


// Holds general DNS information.
typedef struct {
    uint16_t id;
    char qr;
    char AA;
    char TC;
    uint8_t rcode;
    uint8_t opcode;
    uint16_t qdcount;
    dns_question * queries;
    uint16_t ancount;
    dns_rr * answers;
    uint16_t nscount;
    dns_rr * name_servers;
    uint16_t arcount;
    dns_rr * additional;
} dns_info;

// Transport information.
typedef struct {
    uint16_t srcport;
    uint16_t dstport;
    // Length of the payload.
    uint16_t length;
    uint8_t transport; 
} transport_info;
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

void print_mac(unsigned char* mac){
	int i;
	for(i=0; i<6; i++){
		if(i!=0) printf(":");
		printf("%X",mac[i]);
	}
}

char* print_url(char data[]){
	int i=0;
	int toread = data[0];
	int start = 0;
	i++;


	while(toread != 0){
		// print the (#) where a "." in the url is
		//printf("(%d)", toread);
		printf(".");

		// print everything bettween the dots
		for(; i<=start+toread; i++)
			printf("%c",data[i]);
		
		// next chunk
		toread = data[i];
		start = i;
		i++;
	}

	// return a char* to the first non-url char
	return &data[i];
}



int sizeofUrl(char data[]){
	int i = 0;
	int toskip = data[0];

	// skip each set of chars until (0) at the end
	while(toskip!=0){
		i += toskip+1;
		toskip = data[i];
	}

	// return the length of the array including the (0) at the end
	return i+1;
}
/*
char* getUrl(char data[]){
	int size = sizeofUrl(data)-2;
	//char *url = malloc(size);
	
	int i=0;
	int toread = data[0];
	int start = 0;
	i++;
	int j = 0;
	while(toread != 0){
		// a "." in the url
		if(start!=0){
			url[j] = ".";
			j++;
		}	
		// print everything bettween the dots
		for(; i<=start+toread; i++){
			url[j] = data[i];
			j++;
		}
		// next chunk
		toread = data[i];
		start = i;
		i++;
	}
}
*/
void printRRType(int i){
	switch(i){
		case 1:
			printf("IPv4 address record");
			break;
		case 15:
			printf("MX mail exchange record");
			break;
		case 18:
			printf("AFS database record");
			break;
		case 28:
			printf("IPv6 address record");
			break;
		default:
			printf("unknown (%d)",i);
	}
}

void print_packet(void *pack){
	char *tab = "   ";
	
	// listening with an eth header	
	packet_desc* pd = (packet_desc*)pack;
	int offset = pd->wifi.len - sizeof(pd->wifi);

	printf("IEEE 802.11 HEADER\n");
	printf("%sversion:%d\n", tab, pd->wifi.version );
	printf("%spad:%d\n", tab, pd->wifi.pad );
	printf("%slength:%d extra:%i \n", tab, pd->wifi.len, offset );
	printf("%sfields present:%8x\n", tab, pd->wifi.present );

	printf("LOGICAL LINK CONTROL HEADER\n");
	
	printf("IP HEADER\n");	
	printf("%ssource:%s\n", tab, inet_ntoa(pd->ip.src) );
	printf("%sdest:%s\n", tab, inet_ntoa(pd->ip.dst) );

	printf("UDP HEADER\n");	
	printf("%ssource port:%d\n", tab, ntohs(pd->udp.sport) );	
	printf("%sdest port:%d\n", tab, ntohs(pd->udp.dport) );	
	
	printf("DNS HEADER\n");
	printf("%sid:%d\n", tab, ntohs(pd->dns.id));
	printf("%sflags:%d\n", tab, ntohs(pd->dns.flags));
	printf("%s# questions:%d\n", tab, ntohs(pd->dns.qdcount));
	printf("%s# answers:%d\n", tab, ntohs(pd->dns.ancount));
	printf("%s# ns:%d\n", tab, ntohs(pd->dns.nscount));
	printf("%s# ar:%d\n", tab, ntohs(pd->dns.arcount));

	printf("RESOURCE RECORDS\n");

	int numRRs = ntohs(pd->dns.qdcount) + ntohs(pd->dns.ancount) + ntohs(pd->dns.nscount) + ntohs(pd->dns.arcount);
	int i;

	numRRs = 0;	
	for(i=0; i<numRRs; i++){
	//	printf("%sRR(%d)\n", tab, i);
		printf("(%d)", sizeofUrl(pd->data)-2); print_url(pd->data); printf("\n");

		// extract variables
		static_RR* RRd = (static_RR*)((void*)pd->data + sizeofUrl(pd->data));
		int type = ntohs(RRd->type);
		int clas = ntohs(RRd->clas);
		int ttl = (uint32_t)ntohl(RRd->ttl);
		int rdlength = ntohs(RRd->rdlength);
		uint8_t* rd = (uint8_t*)(&RRd->rdlength + sizeof(uint16_t));
	
		printf("%stype(%d):",tab,type); printRRType( ntohs(RRd->type) ); printf("\n");
		printf("%sclass:%d TTL:%d RDlength:%d\n", tab, clas, ttl, rdlength);
		if( rdlength != 0 ){
			printf("data:");
			printf("%d.%d.%d.%d",rd[0], rd[1], rd[2], rd[3]  );
			printf("\n");
		}

	}
	
}


/*********************************************************** INTERFACE ***************************************************************/

void logInterface(string strInterface, string strLog, string strTime)
{
	int time;
	if (strTime.compare(""))
    {
		time = 60;			// Default value
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

	pcap_t* pcapInterface;
	//if ((pcapInterface = pcap_open_live(strInterface.c_str(), 1, 1000, 1000))



}

/*********************************************************** PCAP FILE ***************************************************************/



// This method is called in pcap_loop for each packet
void pcapHandler(unsigned char *useless, const struct pcap_pkthdr* pkthdr, const unsigned char* packet)
{
    cerr << "Handling packet "<< count++ << endl;
	/*
	// cast the packet
	packet_desc *pd = (packet_desc*)packet;	

	int time = pkthdr->ts.tv_sec * 1000000.0 + pkthdr->ts.tv_usec;;
	printf("received at %d a packet: %d/%d\n", time, pkthdr->caplen, pkthdr->len);
	
	// only deal with dns packets
	// really should be done with filter
//	if( ntohs(pd->udp.dport)==53 ){
		print_packet(pd);
		printf("\n");
//	}
    */
}


void logFile(string strFile, string strLog)
{
    cerr << "Log file" << endl;
	char errBuf[PCAP_ERRBUF_SIZE];
	pcap_t* pcapFile;
    string str_filter = "udp port 53";
    struct bpf_program fp;			/* compiled filter program (expression) */
    bpf_u_int32 mask;           /* subnet mask */
    bpf_u_int32 net;            /* ip */

	if ((pcapFile = pcap_open_offline(strFile.c_str(), errBuf)) == NULL)
    {
		ERR_RET("Unable to open pcap file. Terminating.");
    }

    int ret2 = pcap_compile(pcapFile, &fp, str_filter.c_str(), 0, 0xffffffff); // create the filter
    int ret3 = pcap_setfilter(pcapFile, &fp); // attach the filter to the handle

	if (pcap_loop(pcapFile, -1, pcapHandler, NULL) < 0)
    {
		ERR_RET("Error occured when reading pcap file. Terminating.");
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
