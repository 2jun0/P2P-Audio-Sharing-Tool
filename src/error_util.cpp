#include <sstream>
#include "error_util.hpp"

std::string hresultToString(HRESULT hr)
{
    char buf[512];
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, hr, 0, buf, sizeof(buf), nullptr);
    return "(" + std::to_string(hr) + ") " + std::string(buf);
}

std::string win32ErrorToString(DWORD err)
{
    char buf[512];
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, err, 0, buf, sizeof(buf), nullptr);
    return "(" + std::to_string(err) + ") " + std::string(buf);
}