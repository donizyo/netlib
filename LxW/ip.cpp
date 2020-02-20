#include "stdafx.h"
#include "network.h"

#include <regex>

using namespace Network;

IP::
IP(const std::string& ip)
    : ip{ ip }
{
}

IP
IP::
Construct(const std::string& ip)
{
    const std::string pattern{ R"(^(?:(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\.){3}(?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])$)" };
    const std::regex rgx_ipv4{ pattern, std::regex_constants::ECMAScript };
    std::smatch match;
    if (std::regex_match(ip, match, rgx_ipv4))
        return IP{ ip };
    throw InvalidIPException{ ip };
}

static inline std::string FormatInvalidIPExceptionMessage(const std::string& ip)
{
    std::stringstream ss;
    ss << "Invalid IP: '" << ip << "'!";
    return ss.str();
}

InvalidIPException::
InvalidIPException(const std::string& ip)
    : std::runtime_error(FormatInvalidIPExceptionMessage(ip))
{
}
