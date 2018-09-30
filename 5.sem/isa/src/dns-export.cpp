#include <iostream>		// no comment needed...
#include <unistd.h>		// getopt
#include <signal.h>     // Signal shandling

using namespace std;

/*************************** CONST *******************************************************************************************/

#define ERR_RET(message) cerr << message << endl; exit(EXIT_FAILURE);

const string help = "Invalid parameters!\n\n"
                    "Usage: dns-export [-r file.pcap] [-i interface] [-s syslog-server] [-t seconds] \n";

/*****************************************************************************************************************************/

// Structure representing parameters of running program in raw string format
struct raw_parameters
{
	string file;
	string interface;
	string syslogServer;
	string time;
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
void parseArg(int argc, char const *argv[], struct raw_parameters params)
{
	int option;

	while ((option = getopt(argc, (char**)argv, "r:i:s:t:")) != -1) {
		switch (option) {
		case 'r':
			if (params.file.compare("")) {
				ERR_RET(help);
			}
			params.file = string(optarg);
			break;
		case 'i':
			if (params.interface.compare("")) {
				ERR_RET(help);
			}
			params.interface = string(optarg);
			break;
		case 's':
			if (params.syslogServer.compare("")) {
				ERR_RET(help);
			}
			params.syslogServer = string(optarg);
			break;
		case 't':
			if (params.time.compare("")) {
				ERR_RET(help);
			}
			params.time = string(optarg);
			break;
		default:
			ERR_RET(help);
		}
	}
}
/*********************************************************** MAIN ***************************************************************/

// Main function
int main(int argc, char const *argv[]) {
    // Handle SIUSR1 
    signal(SIGUSR1, handleSig);

	// Parse arguments
	struct raw_parameters params;
	parseArg(argc, argv, params);



	return 0;
}
