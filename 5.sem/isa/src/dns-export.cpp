#include <iostream>		// No comment needed...
#include <unistd.h>		// Getopt
#include <signal.h>     // Signal shandling
#include <pcap.h>		// No comment needed...

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



}

/*********************************************************** PCAP FILE ***************************************************************/

void logFile(string strFile, string strLog)
{
	char errBuf[PCAP_ERRBUF_SIZE];
	pcap_t* pcapFile;
	if ((pcapFile = pcap_open_offline(strFile.c_str(), errBuf)) == NULL)
		ERR_RET("Unable to open pcap file. Terminating.");

	
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
