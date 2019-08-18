#pragma once

#ifndef _NETWORK_H
#define _NETWORK_H

#include "stdafx.h"

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

    class DllExport Socket
    {
    private:
        SOCKET sock;
        AddressFamily af;
        SocketType type;
    public:
        Socket() = delete;
        Socket(const Socket &) = delete;
        Socket(AddressFamily af, SocketType type, std::string address, std::int16_t port);
        ~Socket();

        const SOCKET& GetHandle() const;

        void Connect(std::string address, std::int16_t port);

        void Listen();

        void Accept();

        void Send(std::string text, int flags);

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
        TcpSocket(std::string address, std::int16_t port);
    };

    class DllExport UdpSocket : public Socket
    {
    public:
        UdpSocket(std::string address, std::int16_t port);
    };
}

#endif /* _NETWORK_H */
