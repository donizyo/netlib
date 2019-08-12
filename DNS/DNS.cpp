// DNS.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

namespace DNS
{
    const static int PORT_DNS = 53;

    enum RecordType
    {
        T_A = 1,
        T_NS = 2,
        T_CNAME = 5,
        T_SOA = 6,
        T_WKS = 11,
        T_PTR = 12,
        T_HINFO = 13,
        T_MINFO = 14,
        T_MX = 15,
        T_TXT = 16,
        T_AAAA = 28,
    };

#pragma pack(push, 1)
    struct DNSHeader
    {
        USHORT id;

        UCHAR rd : 1;
        UCHAR tc : 1;
        UCHAR aa : 1;
        UCHAR opcode : 4;
        UCHAR qr : 1;

        UCHAR rcode : 4;
        UCHAR z : 3;
        UCHAR ra : 1;

        USHORT qdcount;
        USHORT ancount;
        USHORT nscount;
        USHORT arcount;
    };

    struct DNSQuestion
    {
        USHORT qtype;
        USHORT qclass;
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
#define rLetDigHyp  rLetter rDigit "\-"
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
