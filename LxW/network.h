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
    };

    class DllExport Socket
    {
    private:
        SOCKET sock{ INVALID_SOCKET };
        const AddressFamily family;
        const SocketType type;
    public:
        Socket() = delete;
        Socket(const Socket &) = delete;
        Socket(_In_ const AddressFamily af, _In_ const SocketType type, _In_ const std::string& address, _In_ const PORT port);
        ~Socket();

        const SOCKET& GetHandle() const;

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

        void ReceiveFrom(_In_ const int length, _Out_writes_bytes_all_(length) char* buffer, _In_ const int flags, _In_ const IP& ip, _In_ const PORT port) const;
        void ReceiveFrom(_Inout_ std::vector<char>& buff, _In_ const int flags, _In_ const IP& ip, _In_ const PORT port);

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
