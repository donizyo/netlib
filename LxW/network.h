#pragma once

#ifndef _NETWORK_H
#define _NETWORK_H

#include "stdafx.h"

#include <string>
#include <iostream>

#if OS == OS_WINDOWS
#   define _WINSOCK_DEPRECATED_NO_WARNINGS

#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif

#   include <winsock2.h>
#   include <Ws2tcpip.h>
#   pragma comment(lib, "Ws2_32.lib")

#   include <MSWSock.h>
#   pragma comment(lib, "Mswsock.lib")

#   include <iphlpapi.h>
#   pragma comment(lib, "Iphlpapi.lib")

#   include <Windows.h>


#   include <basetsd.h>

#   define MALLOC(x)       HeapAlloc(GetProcessHeap(), 0, (x))
#   define FREE(x)         HeapFree(GetProcessHeap(), 0, (x))

#   define NEWLINE "\r\n"

#   define DllExport __declspec(dllexport)
#   define DllImport __declspec(dllimport)

#elif OS == OS_LINUX

#   include <stdlib.h>
#   include <stdio.h>
#   include <errno.h>
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <sys/select.h>
#   include <sys/time.h>
#   include <arpa/inet.h>
#   include <poll.h>
#   include <unistd.h>

#   define NEWLINE "\n"

#   define DllExport
#   define DllImport

typedef int SOCKET;
typedef struct sockaddr SOCKADDR;

#   define INVALID_SOCKET (-1)

#endif

#ifndef UDP_BUFFER_CAPACITY
#   define UDP_BUFFER_CAPACITY 512
#endif
#ifndef TCP_BUFFER_CAPACITY
#   define TCP_BUFFER_CAPACITY 65536
#endif

/*
Used in template type list.
i.e.
template <typename T Extends(Number)>
class Vector {};
*/
#define Extends(BaseType)

#define NTOHS(d) (d = ntohs(d))

typedef SOCKET * SOCKPTR;

extern int DllExport InitNetwork();
extern int DllExport EndNetwork();

namespace Network
{
    enum class AddressFamily : int
    {
        /*
        The Internet Protocol version 4 (IPv4) address family.
        */
        IPv4 = AF_INET,
        /*
        The Internet Protocol version 6 (IPv6) address family.
        */
        IPv6 = AF_INET6,
    };

    enum class SocketType : int
    {
        /*
        A socket type that provides sequenced, reliable, two-way,
        connection-based byte streams with an OOB data transmission mechanism.
        This socket type uses the Transmission Control Protocol (TCP)
        for the Internet address family (AF_INET or AF_INET6).
        */
        Stream = SOCK_STREAM,
        /*
        A socket type that supports datagrams, which are connectionless,
        unreliable buffers of a fixed (typically small) maximum length.
        This socket type uses the User Datagram Protocol (UDP) for
        the Internet address family (AF_INET or AF_INET6).
        */
        Datagram = SOCK_DGRAM,
        /*
        A socket type that provides a raw socket that allows an application
        to manipulate the next upper-layer protocol header.
        To manipulate the IPv4 header, the IP_HDRINCL socket option
        must be set on the socket. To manipulate the IPv6 header,
        the IPV6_HDRINCL socket option must be set on the socket.
        */
        Raw = SOCK_RAW,
    };

    // Internet Header Format
    // @see: https://tools.ietf.org/html/rfc791
    //      A summary of the contents of the internet header follows:
    //
    //       0                   1                   2                   3
    //       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    //      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //      |Version|  IHL  |Type of Service|          Total Length         |
    //      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //      |         Identification        |Flags|      Fragment Offset    |
    //      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //      |  Time to Live |    Protocol   |         Header Checksum       |
    //      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //      |                       Source Address                          |
    //      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //      |                    Destination Address                        |
    //      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //      |                    Options                    |    Padding    |
    //      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    struct IPHeader
    {
        // Version
        //      4
        // IHL
        //      Internet header length in 32-bit words.
#if (OS == OS_WINDOWS) /* Little endian is used on Windows */
        std::uint8_t    hl : 4;
        std::uint8_t    v : 4;
#elif (OS == OS_LINUX)
#   if __BYTE_ORDER == __LITTLE_ENDIAN
        std::uint8_t    hl : 4;
        std::uint8_t    v : 4;
#   elif __BYTE_ORDER == __BIG_ENDIAN
        std::uint8_t    v : 4;
        std::uint8_t    hl : 4;
#   endif
#else
#   error "Unsupported operation system!"
#endif
    // Type of Service
        std::uint8_t    tos;
        // Total Length
        //      Length of internet header and data in octets.
        std::uint16_t   len;
        // Identification, Flags, Fragment Offset
        //      Used in fragmentation
        std::uint16_t   id;     // identification
        std::uint16_t   off;    // fragment offset field
        // Time to Live
        //      Time to live in seconds; as this field is decremented at each
        //      machine in which the datagram is processed, the value in this
        //      field should be at least as great as the number of gateways which
        //      this datagram will traverse.
        std::uint8_t    ttl;
        // Protocol
        std::uint8_t    p;
        // Header Checksum
        //      The 16 bit one's complement of the one's complement sum of all 16
        //      bit words in the header. For computing the checksum, the checksum
        //      field should be zero. This checksum may be replaced in the future.
        std::uint16_t   sum;
        // Source Address
        //      The address of the gateway or host that composes the ICMP message.
        //      Unless otherwise noted, this can be any of a gateway's addresses.
        in_addr         src;
        // Destination Address
        //      The address of the gateway or host to which the message should be
        //      sent.
        in_addr         dst;
    };

    enum class Shutdown : int
    {
#if OS == OS_WINDOWS
        Read = SD_RECEIVE,
        Send = SD_SEND,
        Both = SD_BOTH,
#else /*OS_LINUX*/
        Read = SHUT_RD,
        Send = SHUT_WR,
        Both = SHUT_RDWR,
#endif
    };

    class IP
    {
    private:
        const std::string ip;

    public:
        DllExport IP(const std::string& ip);

        DllExport static const IP LocalHost;

        DllExport const std::string& data() const;
    };

    class InvalidIPException
        : public std::runtime_error
    {
    public:
        DllExport explicit InvalidIPException(_In_ const std::string& ip);
    };

    class SocketException
        : public std::runtime_error
    {
    public:
        DllExport explicit SocketException(_In_ const std::string& message);
    };

    using PORT = std::uint16_t; /* 0-65535 */

    class SocketAddress
    {
    private:
        sockaddr_in name;
        const AddressFamily family;
        const IP ip;
        const PORT port;

    public:
        DllExport SocketAddress(_In_ const AddressFamily af, _In_ const IP& ip, _In_ const PORT port);

        DllExport const sockaddr_in& GetName() const;
        DllExport const AddressFamily& GetFamily() const;
        DllExport const IP& GetIP() const;
        DllExport const PORT GetPort() const;

        DllExport static std::string ToString(const sockaddr* sa);
    };

    class DllExport Socket
    {
    private:
        SOCKET sock{ INVALID_SOCKET };
        const AddressFamily family;
        const SocketType type;
    public:
        Socket() = delete;
        Socket(const Socket&) = default;
        Socket(Socket&&) = default;
        Socket(_In_ const AddressFamily af, _In_ const SocketType type, _In_ const IP& address, _In_ const PORT port);
        Socket(_In_ const AddressFamily af, _In_ const SocketType type);
        ~Socket();

        void Connect(_In_ const std::string& address, _In_ const PORT port) const;
        void Connect(_In_ const SocketAddress& address) const;
        void Disconnect() const;

        void Listen() const;

        void Accept() const;

        void Send(_In_ const std::string& text, _In_ const int flags) const;
        void Send(_In_ const int length, _In_opt_ const char* buffer, _In_ const int flags) const;
        void Send(_In_ const std::vector<char>& buffer, _In_ const int flags) const;

        void SendTo(_In_ const int length, _In_opt_ const char* buffer, _In_ const int flags, _In_ const IP& ip, _In_ const PORT port) const;
        void SendTo(_In_ const std::vector<char>& buffer, _In_ const int flags, _In_ const IP& ip, _In_ const PORT port) const;

        void Receive(_In_ const int bufsize, _Out_writes_bytes_all_(bufsize) char* buf, _In_ const int flags);
        void Receive(_Inout_ std::vector<char>& buff, _In_ const int flags);

        void ReceiveFrom(_In_ const int length, _Out_writes_bytes_all_(length) char* buffer, _In_ const int flags,
            _Out_writes_bytes_to_opt_(*fromlen, *fromlen) sockaddr FAR* from, _Inout_opt_ int FAR* fromlen) const;
        void ReceiveFrom(_Inout_ std::vector<char>& buff, _In_ const int flags,
            _Out_writes_bytes_to_opt_(*fromlen, *fromlen) sockaddr FAR* from, _Inout_opt_ int FAR* fromlen);

        void Select();

        class SocketStream
        {
        protected:
            Socket& parent;
        public:
            SocketStream(_In_ Socket& s);
            SocketStream() = delete;
            SocketStream(_In_ const SocketStream&) = default;
        };
    public:
        static void Select(_In_ const int nfds, _Inout_updates_bytes_all_opt_(nfds) fd_set* rfds, _Inout_updates_bytes_all_opt_(nfds) fd_set* wfds, _Inout_updates_bytes_all_opt_(nfds) fd_set* efds, _In_reads_bytes_opt_(nfds) const timeval* timeout);
    };


    class DllExport TcpSocket : public Socket
    {
    public:
        TcpSocket(_In_ const std::string& address, _In_ const PORT port);
        ~TcpSocket();

        Socket::SocketStream GetInputStream() const;
        Socket::SocketStream GetOutputStream() const;
    };

    class DllExport UdpSocket : public Socket
    {
    public:
        UdpSocket(_In_ const std::string& address, _In_ const PORT port);
        ~UdpSocket();
    };
}

#endif /* _NETWORK_H */
