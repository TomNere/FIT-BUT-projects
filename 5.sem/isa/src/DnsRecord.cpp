#include <string>   // Strings
#include <sstream>  // stringstream

using namespace std;

// Class holding one statistic info
// Domain name, rr type and rr data
class DnsRecord
{
    /********************************************** Private variables *****************************************/

	string domainName;
	uint16_t rrType;
    string rrName;      // rrType string
	string rrData;
	uint16_t count;

    /************************************************ PUBLIC Methods ******************************************/

    public:

        // Initialize new record
        DnsRecord(string d, uint32_t rrt, string rrn, string rra)
        {
            this->domainName = d;
            this->rrType = rrt;
            this->rrName = rrn;
            this->rrData = rra;
            this->count = 1;
        }

        // Test if this record is equal to given parameters
        bool IsEqual(string d, uint16_t rrt, string rra)
        {
            if (d.compare(this->domainName) || rrt != this->rrType || rra.compare(this->rrData))
            {
                return false;
            }
            
            return true;
        }

        // Simply increment counter
        void AddRecord()
        {
            count++;
        }

        // Return record in one string
        string GetString()
        {
            string str;
            stringstream ss;
            ss << this->domainName << " " << this->rrName << " " << this->rrData << " " << count;
            return ss.str();
        }
};