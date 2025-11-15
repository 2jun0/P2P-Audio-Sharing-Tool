#ifdef _WIN32
#include <winsock2.h>
#include <mutex>
#include <stdexcept>
#include <string>
#include "winsock_guard.hpp"

void ensureWinsock()
{
    static std::once_flag onceFlag;
    static int initResult = 0;

    std::call_once(onceFlag, []()
                   {
                       WSADATA wsaData;
                       initResult = WSAStartup(MAKEWORD(2, 2), &wsaData); });

    if (initResult != 0)
        throw std::runtime_error("WSAStartup failed: " + std::to_string(initResult));
}
#endif
