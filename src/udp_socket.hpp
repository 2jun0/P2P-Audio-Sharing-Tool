#ifndef UDP_SOCKET_HPP
#define UDP_SOCKET_HPP

#include <string>
#include <chrono>

// Minimal cross-platform UDP helper wrapping broadcast bind, sendto, recvfrom, and timeout.
class UdpSocket {
public:
    UdpSocket();
    ~UdpSocket();

    // Create socket, enable broadcast, bind to INADDR_ANY:port
    void openBroadcast(int port);

    // Set receive timeout
    void setRecvTimeout(std::chrono::milliseconds timeout);

    // Send to address:port
    bool sendTo(const std::string& address, int port, const char* data, size_t len);

    // Receive into buffer; returns bytes or -1 on error; fills sender address string and error code
    int recvFrom(char* buffer, size_t len, std::string& fromAddress, int& outErrno);

    // Close the socket if open
    void close();

    bool isOpen() const { return fd_ >= 0; }
    int native() const { return fd_; }

private:
    int fd_;
    int boundPort_;
};

#endif // UDP_SOCKET_HPP
