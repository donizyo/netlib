// DNS.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

namespace DNS
{
    const static int PORT_DNS = 53;

    enum RecordType
    {
        // RFC 1035
        T_A = 1, // a host address
        T_NS = 2, // an authoritative name server
        T_MD = 3, // a mail destination (Obsolete - use MX)
        T_MF = 4, // a mail forwarder (Obsolete - use MX)
        T_CNAME = 5, // the canonical name for an alias
        T_SOA = 6, // marks the start of a zone of authority
        T_MB = 7, // a mailbox domain name (EXPERIMENTAL)
        T_MG = 8, // a mail group member (EXPERIMENTAL)
        T_MR = 9, // a mail rename domain name (EXPERIMENTAL)
        T_NULL = 10, // a null RR (EXPERIMENTAL)
        T_WKS = 11, // a well known service description
        T_PTR = 12, // a domain name pointer
        T_HINFO = 13, // host information
        T_MINFO = 14, // mailbox or mail list information
        T_MX = 15, // mail exchange
        T_TXT = 16, // text strings

        T_AAAA = 28,
    };

#pragma pack(push, 1)
    struct DNSHeader
    {
        uint16_t id;

        uint8_t rd : 1;
        uint8_t tc : 1;
        uint8_t aa : 1;
        uint8_t opcode : 4;
        uint8_t qr : 1;

        uint8_t rcode : 4;
        uint8_t z : 3;
        uint8_t ra : 1;

        uint16_t qdcount;
        uint16_t ancount;
        uint16_t nscount;
        uint16_t arcount;
    };

    struct DNSQuestion
    {
        uint16_t qtype;
        uint16_t qclass;
    };

// @see: https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-levels-2-and-4-c4200?view=vs-2019
#pragma warning(disable : 4200)
    struct ResourceRecord
    {
        std::string name;
        uint16_t type;
        uint16_t _class;
        uint32_t ttl;
        uint16_t data_len;
        char rdata[];
    };
#pragma pack(pop)

// <letter> ::= any one of the 52 alphabetic characters A through Z in
// upper case and a through z in lower case
#define rLetter     "a-zA-Z"
#define pLetter     "[" rLetter "]"
// <digit> ::= any one of the ten digits 0 through 9
#define rDigit      "0-9"
#define pDigit      "[" rDigit "]"
// <let-dig> ::= <letter> | <digit>
#define rLetDig     rLetter rDigit
#define pLetDig     "[" rLetDig "]"
// <let-dig-hyp> ::= <let-dig> | "-"
#define rLetDigHyp  rLetter rDigit "\\-"
// <ldh-str> ::= <let-dig-hyp> | <let-dig-hyp> <ldh-str>
#define pLdhStr     "[" rLetDigHyp "]+"
// <label> ::= <letter> [ [ <ldh-str> ] <let-dig> ]
#define pLabel      pLetter "(" "(" pLdhStr ")?" pLetDig ")?"
// <subdomain> ::= <label> | <subdomain> "." <label>
#define pSubdomain  "(" pLabel "." ")*" pLabel
// <domain> ::= <subdomain> | " "
#define pDomain     "(" pSubdomain ")?"

    /*
    Return: true, if matches;
            false, otherwise.
    */
    bool ValidateDomainName(std::string name)
    {
        std::regex rgx((pDomain), std::regex_constants::ECMAScript);
        return std::regex_match(name, rgx);
    }

#undef rLetter
#undef pLetter
#undef rDigit
#undef pDigit
#undef rLetDig
#undef pLetDig
#undef rLetDigHyp
#undef pLdhStr
#undef pLabel
#undef pSubdomain
#undef pDomain

    class DNSClient
    {
    private:
        const Network::Socket& s;
    public:
        DNSClient(std::string address = "127.0.0.1", Network::PORT port = 0)
            : s(Network::TcpSocket(address, port))
        {
        }
    };

    class DNSServer
    {
    private:
        const Network::Socket& s;
    public:
        DNSServer(std::string address = "0.0.0.0", Network::PORT port = PORT_DNS)
            : s(Network::UdpSocket(address, port))
        {
        }
    };
};

int main()
{
    DNS::ResourceRecord rr;
    std::cout << "Hello World!\n"; 
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
