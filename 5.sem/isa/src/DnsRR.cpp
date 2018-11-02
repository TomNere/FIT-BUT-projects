
#include <stdint.h> // Getopt
#include <string>

using namespace std;

// Holds the information for a dns resource record.
class DnsRR
{
    // parsers

    public:
        string name;
        uint16_t type;
        uint16_t cls;
        string rrName;
        uint16_t ttl;
        uint16_t rdlength;
        uint16_t dataLen;
        string data;

        void ParseRR()
        {
            
        }
};