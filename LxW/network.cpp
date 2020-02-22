#include "stdafx.h"

#ifndef SOCKETAPI
#include "network.h"

using namespace Network;

constexpr static int RECV_BUFSIZE{ 4096 };

void CloseSocket(_In_ const SOCKET& s, _In_ const Network::Shutdown how);
void HandleError(_In_ const std::string&& func_name, _In_ const std::string&& winapi_func_name);

#if OS == OS_WINDOWS
WSADATA wsaData{ 0 };
std::atomic_bool isInitialized{ false };

int
InitNetwork()
{
    if (isInitialized)
    {
        // WSA data already initialized
        return 1;
    }
    isInitialized = true;

    std::clog << "WSA> Start up ..." << std::endl;

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
    std::clog << "WSA> Clean up ..." << std::endl;

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

SOCKET
NewSocket(_In_ const AddressFamily af, _In_ const SocketType type, _In_ int protocol, _In_ const IP& ip, _In_ const PORT port)
{
#ifdef _DEBUG
    std::cout << "Net> NewSocket("
        << "af=" << static_cast<int>(af) << ", "
        << "type=" << static_cast<int>(type) << ", "
        << "protocol=" << protocol << ", "
        << "addr=" << ip.data() << ", "
        << "port=" << port
        << ") invoked."
        << std::endl;
#endif

    // Windows:
    // If no error occurs, socket returns a descriptor referencing the new socket.
    // Otherwise, a value of INVALID_SOCKET is returned,
    // and a specific error code can be retrieved by calling WSAGetLastError.
    // Linux:
    // On success, a file descriptor for the new socket is returned.
    // On error, -1 is returned, and _errno_ is set appropriately.
    SOCKET s{ socket(static_cast<int>(af), static_cast<int>(type), protocol) };
    if (s == INVALID_SOCKET)
    {
        fprintf(stderr, "ERR @ socket" NEWLINE);
        HandleError("NewSocket", "socket");
        return INVALID_SOCKET;
    }

    SocketAddress sockaddr{ af, ip, port };
    const sockaddr_in& addr_in{ sockaddr.GetName() };

    if (bind(s, reinterpret_cast<const SOCKADDR FAR*>(&addr_in), sizeof(addr_in)) == SOCKET_ERROR)
    {
        fprintf(stderr, "ERR @ bind" NEWLINE);
        goto error;
    }

    return s;

error:
    CloseSocket(s, Network::Shutdown::Send);
    return INVALID_SOCKET;
}

Network::Socket::
Socket(_In_ const AddressFamily af, _In_ const SocketType type, _In_ const std::string& address, _In_ const PORT port)
    : family{ af }
    , type{ type }
    , sock{ NewSocket(af, type, 0, address, port) }
{
}

Network::Socket::
~Socket()
{
    if (sock != INVALID_SOCKET)
    {
        CloseSocket(sock, Network::Shutdown::Both);
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
Connect(_In_ const std::string& address, _In_ const PORT port) const
{
    SocketAddress sockaddr{ this->family, IP{address}, port };
    Connect(sockaddr);
}

void
Network::Socket::
Connect(_In_ const SocketAddress& address) const
{
    sockaddr_in name{ address.GetName() };
    if (connect(sock, reinterpret_cast<const SOCKADDR FAR*>(&name), sizeof(name)) != 0)
    {
        HandleError("Network::Socket::Connect", "connect");
        throw 1;
    }
}

void
Network::Socket::
Disconnect() const
{
    if (shutdown(sock, static_cast<int>(Network::Shutdown::Both)) == SOCKET_ERROR)
    {
        HandleError("Network::Socket::Disconnect", "shutdown");
        throw 1;
    }
}

void
Network::Socket::
Listen() const
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
        HandleError("Network::Socket::Listen", "listen");
        throw 1;
    }
}

void
Network::Socket::
Accept() const
{
    sockaddr_in addr{ 0 };
    int addrlen{ sizeof(addr) };
    SOCKET s = accept(sock, reinterpret_cast<SOCKADDR FAR*>(&addr), &addrlen);
    if (s == INVALID_SOCKET)
    {
        HandleError("Network::Socket::Accept", "accept");
        throw 1;
    }
}

void
Network::Socket::
Send(_In_ const std::string& text, _In_ const int flags) const
{
    Send(text.length(), text.c_str(), flags);
}

void
Network::Socket::
Send(_In_ const int length, _In_opt_ const char* buffer, _In_ const int flags) const
{
    if (buffer == nullptr)
        return;

    int nbytes = send(sock, buffer, length, flags);
    if (nbytes == SOCKET_ERROR)
    {
        HandleError("Network::Socket::Send", "send");
        throw 1;
    }
}

void
Network::Socket::
SendTo(_In_ const int length, _In_opt_ const char* buffer, _In_ const int flags, _In_ const std::string& ip, _In_ const PORT port) const
{
    std::clog << "Net> Network::Socket::SendTo(length=" << length
        << ", buffer=[]"
        << ", flags=" << flags
        << ", ip=" << ip
        << ", port=" << port
        << ")"
        << std::endl;

    if (buffer == nullptr)
        return;

    SocketAddress sockaddr{ this->family, ip, port };
    const sockaddr_in& name{ sockaddr.GetName() };
    int nbytes = sendto(sock, buffer, length, flags, reinterpret_cast<const SOCKADDR FAR*>(&name), sizeof(name));
    if (nbytes == SOCKET_ERROR)
    {
        HandleError("Network::Socket::SendTo", "sendto");
        throw 1;
    }
}

void
Network::Socket::
Receive(_Out_ std::string& text, _In_ const int flags)
{
    char recvbuf[RECV_BUFSIZE];
    memset(recvbuf, 0, sizeof(recvbuf));
    Receive(sizeof(recvbuf), recvbuf, flags);
    text = std::string(recvbuf, strlen(recvbuf));
}

void
Network::Socket::
Receive(_In_ const int bufsize, _Out_writes_bytes_all_(bufsize) char* buf, _In_ const int flags)
{
    if (buf == nullptr)
        throw 1;

    if (recv(sock, buf, bufsize, flags) == SOCKET_ERROR)
    {
        HandleError("Network::Socket::Receive", "recv");
        throw 1;
    }
}

void
Network::Socket::
Receive(_Inout_ std::vector<char>& buff, _In_ const int flags)
{
    Receive(buff.capacity(), buff.data(), flags);
}

void
Network::Socket::
ReceiveFrom(_In_ const int length, _Out_writes_bytes_all_(length) char* buffer, _In_ const int flags, _In_ const IP& ip, _In_ const PORT port) const
{
    sockaddr_in name = { 0 };
    int namelen{ sizeof(name) };
    int nbytes = recvfrom(sock, buffer, length, flags, (SOCKADDR FAR*)&name, &namelen);
    if (nbytes == SOCKET_ERROR)
    {
        HandleError("Network::Socket::ReceiveFrom", "recvfrom");
        throw 1;
    }
}

void
Network::Socket::
ReceiveFrom(_Inout_ std::vector<char>& buff, _In_ const int flags, _In_ const IP& ip, _In_ const PORT port)
{
    ReceiveFrom(buff.capacity(), buff.data(), flags, ip, port);
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
Select(_In_ const int nfds, _Inout_updates_bytes_all_opt_(nfds) fd_set* rfds, _Inout_updates_bytes_all_opt_(nfds) fd_set* wfds, _Inout_updates_bytes_all_opt_(nfds) fd_set* efds, _In_reads_bytes_opt_(nfds) const timeval* timeout)
{
    int retval = select(nfds, rfds, wfds, efds, timeout);
    if (retval == SOCKET_ERROR)
    {
        HandleError("Network::Socket::Select", "select");
        throw 1;
    }
}

Network::Socket::SocketStream::
SocketStream(_In_ Socket& s)
    : parent(s)
{
}

Network::TcpSocket::
TcpSocket(_In_ const std::string& address, _In_ const PORT port)
    : Socket(Network::AddressFamily::IPv4, Network::SocketType::Stream, address, port)
{
}

Network::TcpSocket::
~TcpSocket()
{
}

Network::UdpSocket::
UdpSocket(_In_ const std::string& address, _In_ const PORT port)
    : Socket(Network::AddressFamily::IPv4, Network::SocketType::Datagram, address, port)
{
}

Network::UdpSocket::
~UdpSocket()
{
}

using SocketStream = Network::Socket::SocketStream;
#include <mutex>
class StreamBuffer : public std::basic_streambuf<char>
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

template<class _StreamType Extends(std::ios_base), class _BufferType Extends(std::basic_streambuf<char>)>
class SocketStreamEx
    : public SocketStream
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


Network::Socket::SocketStream
Network::TcpSocket::
GetInputStream() const
{
    SocketInputStream stream(*const_cast<TcpSocket*>(this));

    return stream;
}

Network::Socket::SocketStream
Network::TcpSocket::
GetOutputStream() const
{
    SocketOutputStream stream(*const_cast<TcpSocket*>(this));

    return stream;
}
#endif

void
CloseSocket(_In_ const SOCKET& s, _In_ const Network::Shutdown how)
{
    shutdown(s, static_cast<int>(how));
#if OS == OS_WINDOWS
    closesocket(s);
#elif OS == OS_LINUX
    close(s);
#endif
}

#if OS == OS_WINDOWS
const char* TranslateErrorCode(_In_ const int code)
{
    const char* name = nullptr;
    switch (code)
    {
    case WSAEINTR: name = "WSAEINTR"; break;
    case WSAEBADF: name = "WSAEBADF"; break;
    case WSAEACCES: name = "WSAEACCES"; break;
    case WSAEFAULT: name = "WSAEFAULT"; break;
    case WSAEINVAL: name = "WSAEINVAL"; break;
    case WSAEMFILE: name = "WSAEMFILE"; break;
    case WSAEWOULDBLOCK: name = "WSAEWOULDBLOCK"; break;
    case WSAEINPROGRESS: name = "WSAEINPROGRESS"; break;
    case WSAEALREADY: name = "WSAEALREADY"; break;
    case WSAENOTSOCK: name = "WSAENOTSOCK"; break;
    case WSAEDESTADDRREQ: name = "WSAEDESTADDRREQ"; break;
    case WSAEMSGSIZE: name = "WSAEMSGSIZE"; break;
    case WSAEPROTOTYPE: name = "WSAEPROTOTYPE"; break;
    case WSAENOPROTOOPT: name = "WSAENOPROTOOPT"; break;
    case WSAEPROTONOSUPPORT: name = "WSAEPROTONOSUPPORT"; break;
    case WSAESOCKTNOSUPPORT: name = "WSAESOCKTNOSUPPORT"; break;
    case WSAEOPNOTSUPP: name = "WSAEOPNOTSUPP"; break;
    case WSAEPFNOSUPPORT: name = "WSAEPFNOSUPPORT"; break;
    case WSAEAFNOSUPPORT: name = "WSAEAFNOSUPPORT"; break;
    case WSAEADDRINUSE: name = "WSAEADDRINUSE"; break;
    case WSAEADDRNOTAVAIL: name = "WSAEADDRNOTAVAIL"; break;
    case WSAENETDOWN: name = "WSAENETDOWN"; break;
    case WSAENETUNREACH: name = "WSAENETUNREACH"; break;
    case WSAENETRESET: name = "WSAENETRESET"; break;
    case WSAECONNABORTED: name = "WSAECONNABORTED"; break;
    case WSAECONNRESET: name = "WSAECONNRESET"; break;
    case WSAENOBUFS: name = "WSAENOBUFS"; break;
    case WSAEISCONN: name = "WSAEISCONN"; break;
    case WSAENOTCONN: name = "WSAENOTCONN"; break;
    case WSAESHUTDOWN: name = "WSAESHUTDOWN"; break;
    case WSAETOOMANYREFS: name = "WSAETOOMANYREFS"; break;
    case WSAETIMEDOUT: name = "WSAETIMEDOUT"; break;
    case WSAECONNREFUSED: name = "WSAECONNREFUSED"; break;
    case WSAELOOP: name = "WSAELOOP"; break;
    case WSAENAMETOOLONG: name = "WSAENAMETOOLONG"; break;
    case WSAEHOSTDOWN: name = "WSAEHOSTDOWN"; break;
    case WSAEHOSTUNREACH: name = "WSAEHOSTUNREACH"; break;
    case WSAENOTEMPTY: name = "WSAENOTEMPTY"; break;
    case WSAEPROCLIM: name = "WSAEPROCLIM"; break;
    case WSAEUSERS: name = "WSAEUSERS"; break;
    case WSAEDQUOT: name = "WSAEDQUOT"; break;
    case WSAESTALE: name = "WSAESTALE"; break;
    case WSAEREMOTE: name = "WSAEREMOTE"; break;
    case WSASYSNOTREADY: name = "WSASYSNOTREADY"; break;
    case WSAVERNOTSUPPORTED: name = "WSAVERNOTSUPPORTED"; break;
    case WSANOTINITIALISED: name = "WSANOTINITIALISED"; break;
    case WSAEDISCON: name = "WSAEDISCON"; break;
    case WSAENOMORE: name = "WSAENOMORE"; break;
    case WSAECANCELLED: name = "WSAECANCELLED"; break;
    case WSAEINVALIDPROCTABLE: name = "WSAEINVALIDPROCTABLE"; break;
    case WSAEINVALIDPROVIDER: name = "WSAEINVALIDPROVIDER"; break;
    case WSAEPROVIDERFAILEDINIT: name = "WSAEPROVIDERFAILEDINIT"; break;
    case WSASYSCALLFAILURE: name = "WSASYSCALLFAILURE"; break;
    case WSASERVICE_NOT_FOUND: name = "WSASERVICE_NOT_FOUND"; break;
    case WSATYPE_NOT_FOUND: name = "WSATYPE_NOT_FOUND"; break;
    case WSA_E_NO_MORE: name = "WSA_E_NO_MORE"; break;
    case WSA_E_CANCELLED: name = "WSA_E_CANCELLED"; break;
    case WSAEREFUSED: name = "WSAEREFUSED"; break;
    case WSAHOST_NOT_FOUND: name = "WSAHOST_NOT_FOUND"; break;
    case WSATRY_AGAIN: name = "WSATRY_AGAIN"; break;
    case WSANO_RECOVERY: name = "WSANO_RECOVERY"; break;
    case WSANO_DATA: name = "WSANO_DATA"; break;
    case WSA_QOS_RECEIVERS: name = "WSA_QOS_RECEIVERS"; break;
    case WSA_QOS_SENDERS: name = "WSA_QOS_SENDERS"; break;
    case WSA_QOS_NO_SENDERS: name = "WSA_QOS_NO_SENDERS"; break;
    case WSA_QOS_NO_RECEIVERS: name = "WSA_QOS_NO_RECEIVERS"; break;
    case WSA_QOS_REQUEST_CONFIRMED: name = "WSA_QOS_REQUEST_CONFIRMED"; break;
    case WSA_QOS_ADMISSION_FAILURE: name = "WSA_QOS_ADMISSION_FAILURE"; break;
    case WSA_QOS_POLICY_FAILURE: name = "WSA_QOS_POLICY_FAILURE"; break;
    case WSA_QOS_BAD_STYLE: name = "WSA_QOS_BAD_STYLE"; break;
    case WSA_QOS_BAD_OBJECT: name = "WSA_QOS_BAD_OBJECT"; break;
    case WSA_QOS_TRAFFIC_CTRL_ERROR: name = "WSA_QOS_TRAFFIC_CTRL_ERROR"; break;
    case WSA_QOS_GENERIC_ERROR: name = "WSA_QOS_GENERIC_ERROR"; break;
    case WSA_QOS_ESERVICETYPE: name = "WSA_QOS_ESERVICETYPE"; break;
    case WSA_QOS_EFLOWSPEC: name = "WSA_QOS_EFLOWSPEC"; break;
    case WSA_QOS_EPROVSPECBUF: name = "WSA_QOS_EPROVSPECBUF"; break;
    case WSA_QOS_EFILTERSTYLE: name = "WSA_QOS_EFILTERSTYLE"; break;
    case WSA_QOS_EFILTERTYPE: name = "WSA_QOS_EFILTERTYPE"; break;
    case WSA_QOS_EFILTERCOUNT: name = "WSA_QOS_EFILTERCOUNT"; break;
    case WSA_QOS_EOBJLENGTH: name = "WSA_QOS_EOBJLENGTH"; break;
    case WSA_QOS_EFLOWCOUNT: name = "WSA_QOS_EFLOWCOUNT"; break;
    case WSA_QOS_EUNKOWNPSOBJ: name = "WSA_QOS_EUNKOWNPSOBJ"; break;
    case WSA_QOS_EPOLICYOBJ: name = "WSA_QOS_EPOLICYOBJ"; break;
    case WSA_QOS_EFLOWDESC: name = "WSA_QOS_EFLOWDESC"; break;
    case WSA_QOS_EPSFLOWSPEC: name = "WSA_QOS_EPSFLOWSPEC"; break;
    case WSA_QOS_EPSFILTERSPEC: name = "WSA_QOS_EPSFILTERSPEC"; break;
    case WSA_QOS_ESDMODEOBJ: name = "WSA_QOS_ESDMODEOBJ"; break;
    case WSA_QOS_ESHAPERATEOBJ: name = "WSA_QOS_ESHAPERATEOBJ"; break;
    case WSA_QOS_RESERVED_PETYPE: name = "WSA_QOS_RESERVED_PETYPE"; break;
    case WSA_SECURE_HOST_NOT_FOUND: name = "WSA_SECURE_HOST_NOT_FOUND"; break;
    case WSA_IPSEC_NAME_POLICY_ERROR: name = "WSA_IPSEC_NAME_POLICY_ERROR"; break;
    }
    return name;
}
const char* GetDetailedErrorString(_In_ const std::string& winapi_func_name, _In_ const int code)
{
    const char* result = nullptr;
    if (winapi_func_name == "inet_pton")
    {
        // https://docs.microsoft.com/en-us/windows/win32/api/ws2tcpip/nf-ws2tcpip-inet_pton
        switch (code)
        {
        case WSAEAFNOSUPPORT:
            result = ("The address family specified in the Family parameter is not supported. "
                "This error is returned if the Family parameter specified "
                "was not AF_INET or AF_INET6.");
            break;
        case WSAEFAULT:
            result = ("The pszAddrString or pAddrBuf parameters are NULL "
                "or are not part of the user address space.");
            break;
        }
    }
    else if (winapi_func_name == "socket")
    {
        // https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-socket
        switch (code)
        {
        case WSANOTINITIALISED:
            result = "A successful WSAStartup call must occur before using this function.";
            break;
        case WSAENETDOWN:
            result = "The network subsystem or the associated service provider has failed.";
            break;
        case WSAEAFNOSUPPORT:
            result = "The specified address family is not supported. For example, an application tried to create a socket for the AF_IRDA address family but an infrared adapter and device driver is not installed on the local computer.";
            break;
        case WSAEINPROGRESS:
            result = "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
            break;
        case WSAEMFILE:
            result = "No more socket descriptors are available.";
            break;
        case WSAEINVAL:
            result = "An invalid argument was supplied. This error is returned if the af parameter is set to AF_UNSPEC and the type and protocol parameter are unspecified.";
            break;
        case WSAEINVALIDPROVIDER:
            result = "The service provider returned a version other than 2.2.";
            break;
        case WSAEINVALIDPROCTABLE:
            result = "The service provider returned an invalid or incomplete procedure table to the WSPStartup.";
            break;
        case WSAENOBUFS:
            result = "No buffer space is available. The socket cannot be created.";
            break;
        case WSAEPROTONOSUPPORT:
            result = "The specified protocol is not supported.";
            break;
        case WSAEPROTOTYPE:
            result = "The specified protocol is the wrong type for this socket.";
            break;
        case WSAEPROVIDERFAILEDINIT:
            result = "The service provider failed to initialize. This error is returned if a layered service provider (LSP) or namespace provider was improperly installed or the provider fails to operate correctly.";
            break;
        case WSAESOCKTNOSUPPORT:
            result = "The specified socket type is not supported in this address family.";
            break;
        }
    }
    else if (winapi_func_name == "connect")
    {
        // https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-connect
        switch (code)
        {
        case WSANOTINITIALISED:
            result = "A successful WSAStartup call must occur before using this function.";
            break;
        case WSAENETDOWN:
            result = "The network subsystem has failed.";
            break;
        case WSAEADDRINUSE:
            result = "The socket's local address is already in use and the socket was not marked to allow address reuse with SO_REUSEADDR. This error usually occurs when executing bind, but could be delayed until the connect function if the bind was to a wildcard address (INADDR_ANY or in6addr_any) for the local IP address. A specific address needs to be implicitly bound by the connect function.";
            break;
        case WSAEINTR:
            result = "The blocking Windows Socket 1.1 call was canceled through WSACancelBlockingCall.";
            break;
        case WSAEINPROGRESS:
            result = "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
            break;
        case WSAEALREADY:
            result = "A nonblocking connect call is in progress on the specified socket. ";
            break;
        case WSAEADDRNOTAVAIL:
            result = "The remote address is not a valid address (such as INADDR_ANY or in6addr_any) .";
            break;
        case WSAEAFNOSUPPORT:
            result = "Addresses in the specified family cannot be used with this socket.";
            break;
        case WSAECONNREFUSED:
            result = "The attempt to connect was forcefully rejected.";
            break;
        case WSAEFAULT:
            result = "The sockaddr structure pointed to by the name contains incorrect address format for the associated address family or the namelen parameter is too small. This error is also returned if the sockaddr structure pointed to by the name parameter with a length specified in the namelen parameter is not in a valid part of the user address space.";
            break;
        case WSAEINVAL:
            result = "The parameter s is a listening socket.";
            break;
        case WSAEISCONN:
            result = "The socket is already connected (connection-oriented sockets only).";
            break;
        case WSAENETUNREACH:
            result = "The network cannot be reached from this host at this time.";
            break;
        case WSAEHOSTUNREACH:
            result = "A socket operation was attempted to an unreachable host.";
            break;
        case WSAENOBUFS:
            result = "No buffer space is available. The socket cannot be connected.";
            break;
        case WSAENOTSOCK:
            result = "The descriptor specified in the s parameter is not a socket.";
            break;
        case WSAETIMEDOUT:
            result = "An attempt to connect timed out without establishing a connection.";
            break;
        case WSAEWOULDBLOCK:
            result = "The socket is marked as nonblocking and the connection cannot be completed immediately.";
            break;
        case WSAEACCES:
            result = "An attempt to connect a datagram socket to broadcast address failed because setsockopt option SO_BROADCAST is not enabled.";
            break;
        }
    }
    else if (winapi_func_name == "shutdown")
    {
        // https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-shutdown
        switch (code)
        {
        case WSAECONNABORTED:
            result = "The virtual circuit was terminated due to a time-out or other failure. The application should close the socket as it is no longer usable. This error applies only to a connection-oriented socket.";
            break;
        case WSAECONNRESET:
            result = "The virtual circuit was reset by the remote side executing a hard or abortive close. The application should close the socket as it is no longer usable. This error applies only to a connection-oriented socket.";
            break;
        case WSAEINPROGRESS:
            result = "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
            break;
        case WSAEINVAL:
            result = "The how parameter is not valid, or is not consistent with the socket type. For example, SD_SEND is used with a UNI_RECV socket type.";
            break;
        case WSAENETDOWN:
            result = "The network subsystem has failed.";
            break;
        case WSAENOTCONN:
            result = "The socket is not connected. This error applies only to a connection-oriented socket.";
            break;
        case WSAENOTSOCK:
            result = "The descriptor is not a socket.";
            break;
        case WSANOTINITIALISED:
            result = "A successful WSAStartup call must occur before using this function.";
            break;
        }
    }
    else if (winapi_func_name == "listen")
    {
        // https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-listen
        switch (code)
        {
        case WSANOTINITIALISED:
            result = "A successful WSAStartup call must occur before using this function.";
        case WSAENETDOWN:
            result = "The network subsystem has failed.";
        case WSAEADDRINUSE:
            result = "The socket's local address is already in use and the socket was not marked to allow address reuse with SO_REUSEADDR. This error usually occurs during execution of the bind function, but could be delayed until this function if the bind was to a partially wildcard address (involving ADDR_ANY) and if a specific address needs to be committed at the time of this function.";
        case WSAEINPROGRESS:
            result = "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
        case WSAEINVAL:
            result = "The socket has not been bound with bind.";
        case WSAEISCONN:
            result = "The socket is already connected.";
        case WSAEMFILE:
            result = "No more socket descriptors are available.";
        case WSAENOBUFS:
            result = "No buffer space is available.";
        case WSAENOTSOCK:
            result = "The descriptor is not a socket.";
        case WSAEOPNOTSUPP:
            result = "The referenced socket is not of a type that supports the listen operation.";
        }
    }
    else if (winapi_func_name == "accept")
    {
        // https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-accept
        switch (code)
        {
        case WSANOTINITIALISED:
            result = "A successful WSAStartup call must occur before using this function.";
            break;
        case WSAECONNRESET:
            result = "An incoming connection was indicated, but was subsequently terminated by the remote peer prior to accepting the call.";
            break;
        case WSAEFAULT:
            result = "The addrlen parameter is too small or addr is not a valid part of the user address space.";
            break;
        case WSAEINTR:
            result = "A blocking Windows Sockets 1.1 call was canceled through WSACancelBlockingCall.";
            break;
        case WSAEINVAL:
            result = "The listen function was not invoked prior to accept.";
            break;
        case WSAEINPROGRESS:
            result = "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
            break;
        case WSAEMFILE:
            result = "The queue is nonempty upon entry to accept and there are no descriptors available.";
            break;
        case WSAENETDOWN:
            result = "The network subsystem has failed.";
            break;
        case WSAENOBUFS:
            result = "No buffer space is available.";
            break;
        case WSAENOTSOCK:
            result = "The descriptor is not a socket.";
            break;
        case WSAEOPNOTSUPP:
            result = "The referenced socket is not a type that supports connection-oriented service.";
            break;
        case WSAEWOULDBLOCK:
            result = "The socket is marked as nonblocking and no connections are present to be accepted.";
            break;
        }
    }
    else if (winapi_func_name == "send")
    {
        // https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-send
        switch (code)
        {
        case WSANOTINITIALISED:
            result = "A successful WSAStartup call must occur before using this function.";
            break;
        case WSAENETDOWN:
            result = "The network subsystem has failed.";
            break;
        case WSAEACCES:
            result = "The requested address is a broadcast address, but the appropriate flag was not set. Call setsockopt with the SO_BROADCAST socket option to enable use of the broadcast address.";
            break;
        case WSAEINTR:
            result = "A blocking Windows Sockets 1.1 call was canceled through WSACancelBlockingCall.";
            break;
        case WSAEINPROGRESS:
            result = "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
            break;
        case WSAEFAULT:
            result = "The buf parameter is not completely contained in a valid part of the user address space.";
            break;
        case WSAENETRESET:
            result = "The connection has been broken due to the keep-alive activity detecting a failure while the operation was in progress.";
            break;
        case WSAENOBUFS:
            result = "No buffer space is available.";
            break;
        case WSAENOTCONN:
            result = "The socket is not connected.";
            break;
        case WSAENOTSOCK:
            result = "The descriptor is not a socket.";
            break;
        case WSAEOPNOTSUPP:
            result = "MSG_OOB was specified, but the socket is not stream-style such as type SOCK_STREAM, OOB data is not supported in the communication domain associated with this socket, or the socket is unidirectional and supports only receive operations.";
            break;
        case WSAESHUTDOWN:
            result = "The socket has been shut down; it is not possible to send on a socket after shutdown has been invoked with how set to SD_SEND or SD_BOTH.";
            break;
        case WSAEWOULDBLOCK:
            result = "The socket is marked as nonblocking and the requested operation would block.";
            break;
        case WSAEMSGSIZE:
            result = "The socket is message oriented, and the message is larger than the maximum supported by the underlying transport.";
            break;
        case WSAEHOSTUNREACH:
            result = "The remote host cannot be reached from this host at this time.";
            break;
        case WSAEINVAL:
            result = "The socket has not been bound with bind, or an unknown flag was specified, or MSG_OOB was specified for a socket with SO_OOBINLINE enabled.";
            break;
        case WSAECONNABORTED:
            result = "The virtual circuit was terminated due to a time-out or other failure. The application should close the socket as it is no longer usable.";
            break;
        case WSAECONNRESET:
            result = "The virtual circuit was reset by the remote side executing a hard or abortive close. For UDP sockets, the remote host was unable to deliver a previously sent UDP datagram and responded with a \"Port Unreachable\" ICMP packet. The application should close the socket as it is no longer usable.";
            break;
        case WSAETIMEDOUT:
            result = "The connection has been dropped, because of a network failure or because the system on the other end went down without notice.";
            break;
        }
    }
    else if (winapi_func_name == "sendto")
    {
        // https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-sendto
        switch (code)
        {
        case WSANOTINITIALISED:
            result = "A successful WSAStartup call must occur before using this function.";
            break;
        case WSAENETDOWN:
            result = "The network subsystem has failed.";
            break;
        case WSAEACCES:
            result = "The requested address is a broadcast address, but the appropriate flag was not set. Call setsockopt with the SO_BROADCAST parameter to allow the use of the broadcast address.";
            break;
        case WSAEINVAL:
            result = "An unknown flag was specified, or MSG_OOB was specified for a socket with SO_OOBINLINE enabled.";
            break;
        case WSAEINTR:
            result = "A blocking Windows Sockets 1.1 call was canceled through WSACancelBlockingCall.";
            break;
        case WSAEINPROGRESS:
            result = "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
            break;
        case WSAEFAULT:
            result = "The buf or to parameters are not part of the user address space, or the tolen parameter is too small.";
            break;
        case WSAENETRESET:
            result = "The connection has been broken due to keep-alive activity detecting a failure while the operation was in progress.";
            break;
        case WSAENOBUFS:
            result = "No buffer space is available.";
            break;
        case WSAENOTCONN:
            result = "The socket is not connected (connection-oriented sockets only).";
            break;
        case WSAENOTSOCK:
            result = "The descriptor is not a socket.";
            break;
        case WSAEOPNOTSUPP:
            result = "MSG_OOB was specified, but the socket is not stream-style such as type SOCK_STREAM, OOB data is not supported in the communication domain associated with this socket, or the socket is unidirectional and supports only receive operations.";
            break;
        case WSAESHUTDOWN:
            result = "The socket has been shut down; it is not possible to sendto on a socket after shutdown has been invoked with how set to SD_SEND or SD_BOTH.";
            break;
        case WSAEWOULDBLOCK:
            result = "The socket is marked as nonblocking and the requested operation would block.";
            break;
        case WSAEMSGSIZE:
            result = "The socket is message oriented, and the message is larger than the maximum supported by the underlying transport.";
            break;
        case WSAEHOSTUNREACH:
            result = "The remote host cannot be reached from this host at this time.";
            break;
        case WSAECONNABORTED:
            result = "The virtual circuit was terminated due to a time-out or other failure. The application should close the socket as it is no longer usable.";
            break;
        case WSAECONNRESET:
            result = "The virtual circuit was reset by the remote side executing a hard or abortive close. For UPD sockets, the remote host was unable to deliver a previously sent UDP datagram and responded with a \"Port Unreachable\" ICMP packet. The application should close the socket as it is no longer usable.";
            break;
        case WSAEADDRNOTAVAIL:
            result = "The remote address is not a valid address, for example, ADDR_ANY.";
            break;
        case WSAEAFNOSUPPORT:
            result = "Addresses in the specified family cannot be used with this socket.";
            break;
        case WSAEDESTADDRREQ:
            result = "A destination address is required.";
            break;
        case WSAENETUNREACH:
            result = "The network cannot be reached from this host at this time.";
            break;
        case WSAETIMEDOUT:
            result = "The connection has been dropped, because of a network failure or because the system on the other end went down without notice. ";
            break;
        }
    }
    else if (winapi_func_name == "recv")
    {
        // https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-recv
        switch (code)
        {
        case WSANOTINITIALISED:
            result = "A successful WSAStartup call must occur before using this function.";
            break;
        case WSAENETDOWN:
            result = "The network subsystem has failed.";
            break;
        case WSAEFAULT:
            result = "The buf parameter is not completely contained in a valid part of the user address space.";
            break;
        case WSAENOTCONN:
            result = "The socket is not connected.";
            break;
        case WSAEINTR:
            result = "The (blocking) call was canceled through WSACancelBlockingCall.";
            break;
        case WSAEINPROGRESS:
            result = "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
            break;
        case WSAENETRESET:
            result = "For a connection-oriented socket, this error indicates that the connection has been broken due to keep-alive activity that detected a failure while the operation was in progress. For a datagram socket, this error indicates that the time to live has expired.";
            break;
        case WSAENOTSOCK:
            result = "The descriptor is not a socket.";
            break;
        case WSAEOPNOTSUPP:
            result = "MSG_OOB was specified, but the socket is not stream-style such as type SOCK_STREAM, OOB data is not supported in the communication domain associated with this socket, or the socket is unidirectional and supports only send operations.";
            break;
        case WSAESHUTDOWN:
            result = "The socket has been shut down; it is not possible to receive on a socket after shutdown has been invoked with how set to SD_RECEIVE or SD_BOTH.";
            break;
        case WSAEWOULDBLOCK:
            result = "The socket is marked as nonblocking and the receive operation would block.";
            break;
        case WSAEMSGSIZE:
            result = "The message was too large to fit into the specified buffer and was truncated.";
            break;
        case WSAEINVAL:
            result = "The socket has not been bound with bind, or an unknown flag was specified, or MSG_OOB was specified for a socket with SO_OOBINLINE enabled or (for byte stream sockets only) len was zero or negative.";
            break;
        case WSAECONNABORTED:
            result = "The virtual circuit was terminated due to a time-out or other failure. The application should close the socket as it is no longer usable.";
            break;
        case WSAETIMEDOUT:
            result = "The connection has been dropped because of a network failure or because the peer system failed to respond.";
            break;
        case WSAECONNRESET:
            result = "The virtual circuit was reset by the remote side executing a hard or abortive close. The application should close the socket as it is no longer usable. On a UDP-datagram socket, this error would indicate that a previous send operation resulted in an ICMP \"Port Unreachable\" message.";
            break;
        }
    }
    else if (winapi_func_name == "recvfrom")
    {
        // https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-recvfrom
        switch (code)
        {
        case WSANOTINITIALISED:
            result = "A successful WSAStartup call must occur before using this function.";
            break;
        case WSAENETDOWN:
            result = "The network subsystem has failed.";
            break;
        case WSAEFAULT:
            result = "The buffer pointed to by the buf or from parameters are not in the user address space, or the fromlen parameter is too small to accommodate the source address of the peer address.";
            break;
        case WSAEINTR:
            result = "The (blocking) call was canceled through WSACancelBlockingCall.";
            break;
        case WSAEINPROGRESS:
            result = "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
            break;
        case WSAEINVAL:
            result = "The socket has not been bound with bind, or an unknown flag was specified, or MSG_OOB was specified for a socket with SO_OOBINLINE enabled, or (for byte stream-style sockets only) len was zero or negative.";
            break;
        case WSAEISCONN:
            result = "The socket is connected. This function is not permitted with a connected socket, whether the socket is connection oriented or connectionless.";
            break;
        case WSAENETRESET:
            result = "For a datagram socket, this error indicates that the time to live has expired.";
            break;
        case WSAENOTSOCK:
            result = "The descriptor in the s parameter is not a socket.";
            break;
        case WSAEOPNOTSUPP:
            result = "MSG_OOB was specified, but the socket is not stream-style such as type SOCK_STREAM, OOB data is not supported in the communication domain associated with this socket, or the socket is unidirectional and supports only send operations.";
            break;
        case WSAESHUTDOWN:
            result = "The socket has been shut down; it is not possible to recvfrom on a socket after shutdown has been invoked with how set to SD_RECEIVE or SD_BOTH.";
            break;
        case WSAEWOULDBLOCK:
            result = "The socket is marked as nonblocking and the recvfrom operation would block.";
            break;
        case WSAEMSGSIZE:
            result = "The message was too large to fit into the buffer pointed to by the buf parameter and was truncated.";
            break;
        case WSAETIMEDOUT:
            result = "The connection has been dropped, because of a network failure or because the system on the other end went down without notice.";
            break;
        case WSAECONNRESET:
            result = "The virtual circuit was reset by the remote side executing a hard or abortive close. The application should close the socket; it is no longer usable. On a UDP-datagram socket this error indicates a previous send operation resulted in an ICMP Port Unreachable message. ";
        }
    }

    if (result == nullptr)
        result = "";
    return result;
}

void HandleErrorCode(_In_ const std::string& func_name, _In_ const int code, _In_ const std::string& winapi_func_name)
{
    std::ostringstream ss;
    ss << "ERR @ '"
        << func_name
        << "' ("
        << code
        << " "
        << TranslateErrorCode(code)
        << ") "
        << GetDetailedErrorString(winapi_func_name, code);
#ifdef _DEBUG
    std::cerr << ss.str();
#endif
}

/*
Print the name of buggy function and error code.
@see: https://docs.microsoft.com/zh-cn/windows/win32/winsock/windows-sockets-error-codes-2
*/
void
HandleError(_In_ const std::string&& func_name, _In_ const std::string&& winapi_func_name)
{
    int code = WSAGetLastError();
    HandleErrorCode(func_name, code, winapi_func_name);
}
#elif OS == OS_LINUX
void
HandleError(std::string func_name)
{
    std::stringstream ss;
    ss << "ERR @ " << func_name;
    perror(ss.str().c_str());
}
#endif
