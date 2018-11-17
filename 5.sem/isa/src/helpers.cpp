#include <string>       // Strings
#include <iomanip>      // setfill...

using namespace std;

/************************************** Consts and Macros  **************************************/

// Colors for LOGGING
const string yellow("\033[1;33m");
const string reset("\033[0m");

// Macro for error states
#define ERR_RET(message)     \
    cerr << message << " Terminating." << reset << endl; \
    exit(EXIT_FAILURE);

// If 1 logging is active
#define VERBOSE 0

// Macro for logging data when debbuging
#define LOGGING(message) \
    if (VERBOSE == 1)    \
    {                    \
        cerr << yellow << "LOG: " <<   \
        message << reset << endl;      \
    }

/************************************ Statics from cpp-base64 ***********************************/

static const string base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "0123456789+/";

// Only static helper methods
class Helpers
{
    public:
        // Symbolize char as hex number
        static string ToHex(uint8_t ch)
        {
            stringstream ss;
            ss << "0x" << setfill('0') << setw(2) << hex << ch;
            return ss.str(); 
        }

        // Encode byte array as base64
        // This code come from https://github.com/ReneNyffenegger/cpp-base64
        // zlib License - we can use this code
        // Code by René Nyffenegger was slightly modified to match our coding style

        static string Base64Encode(unsigned char const* bytesToEncode, unsigned int inLen)
        {
            string ret;
            int i = 0;
            int j = 0;
            unsigned char charArray3[3];
            unsigned char charArray4[4];

            while (inLen--)
            {
                charArray3[i++] = *(bytesToEncode++);
                if (i == 3)
                {
                    charArray4[0] = (charArray3[0] & 0xfc) >> 2;
                    charArray4[1] = ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
                    charArray4[2] = ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);
                    charArray4[3] = charArray3[2] & 0x3f;

                    for(i = 0; (i <4) ; i++)
                      ret += base64Chars[charArray4[i]];
                    i = 0;
                }
            }

            if (i)
            {
                for(j = i; j < 3; j++)
                {
                    charArray3[j] = '\0';
                }

                charArray4[0] = ( charArray3[0] & 0xfc) >> 2;
                charArray4[1] = ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
                charArray4[2] = ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);

                for (j = 0; (j < i + 1); j++)
                {
                    ret += base64Chars[charArray4[j]];
                }

                while((i++ < 3))
                {
                    ret += '=';
                }
            }

            return ret;
        }
};
