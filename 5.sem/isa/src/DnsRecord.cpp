#include <string>
#include <unistd.h> // Getopt
#include <sstream>

using namespace std;

class DnsRecord
{
	string domain;
	uint32_t rrType;
    string rrName;
	string rrAnswer;
	uint count;

    public:

        DnsRecord(string d, uint32_t rrt, string rrn, string rra)
        {
            this->domain = d;
            this->rrType = rrt;
            this->rrName = rrn;
            this->rrAnswer = rra;
            this->count = 1;
        }

        bool IsEqual(string d, uint32_t rrt, string rra)
        {
            if (d.compare(this->domain) || rrt != this->rrType || rra.compare(this->rrAnswer))
            {
                return false;
            }
            
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
            ss << this->domain << " " << this->rrName << " " << this->rrAnswer << " " << count << endl;
            return ss.str();
        }
};