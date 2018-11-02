#include "DnsRR.cpp"
#include <stdint.h>
#include <list>
using namespace std;

// Holds general DNS information.
class DnsData
{
    // parsers
    

    public:
        uint16_t id;
        char qr;
        char AA;
        char TC;
        uint8_t rcode;
        uint8_t opcode;
        uint16_t ancount;
        list<DnsRR> answers;

        void AddAnswer(DnsRR answer)
        {
            this->answers.push_front(answer);
        }

};

