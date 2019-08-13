// DNS.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

namespace DNS
{
    const static int PORT_DNS = 53;

    /*
    TYPE fields are used in resource records. Note that these types are a
    subset of QTYPEs.
    */
    enum RecordType : uint8_t
    {
        // RFC 1035
        RT_A = 1, // a host address
        RT_NS = 2, // an authoritative name server
        RT_MD = 3, // a mail destination (Obsolete - use MX)
        RT_MF = 4, // a mail forwarder (Obsolete - use MX)
        RT_CNAME = 5, // the canonical name for an alias
        RT_SOA = 6, // marks the start of a zone of authority
        RT_MB = 7, // a mailbox domain name (EXPERIMENTAL)
        RT_MG = 8, // a mail group member (EXPERIMENTAL)
        RT_MR = 9, // a mail rename domain name (EXPERIMENTAL)
        RT_NULL = 10, // a null RR (EXPERIMENTAL)
        RT_WKS = 11, // a well known service description
        RT_PTR = 12, // a domain name pointer
        RT_HINFO = 13, // host information
        RT_MINFO = 14, // mailbox or mail list information
        RT_MX = 15, // mail exchange
        RT_TXT = 16, // text strings

        RT_AAAA = 28,
    };

    /*
    QTYPE fields appear in the question part of a query. QTYPES are a
    superset of TYPEs, hence all TYPEs are valid QTYPEs.
    */
    enum QuestionType : uint8_t
    {
        // RFC 1035
        QT_AXFR = 252, // A request for a transfer of an entire zone
        QT_MAILB = 253, // A request for mailbox-related records (MB, MG or MR)
        QT_MAILA = 254, // A request for mail agent RRs (Obsolete - see MX)
        QT_ALL = 255, // A request for all records
    };

    /*
    CLASS fields appear in resource records.
    */
    enum ClassValue
    {
        CV_IN = 1, // the Internet
        CV_CS = 2, // the CSNET class (Obsolete - used only for examples in some obsolete RFCs)
        CV_CH = 3, // the CHAOS class
        CV_HS = 4, // Hesiod [Dyer 87]
    };

    /*
    QCLASS fields appear in the question section of a query. QCLASS values
    are a superset of CLASS values; every CLASS is a valid QCLASS.
    */
    enum QuestionClassValue
    {
        CV_ANY = 255, // any class
    };

#pragma pack(push, 1)
    struct Header
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

    struct Question
    {
        uint16_t qtype;
        uint16_t qclass;
    };

    struct ResourceRecord
    {
        std::string name;
        uint16_t type;
        uint16_t _class;
        uint32_t ttl;
        uint16_t data_len;
// @see: https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-levels-2-and-4-c4200?view=vs-2019
#pragma warning(disable : 4200)
        char rdata[];
#pragma warning(default : 4200)
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

    const std::deque<std::string> split(const std::string& s, const char delimiter, bool reverse=false)
    {
        std::deque<std::string> labels;
        std::string label;
        std::istringstream labelStream{ s };
        while (std::getline(labelStream, label, delimiter))
        {
            if (reverse)
                labels.push_front(label);
            else
                labels.push_back(label);
        }
        return labels;
    }

    void ResolveDomainName(std::string domain, RecordType type)
    {
        if (domain.empty())
            return;

        std::deque<std::string> results = split(domain, '.', true);
        int idx = 0;
        for (auto part : results)
        {
            std::cout << ++idx << ' ' << part << std::endl;
        }
    }

    class Client
    {
    private:
        const Network::Socket& s;
    public:
        Client(std::string address = "127.0.0.1", Network::PORT port = 0)
            : s(Network::UdpSocket(address, port))
        {
        }
    };

    class Server
    {
    private:
        const Network::Socket& s;
    public:
        Server(std::string address = "0.0.0.0", Network::PORT port = PORT_DNS)
            : s(Network::UdpSocket(address, port))
        {
        }
    };
};

using namespace std::chrono_literals;

int main()
{
    std::string hostname;
    std::condition_variable cv;
    std::mutex mtx;

    std::thread producer([&]()
    {
        std::cout << "Look up domain name: ";
        std::cin >> hostname;
        cv.notify_all();
    });

    std::thread consumer([&]()
    {
        std::unique_lock<std::mutex> lk(mtx);

        while (hostname.empty())
            cv.wait(lk);
        std::cout << "Handling '"
            << hostname
            << "' ..."
            << std::endl;
        DNS::ResolveDomainName(hostname, DNS::RecordType::RT_A);
    });

    producer.join();
    consumer.join();

    system("pause");
    return 0;
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
