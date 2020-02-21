#include "stdafx.h"
#include "network.h"

using namespace Network;

SocketAddress::
SocketAddress(const AddressFamily af, const IP& ip, const PORT port)
    : name{ 0 }
    , family{ af }
    , ip{ ip }
    , port{ port }
{
    switch (inet_pton(static_cast<int>(af), ip.data().c_str(), &(this->name.sin_addr)))
    {
    case 1:
        // success
        break;
    case 0:
    {
        std::ostringstream ss;
        ss << "Invalid parameter 'ip': "
            << ip.data();
        std::string msg{ ss.str() };
#ifdef _DEBUG
        std::cerr << msg
            << std::endl;
#endif
        throw std::invalid_argument(msg);
    }
    case -1:
        throw std::runtime_error("Unexpected exception occurred upon invoking 'inet_pton'!");
    }
    this->name.sin_family = static_cast<int>(af);
    this->name.sin_port = htons(port);
}

const sockaddr_in&
SocketAddress::
GetName() const
{
    return this->name;
}

const AddressFamily&
SocketAddress::
GetFamily() const
{
    return this->family;
}

const IP&
SocketAddress::
GetIP() const
{
    return this->ip;
}

const PORT
SocketAddress::
GetPort() const
{
    return this->port;
}
