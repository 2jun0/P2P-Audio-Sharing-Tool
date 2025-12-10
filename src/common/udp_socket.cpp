#include "udp_socket.hpp"

#include <stdexcept>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mutex>
#define CLOSESOCK closesocket
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#define CLOSESOCK ::close
#endif

#ifdef _WIN32
// Ensure Winsock is initialized exactly once across all socket instances
static void ensureWinsock()
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

UdpSocket::UdpSocket() : fd_(-1), boundPort_(0) {}

UdpSocket::~UdpSocket()
{
    close();
}

void UdpSocket::openBroadcast(int port)
{
#ifdef _WIN32
    ensureWinsock();
#endif
    fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_ < 0)
    {
        throw std::runtime_error("Failed to create UDP socket");
    }

    int broadcastEnable = 1;
#ifdef _WIN32
    if (setsockopt(fd_, SOL_SOCKET, SO_BROADCAST, (char *)&broadcastEnable, sizeof(broadcastEnable)) < 0)
#else
    if (setsockopt(fd_, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0)
#endif
    {
        CLOSESOCK(fd_);
        fd_ = -1;
        throw std::runtime_error("Failed to set SO_BROADCAST");
    }

    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_port = htons(static_cast<uint16_t>(port));
    local.sin_addr.s_addr = INADDR_ANY;
    if (::bind(fd_, reinterpret_cast<sockaddr *>(&local), sizeof(local)) < 0)
    {
        CLOSESOCK(fd_);
        fd_ = -1;
        throw std::runtime_error("Failed to bind UDP socket");
    }

    boundPort_ = port;
}

void UdpSocket::setRecvTimeout(std::chrono::milliseconds timeout)
{
    if (fd_ < 0)
        return;
#ifdef _WIN32
    DWORD tv = static_cast<DWORD>(timeout.count());
    setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
#else
    timeval tv{};
    tv.tv_sec = static_cast<time_t>(timeout.count() / 1000);
    tv.tv_usec = static_cast<suseconds_t>((timeout.count() % 1000) * 1000);
    setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
#endif
}

bool UdpSocket::sendTo(const std::string &address, int port, const char *data, size_t len)
{
    if (fd_ < 0)
        return false;
    sockaddr_in dst{};
    dst.sin_family = AF_INET;
    dst.sin_port = htons(static_cast<uint16_t>(port));
    dst.sin_addr.s_addr = inet_addr(address.c_str());
    return ::sendto(fd_, data, static_cast<int>(len), 0, reinterpret_cast<sockaddr *>(&dst), sizeof(dst)) >= 0;
}

int UdpSocket::recvFrom(char *buffer, size_t len, std::string &fromAddress, int &outErrno)
{
    if (fd_ < 0)
    {
        outErrno = EBADF;
        return -1;
    }
    sockaddr_in src{};
    socklen_t addrLen = sizeof(src);
    int bytes = ::recvfrom(fd_, buffer, static_cast<int>(len), 0, reinterpret_cast<sockaddr *>(&src), &addrLen);
    if (bytes < 0)
    {
#ifdef _WIN32
        int werr = WSAGetLastError();
        // Windows error code ==> POSIX-like error code
        if (werr == WSAEWOULDBLOCK)
            outErrno = EAGAIN;
        else if (werr == WSAEINTR)
            outErrno = EINTR;
        else if (werr == WSAETIMEDOUT)
            outErrno = EAGAIN; // treat as would-block/timeout
        else
            outErrno = werr;
#else
        outErrno = errno;
#endif
        return -1;
    }
    char addrBuf[INET_ADDRSTRLEN] = {0};
    const char *ap = inet_ntop(AF_INET, &src.sin_addr, addrBuf, sizeof(addrBuf));
    fromAddress = ap ? std::string(addrBuf) : std::string();
    outErrno = 0;
    return bytes;
}

void UdpSocket::close()
{
    if (fd_ >= 0)
    {
        CLOSESOCK(fd_);
        fd_ = -1;
        boundPort_ = 0;
    }
}
