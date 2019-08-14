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

typedef SOCKET * SOCKPTR;

int InitNetwork();
int EndNetwork();

namespace Network
{
    enum AddressFamily
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

    enum SocketType
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

    enum Shutdown
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

    using PORT = std::int16_t;

    class DllExport Socket
    {
    private:
        SOCKET sock;
        AddressFamily af;
        SocketType type;
    public:
        Socket() = delete;
        Socket(const Socket &) = delete;
        Socket(AddressFamily af, SocketType type, std::string address, PORT port);
        ~Socket();

        const SOCKET& GetHandle() const;

        void Connect(std::string address, PORT port) const;

        void Listen() const;

        void Accept() const;

        void Send(std::string text, int flags) const;
        void Send(int length, const char* buffer, int flags) const;

        void SendTo(int length, const char* buffer, int flags, std::string ip, PORT port) const;

        void Receive(std::string& text, int flags);
        void Receive(char* buf, int bufsize, int flags);

        void Select();

        class SocketStream
        {
        protected:
            Socket& parent;
        public:
            SocketStream(Socket& s);
        };
    public:
        static void Select(int nfds, fd_set* rfds, fd_set* wfds, fd_set* efds, const timeval* timeout);
    };


    class DllExport TcpSocket : public Socket
    {
    public:
        TcpSocket(std::string address, PORT port);
        ~TcpSocket();
    };

    class DllExport UdpSocket : public Socket
    {
    public:
        UdpSocket(std::string address, PORT port);
        ~UdpSocket();
    };
}

#endif /* _NETWORK_H */
