#include "stdafx.h"

#ifndef SOCKETAPI
#include "network.h"

#define RECV_BUFSIZE    4096

void CloseSocket(SOCKET s);
void HandleError(std::string func_name);

#if OS == OS_WINDOWS
WSADATA wsaData;

int
InitNetwork()
{
    memset(&wsaData, 0, sizeof(wsaData));

    WORD wVersionRequested = MAKEWORD(2, 2);
    int errcode = WSAStartup(wVersionRequested, &wsaData);
    if (errcode != NO_ERROR)
    {
        LPCSTR errmsg = nullptr;
        switch (errcode)
        {
        case WSASYSNOTREADY:
            errmsg = "WSASYSNOTREADY";
            break;
        case WSAVERNOTSUPPORTED:
            errmsg = "WSAVERNOTSUPPORTED";
            break;
        case WSAEINPROGRESS:
            errmsg = "WSAEINPROGRESS";
            break;
        case WSAEPROCLIM:
            errmsg = "WSAEPROCLIM";
            break;
        case WSAEFAULT:
            errmsg = "WSAEFAULT";
            break;
        default:
            errmsg = "";
            break;
        }
        fprintf(stderr, "ERR @ WSAStartup (err: %s %x)" NEWLINE,
            errmsg, errcode);
        return -1;
    }
    return 0;
}

int
EndNetwork()
{
    WSACleanup();
    return 0;
}

/*
@see: https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-ioctlsocket
*/
#   define fdctl ioctlsocket
#elif OS == OS_LINUX
int
InitNetwork()
{
    return 0;
}

int
EndNetwork()
{
    return 0;
}

/*
@see: https://linux.die.net/man/2/fcntl
*/
#   define fdctl fcntl
#endif

void
HandleIPAddress(int af, const char * addr, int port, sockaddr_in * name)
{
    switch (inet_pton(af, addr, &name->sin_addr))
    {
    case 1:
        // success
        break;
    case 0:
        std::cerr << "Invalid IP address ("
            << addr
            << ")!"
            << std::endl;
        throw 1;
    case -1:
        std::cerr << "ERR @ inet_pton"
            << std::endl;
        throw 1;
    }
    name->sin_family = af;
    name->sin_port = htons(port);
}

SOCKET
NewSocket(int af, int type, int protocol, const char * addr, int port)
{
    // Windows:
    // If no error occurs, socket returns a descriptor referencing the new socket.
    // Otherwise, a value of INVALID_SOCKET is returned,
    // and a specific error code can be retrieved by calling WSAGetLastError.
    // Linux:
    // On success, a file descriptor for the new socket is returned.
    // On error, -1 is returned, and _errno_ is set appropriately.
    SOCKET s = socket(af, type, protocol);
    if (s == INVALID_SOCKET)
    {
        fprintf(stderr, "ERR @ socket" NEWLINE);
        return INVALID_SOCKET;
    }

    sockaddr_in addr_in = { 0 };

    if (addr == NULL)
        addr = "127.0.0.1";

    HandleIPAddress(af, addr, port, &addr_in);

    if (bind(s, (SOCKADDR *)&addr_in, sizeof(addr_in)) == SOCKET_ERROR)
    {
        fprintf(stderr, "ERR @ bind" NEWLINE);
        goto error;
    }

    return s;

error:
    CloseSocket(s);
    return INVALID_SOCKET;
}

using namespace Network;

Network::Socket::
Socket(AddressFamily addressFamily, SocketType socketType, std::string address, std::int16_t port)
    : sock(INVALID_SOCKET)
    , af(addressFamily)
    , type(socketType)
{
    sock = NewSocket(static_cast<int>(addressFamily), static_cast<int>(socketType), 0, address.c_str(), port);
}

Network::Socket::
~Socket()
{
    if (sock != INVALID_SOCKET)
    {
        CloseSocket(sock);
        sock = INVALID_SOCKET;
    }
}

const SOCKET&
Network::Socket::
GetHandle() const
{
    return sock;
}

void
Network::Socket::
Connect(std::string address, std::int16_t port)
{
    sockaddr_in name = { 0 };
    HandleIPAddress(af, address.c_str(), port, &name);
    if (connect(sock, (SOCKADDR *)&name, sizeof(name)) != 0)
    {
        HandleError("Network::Socket::Connect");
    }
}

void
Network::Socket::
Listen()
{
    /*
    The maximum length of the queue of pending connections.
    If set to SOMAXCONN, the underlying service provider responsible for
    socket s will set the backlog to a maximum reasonable value. If set to
    SOMAXCONN_HINT(N) (where N is a number), the backlog value will be N,
    adjusted to be within the range (200, 65535). Note that SOMAXCONN_HINT
    can be used to set the backlog to a larger value than possible with SOMAXCONN.
    SOMAXCONN_HINT is only supported by the Microsoft TCP/IP service provider.
    There is no standard provision to obtain the actual backlog value.
    */
    int backlog = 128;
    if (listen(sock, backlog) == SOCKET_ERROR)
    {
        HandleError("Network::Socket::Listen");
    }
}

void
Network::Socket::
Accept()
{
    sockaddr_in addr = { 0 };
    int addrlen;
    SOCKET s = accept(sock, (sockaddr*)(&addr), &addrlen);
    if (s == INVALID_SOCKET)
    {
        HandleError("Network::Socket::Accept");
    }
}

void
Network::Socket::
Send(std::string text, int flags)
{
    int nbytes = send(sock, text.c_str(), text.length(), flags);
    if (nbytes == SOCKET_ERROR)
    {
        HandleError("Network::Socket::Send");
    }
}

void
Network::Socket::
Receive(std::string& text, int flags)
{
    char recvbuf[RECV_BUFSIZE];
    memset(recvbuf, 0, sizeof(recvbuf));
    Receive(recvbuf, sizeof(recvbuf), flags);
    text = std::string(recvbuf, strlen(recvbuf));
}

void
Network::Socket::
Receive(char* buf, int bufsize, int flags)
{
    if (recv(sock, buf, bufsize, flags) == SOCKET_ERROR)
    {
        HandleError("Network::Socket::Receive");
    }
}

void
Network::Socket::
Select()
{
    fd_set master;
    FD_ZERO(&master);
    FD_SET(sock, &master);
    int nfds = 1;
    fd_set* rfds = &master;
    Network::Socket::Select(nfds, rfds, nullptr, nullptr, nullptr);
}

void
Network::Socket::
Select(int nfds, fd_set* rfds, fd_set* wfds, fd_set* efds, const timeval* timeout)
{
    int retval = select(nfds, rfds, wfds, efds, timeout);
    if (retval == SOCKET_ERROR)
    {
        HandleError("Network::Socket::Select");
    }
}

Network::Socket::SocketStream::
SocketStream(Socket& s)
    : parent(s)
{
}

Network::TcpSocket::
TcpSocket(std::string address, std::int16_t port)
    : Socket(Network::AddressFamily::IPv4, Network::SocketType::Stream, address, port)
{
}

Network::UdpSocket::
UdpSocket(std::string address, std::int16_t port)
    : Socket(Network::AddressFamily::IPv4, Network::SocketType::Datagram, address, port)
{
}

using SocketStream = Network::Socket::SocketStream;
class StreamBuffer : std::basic_streambuf<char>
{
private:
    std::mutex mtx;
protected:
    _Override
        void _Lock()
    {
        mtx.lock();
    }

    _Override
        void _Unlock()
    {
        mtx.unlock();
    }
};

class SocketInputBuffer : public StreamBuffer
{
private:
    const Socket& parent;
public:
    SocketInputBuffer(const Socket& s)
        : parent(s)
    {
    }
};

class SocketOutputBuffer : public StreamBuffer
{
private:
    const Socket& parent;
public:
    SocketOutputBuffer(const Socket& s)
        : parent(s)
    {
    }
};

template<class _StreamType, class _BufferType>
class SocketStreamEx
    : SocketStream
    , _StreamType
{
private:
    StreamBuffer* buf;
public:
    SocketStreamEx(Socket& s)
        : SocketStream(s)
        , buf(new _BufferType(s))
        , _StreamType(buf)
    {
    }

    ~SocketStreamEx() { delete buf; }
};

using SocketInputStream = SocketStreamEx<std::basic_istream<char>, SocketInputBuffer>;
using SocketOutputStream = SocketStreamEx<std::basic_ostream<char>, SocketOutputBuffer>;
#endif

void
CloseSocket(SOCKET s)
{
#if OS == OS_WINDOWS
    closesocket(s);
#elif OS == OS_LINUX
    close(s);
#endif
}

void
HandleError(std::string func_name)
{
#if OS == OS_WINDOWS
    auto code = WSAGetLastError();
    std::cerr << "ERR @ " << func_name << " -> "
        << code;
#elif OS == OS_LINUX
    std::stringstream ss;
    ss << "ERR @ " << func_name;
    perror(ss.str().c_str());
#endif
}
