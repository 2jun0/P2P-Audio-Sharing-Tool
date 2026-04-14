#include "signaling_service.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <cerrno>
#include <cstring>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define CLOSESOCK closesocket
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#define CLOSESOCK ::close
#endif

using json = nlohmann::json;

// Read all available data from a socket until the peer closes or a complete JSON is received.
static std::string readAll(int fd)
{
    std::string result;
    char buf[2048];
    while (true)
    {
        ssize_t n = ::recv(fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0)
            break;
        result.append(buf, static_cast<size_t>(n));

        // Try to parse as JSON; if successful, we have the full message.
        try
        {
            json::parse(result);
            break;
        }
        catch (...)
        {
        }
    }
    return result;
}

SignalingService::SignalingService(int port, const std::string &myId,
                                   OfferReceivedCallback onOfferReceived,
                                   AcceptReceivedCallback onAccepted,
                                   DisconnectedCallback onDisconnected)
    : port(port), myId(myId),
      onOfferReceived(std::move(onOfferReceived)),
      onAccepted(std::move(onAccepted)),
      onDisconnected(std::move(onDisconnected))
{
}

SignalingService::~SignalingService()
{
    stop();
}

void SignalingService::start()
{
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    serverFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0)
        throw std::runtime_error("[SignalingService] Failed to create TCP socket");

    int optval = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&optval), sizeof(optval));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (::bind(serverFd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
    {
        CLOSESOCK(serverFd);
        serverFd = -1;
        throw std::runtime_error("[SignalingService] Failed to bind TCP port " + std::to_string(port));
    }

    if (::listen(serverFd, 5) < 0)
    {
        CLOSESOCK(serverFd);
        serverFd = -1;
        throw std::runtime_error("[SignalingService] Failed to listen");
    }

    std::cout << "[SignalingService] Listening on TCP port " << port << std::endl;

    running = true;
    acceptThread = std::make_unique<std::thread>(&SignalingService::acceptThreadLoop, this);
}

void SignalingService::stop()
{
    std::cout << "[SignalingService] Stopping" << std::endl;

    running = false;

    // Close server socket to unblock accept()
    if (serverFd >= 0)
    {
        CLOSESOCK(serverFd);
        serverFd = -1;
    }

    // Close all peer connections to unblock monitor threads
    {
        std::lock_guard lock(connectionsMutex);
        for (auto &[_, fd] : connections)
            CLOSESOCK(fd);
        connections.clear();
    }

    if (acceptThread && acceptThread->joinable())
    {
        acceptThread->join();
        acceptThread.reset();
    }

    // Wait for all monitor threads to finish
    while (activeMonitors > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void SignalingService::sendOffer(const std::string &peerAddress)
{
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        std::cerr << "[SignalingService] Failed to create socket for offer" << std::endl;
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    inet_pton(AF_INET, peerAddress.c_str(), &addr.sin_addr);

    if (::connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
    {
        std::cerr << "[SignalingService] Failed to connect to " << peerAddress << ":" << port << std::endl;
        CLOSESOCK(fd);
        return;
    }

    json offer;
    offer["type"] = "offer";
    offer["from"] = myId;
    std::string msg = offer.dump();

    ::send(fd, msg.c_str(), msg.size(), 0);
    std::cout << "[SignalingService] Sent offer to " << peerAddress << std::endl;

    // Wait for response
    std::string response = readAll(fd);

    if (response.empty())
    {
        std::cerr << "[SignalingService] No response from " << peerAddress << std::endl;
        CLOSESOCK(fd);
        return;
    }

    try
    {
        const json resp = json::parse(response);
        const std::string type = resp.value("type", std::string());
        const std::string fromId = resp.value("from", std::string());

        if (type == "accept")
        {
            int acceptPort = resp.value("port", -1);
            std::cout << "[SignalingService] Offer accepted by " << fromId << " on port " << acceptPort << std::endl;

            addConnection(fromId, fd); // fd now owned by monitor thread

            if (onAccepted)
                onAccepted(fromId, peerAddress, acceptPort);
        }
        else
        {
            std::cout << "[SignalingService] Offer rejected by " << fromId << std::endl;
            CLOSESOCK(fd);
        }
    }
    catch (json::parse_error &e)
    {
        std::cerr << "[SignalingService] Failed to parse response: " << e.what() << std::endl;
        CLOSESOCK(fd);
    }
}

void SignalingService::disconnect(const std::string &peerId)
{
    int fd = -1;
    {
        std::lock_guard lock(connectionsMutex);
        auto it = connections.find(peerId);
        if (it != connections.end())
        {
            fd = it->second;
            connections.erase(it);
        }
    }
    if (fd >= 0)
    {
        CLOSESOCK(fd);
        std::cout << "[SignalingService] Disconnected peer " << peerId << std::endl;
    }
}

void SignalingService::acceptThreadLoop()
{
    while (running)
    {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientFd = ::accept(serverFd, reinterpret_cast<sockaddr *>(&clientAddr), &clientLen);
        if (clientFd < 0)
        {
            if (!running)
                break;
            std::cerr << "[SignalingService] Accept error: " << strerror(errno) << std::endl;
            continue;
        }

        char addrBuf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, addrBuf, sizeof(addrBuf));
        std::string clientAddrStr(addrBuf);

        // Handle each client in a detached thread to not block accept loop
        std::thread(&SignalingService::handleClient, this, clientFd, clientAddrStr).detach();
    }
}

void SignalingService::handleClient(int clientFd, const std::string &clientAddr)
{
    std::string raw = readAll(clientFd);
    if (raw.empty())
    {
        CLOSESOCK(clientFd);
        return;
    }

    try
    {
        const json msg = json::parse(raw);
        const std::string type = msg.value("type", std::string());
        const std::string fromId = msg.value("from", std::string());

        if (type == "offer" && !fromId.empty())
        {
            std::cout << "[SignalingService] Received offer from " << fromId << " (" << clientAddr << ")" << std::endl;

            int acceptPort = onOfferReceived(fromId, clientAddr);

            json resp;
            resp["from"] = myId;

            if (acceptPort >= 0)
            {
                resp["type"] = "accept";
                resp["port"] = acceptPort;
                std::cout << "[SignalingService] Accepting with port " << acceptPort << std::endl;

                std::string respStr = resp.dump();
                ::send(clientFd, respStr.c_str(), respStr.size(), 0);

                addConnection(fromId, clientFd); // fd now owned by monitor thread
            }
            else
            {
                resp["type"] = "reject";
                std::cout << "[SignalingService] Rejecting offer from " << fromId << std::endl;

                std::string respStr = resp.dump();
                ::send(clientFd, respStr.c_str(), respStr.size(), 0);
                CLOSESOCK(clientFd);
            }
        }
        else
        {
            CLOSESOCK(clientFd);
        }
    }
    catch (json::parse_error &e)
    {
        std::cerr << "[SignalingService] Parse error: " << e.what() << std::endl;
        CLOSESOCK(clientFd);
    }
}

void SignalingService::addConnection(const std::string &peerId, int fd)
{
    {
        std::lock_guard lock(connectionsMutex);
        auto it = connections.find(peerId);
        if (it != connections.end())
        {
            CLOSESOCK(it->second);
            connections.erase(it);
        }
        connections[peerId] = fd;
    }

    std::thread(&SignalingService::monitorConnection, this, peerId, fd).detach();
}

void SignalingService::monitorConnection(const std::string &peerId, int fd)
{
    activeMonitors++;

    char buf[1];
    while (running)
    {
        ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
        if (n <= 0)
            break;
        // Discard any unexpected data
    }

    // Check if this connection was already removed by disconnect()
    bool remoteDisconnect = false;
    {
        std::lock_guard lock(connectionsMutex);
        remoteDisconnect = connections.erase(peerId) > 0;
    }

    if (remoteDisconnect)
    {
        CLOSESOCK(fd);

        if (running)
        {
            std::cout << "[SignalingService] Peer " << peerId << " disconnected" << std::endl;

            if (onDisconnected)
                onDisconnected(peerId);
        }
    }
    // else: fd already closed by disconnect() or stop()

    activeMonitors--;
}
