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

    // 4. Messages
    // 4.1. Format
    // +---------------------+
    // | Header              |
    // +---------------------+
    // | Question            | the question for the name server
    // +---------------------+
    // | Answer              | RRs answering the question
    // +---------------------+
    // | Authority           | RRs pointing toward an authority
    // +---------------------+
    // | Additional          | RRs holding additional information
    // +---------------------+
    //
    // 4.1.1 Header section format
    //                                 1  1  1  1  1  1
    //   0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    // |                       ID                      |
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    // |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    // |                   QDCOUNT                     |
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    // |                   ANCOUNT                     |
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    // |                   NSCOUNT                     |
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    // |                   ARCOUNT                     |
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
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
    bool ValidateDomainName(_In_ const std::string& name)
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

    /*
    Split the specified string _s_ with a single-character _delimiter_.
    The labels are put into a deque in the order according to the flag _reverse_.
    */
    const std::deque<std::string> split(_In_ const std::string& s, _In_ const char delimiter, _In_ bool reverse=false)
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

    /*
    Encodes the given domain name.
    Example:
        'www.google.cn' -> '3www6google2cn'
        where the digits in the encoded domain name stands for the length of the label string as followed;
        meanwhile the digits should be integer values instead of char values;
        say, int(3) instead of char('3').
    */
    const std::string EncodeDomainName(_In_ const std::string& domain)
    {
        if (domain.empty())
            return domain;

        const std::deque<std::string> sq = split(domain, '.');
        std::stringstream ss;

        for (auto part : sq)
        {
            int plen = part.length();
            plen &= 0xFFu;
            ss.put(plen); // put a single char, which stands for the length of the label string as followed
            ss << part; // label strign
        }
        return ss.str();
    }

    const std::string DecodeDomainName(_In_ const std::string& domain)
    {
        if (domain.empty())
            return domain;

        std::istringstream iss{ domain };
        std::ostringstream oss;
        for (char c; iss >> c;)
        {
            if (0 <= c && c <= 9)
            {
                std::streamoff pos = iss.tellg();
                int len = (int)c;
                oss << '.'
                    << domain.substr((int)(pos & 0xFFFFFFFFu), len);
                iss.seekg(pos + len);
            }
        }
        std::string res = oss.str();
        if (!res.empty())
            res = res.substr(1);
        return res;
    }

    class Handler
    {
    protected:
        Network::Socket* sp{ nullptr };
    public:
        Handler(_In_ Network::Socket* pSocket)
            : sp{ pSocket }
        {
        }
        Handler() = delete;
        Handler(_In_ const Handler&) = delete;

        ~Handler()
        {
            if (sp)
            {
                delete sp;
                sp = nullptr;
            }
        }


        class Message
        {
        protected:
            Header header = { 0 };
            Question question = { 0 };

            Handler* hdlr;
        public:
            Message()
                : hdlr{ nullptr }
            {
            }

            Message(_In_ const Handler* handler, _In_ const std::string& ip, _In_ const Network::PORT port)
                : hdlr{ const_cast<Handler*>(handler) }
            {
                char buffer[TCP_BUFFER_CAPACITY];
                memset(buffer, 0, sizeof(buffer));
                hdlr->sp->ReceiveFrom(sizeof(buffer), buffer, 0, ip, port);

                memcpy(&header, buffer, sizeof(header));
                NTOHS(header.qdcount);
                NTOHS(header.ancount);
                NTOHS(header.nscount);
                NTOHS(header.arcount);

                memcpy(&question, buffer + sizeof(header), sizeof(question));
            }

            bool IsQuery() const { return header.qr == 0; }
            bool IsResponse() const { return header.qr == 1; }

            const uint8_t GetOpcode() const { return header.opcode; }
            const std::string GetOpcodeString() const
            {
                const static std::string text[] = {
                    "Standard Query",
                    "Inverse Query",
                    "Server Status Request",
                };

                const uint8_t opcode = GetOpcode();
                if (0 <= opcode && opcode < (sizeof(text) / sizeof(std::string)))
                    return text[opcode];
                return "Reserved";
            }

            const uint8_t GetResponseCode() const { return header.rcode; }
            const std::string GetResponseString() const
            {
                const static std::string text[] = {
                    "No error",
                    "Format error",
                    "Server failure",
                    "Name error",
                    "Not implemented",
                    "Refused",
                };

                const uint8_t rcode = GetResponseCode();
                if (0 <= rcode && rcode < (sizeof(text) / sizeof(std::string)))
                    return text[rcode];
                return "Unknown";
            }

            const Header& GetHeader() const { return header; }
            const Question& GetQuestion() const { return question; }
            //virtual std::vector<ResourceRecord> GetAuthoritativeAnswers() const = 0;
            //virtual std::vector<ResourceRecord> GetAdditionalInformation() const = 0;
        };

        void Receive(_In_ const std::string& ip, _Out_ Message& message)
        {
            const static Network::PORT port = 53;
            message = Message(this, ip, port);

            if (message.GetResponseCode() != 0)
            {
                std::ostringstream ss;
                ss << "Invalid DNS response (rcode: "
                    << message.GetResponseCode()
                    << " "
                    << message.GetResponseString()
                    << ")";
                std::cerr << ss.str();
                throw std::runtime_error(ss.str());
            }

            const Header& header = message.GetHeader();

            std::cout << "The response message contains:" << std::endl
                << header.qdcount << " questions;" << std::endl
                << header.ancount << " answers;" << std::endl
                << header.nscount << " authoritative servers;" << std::endl
                << header.arcount << " additional records." << std::endl
                << std::endl;

            const Question& question = message.GetQuestion();


            ResourceRecord* record;
            const size_t rrSize = sizeof(ResourceRecord);
            record = (ResourceRecord*)malloc(rrSize);
            memset(record, 0, rrSize);
        }
    };

    using Message = Handler::Message;

    template<class _SocketType Extends(Network::Socket)>
    class Client : public Handler
    {
    protected:
        std::vector<std::string> forwarders;
    public:
        Client(_In_ const std::string& address, _In_ const Network::PORT port)
            : Handler(new _SocketType(address, port))
        {
        }

        void AddForwarder(_In_ const std::string& ip)
        {
            std::clog << "DNS> Add forward: "
                << ip
                << std::endl;
            forwarders.push_back(ip);
        }

        virtual void Send(_In_ const Header& header) const = 0;
        virtual void Send(_In_ const std::string& str) const = 0;
    };

    class UdpClient : public Client<Network::UdpSocket>
    {
    public:
        UdpClient(_In_ const std::string& address, _In_ const Network::PORT port)
            : Client(address, port)
        {
        }

        void Send(_In_ const Header& header) const
        {
            const static Network::PORT port = 53;
            for (auto ip : forwarders)
            {
                std::cout << "DNS> Try to reach IP - "
                    << ip
                    << std::endl;
                sp->SendTo(sizeof(header), (const char*)&header, 0, ip, port);
            }
        }

        void Send(_In_ const std::string& str) const
        {
            const static Network::PORT port = 53;
            for (auto ip : forwarders)
            {
                std::cout << "DNS> Try to reach IP - "
                    << ip
                    << std::endl;
                sp->SendTo(str.length(), str.c_str(), 0, ip, port);
            }
        }
    };

    class TcpClient : public Client<Network::TcpSocket>
    {
    public:
        void Send(_In_ const Header& header) const
        {
            const static Network::PORT port = 53;
            for (auto ip : forwarders)
            {
                std::cout << "DNS> Try to reach IP - "
                    << ip
                    << std::endl;
                sp->Connect(ip, port);
                sp->Send(sizeof(header), (const char*)&header, 0);
            }
        }

        void Send(_In_ const std::string& str) const
        {
            const static Network::PORT port = 53;
            for (auto ip : forwarders)
            {
                std::cout << "DNS> Try to reach IP - "
                    << ip
                    << std::endl;
                sp->Connect(ip, port);
                sp->Send(str.length(), str.c_str(), 0);
            }
        }
    };

    template<class _SocketType Extends(Network::Socket)>
    class Server : public Handler
    {
    public:
        Server(_In_ const std::string& address, _In_ const Network::PORT port)
            : Handler(new _SocketType(address, port))
        {
        }
    };

    class UdpServer : public Server<Network::UdpSocket>
    {};

    class TcpServer : public Server<Network::TcpSocket>
    {};

    void ResolveDomainName(_In_ const std::string& domain, RecordType type)
    {
        if (domain.empty())
        {
            throw std::invalid_argument("Invalid parameter 'domain': \"(empty string)\"");
        }

        if (!ValidateDomainName(domain))
        {
            throw std::runtime_error("Invalid domain name");
        }

        Header header = { 0 };
        // get process id
        int pid = getpid();
        pid &= 0xFFFFu;
        header.id = htons(pid);
        header.rd = 1;
        header.qdcount = htons(1);

        std::string encodedDomain = EncodeDomainName(domain);

        UdpClient client("127.0.0.1", 0);

        const std::string forwarder{ "80.80.80.80" };

        client.AddForwarder(forwarder);

        client.Send(header);
        client.Send(encodedDomain);

        Message message;
        client.Receive(forwarder, message);
    }
};

using namespace std::chrono_literals;

int main()
{
    std::string hostname;
    std::condition_variable cv;
    std::mutex mtx;

    InitNetwork();

    std::thread producer([&hostname,&cv]()
    {
        std::cout << "Look up domain name: ";
        std::cin >> hostname;
        cv.notify_all();
    });

    std::thread consumer([&mtx,&hostname,&cv]()
    {
        std::unique_lock<std::mutex> lk(mtx);

        while (hostname.empty())
            cv.wait(lk);
        std::cout << "DNS> Handling '"
            << hostname
            << "' ..."
            << std::endl;
        DNS::ResolveDomainName(hostname, DNS::RecordType::RT_A);
    });

    producer.join();
    consumer.join();

    system("pause");
    EndNetwork();

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
