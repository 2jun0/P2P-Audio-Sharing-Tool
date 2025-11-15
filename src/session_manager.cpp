#include "session_manager.hpp"
#ifdef _WIN32
#include "winsock_guard.hpp"
#endif
#include <nlohmann/json.hpp>
#include <vector>
#include <cerrno>
#include <iostream>

using json = nlohmann::json;

SessionManager::SessionManager(int port, const std::string &id) : port(port), id(id), socketFd(-1)
{
}

SessionManager::~SessionManager()
{
    stop();
}

void SessionManager::initSocket()
{
#ifdef _WIN32
    ensureWinsock();
#endif
    socketFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd < 0)
        throw std::runtime_error("Failed to create broadcast socket");

    int broadcastEnable = 1;
#ifdef _WIN32
    int rs = setsockopt(socketFd, SOL_SOCKET, SO_BROADCAST, (char *)&broadcastEnable, sizeof(broadcastEnable));
#else
    int rs = setsockopt(socketFd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
#endif
    if (rs < 0)
    {
        close(socketFd);
        throw std::runtime_error("Failed to set broadcast socket option (setsockopt)");
    }

    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(port);
    broadcastAddr.sin_addr.s_addr = inet_addr("255.255.255.255");

    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(port);
    localAddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(socketFd, (sockaddr *)&localAddr, sizeof(localAddr)) < 0)
        throw std::runtime_error("Failed to bind local address");

    // set recv timeout so recvfrom won't block indefinitely (helps stop())
    struct timeval tv;
    tv.tv_sec = 1; // 1 second
    tv.tv_usec = 0;
    setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
}

void SessionManager::start()
{
    initSocket();

    receiveThreadRunning = true;
    receiveThread = std::make_unique<std::thread>(&SessionManager::receiveThreadLoop, this);

    pingThreadRunning = true;
    pingThread = std::make_unique<std::thread>(&SessionManager::pingThreadLoop, this);

    healthCheckThreadRunning = true;
    healthCheckThread = std::make_unique<std::thread>(&SessionManager::healthCheckThreadLoop, this);
}

void SessionManager::stop()
{
    pingThreadRunning = false;
    if (pingThread && pingThread->joinable())
    {
        pingThread->join();
        pingThread.reset();
    }

    receiveThreadRunning = false;
    if (receiveThread && receiveThread->joinable())
    {
        receiveThread->join();
        receiveThread.reset();
    }

    healthCheckThreadRunning = false;
    if (healthCheckThread && healthCheckThread->joinable())
    {
        healthCheckThread->join();
        healthCheckThread.reset();
    }

    close(socketFd);
}

std::vector<std::string> SessionManager::getPeerIds()
{
    std::vector<std::string> ids;
    std::shared_lock lock(peersMutex);
    for (const auto &kv : peers)
        ids.push_back(kv.first);
    return ids;
}

void SessionManager::pingThreadLoop()
{
    while (pingThreadRunning)
    {
        json j;
        j["SESSION"] = "SESSION"; // for check if message is from this program.
        j["type"] = "ping";
        j["from"] = id;

        std::string message = j.dump();
        if (sendto(socketFd, message.c_str(), message.size(), 0, (sockaddr *)&broadcastAddr, sizeof(broadcastAddr)) < 0)
            std::cerr << "[SessionManager] Failed to send ping" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void SessionManager::healthCheckThreadLoop()
{
    while (healthCheckThreadRunning)
    {
        const auto now = std::chrono::steady_clock::now();
        const auto timeout = std::chrono::seconds(60);
        std::vector<std::pair<std::string, Peer>> noneReachableList;
        {
            std::scoped_lock lock(peersMutex);
            for (auto &kv : peers)
            {
                // if peer was considered reachable but hasn't been seen within timeout
                if (kv.second.isReachable && (now - kv.second.lastSeen > timeout))
                {
                    Peer copy = kv.second;
                    kv.second.isReachable = false;
                    noneReachableList.emplace_back(kv.first, std::move(copy));
                }
            }
        }

        for (auto &pr : noneReachableList)
        {
            const auto &peerId = pr.first;
            const auto &peer = pr.second;
            if (onReceiveRequest)
                onReceiveRequest(peerId, false, peer.address);
            if (onSendRequest)
                onSendRequest(peerId, false, peer.address, peer.sendPortTo);
        }

        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

void SessionManager::receiveThreadLoop()
{
    char buffer[2048];
    sockaddr_in sockAddr{};

    while (receiveThreadRunning)
    {
        socklen_t addrLen = sizeof(sockAddr);
        int bytes = recvfrom(socketFd, buffer, sizeof(buffer) - 1, 0, (sockaddr *)&sockAddr, &addrLen);
        if (bytes < 0)
        {
#ifdef _WIN32
            int err = WSAGetLastError();
            // timeout or interrupted, just continue
            if (err == WSAEWOULDBLOCK || err == WSAEINTR || err == WSAETIMEDOUT)
                continue;
#else
            int err = errno;
            // timeout or interrupted, just continue
            if (err == EAGAIN || err == EWOULDBLOCK || err == EINTR)
                continue;
#endif

            // non-recoverable error
            std::cerr << "[SessionManager] Failed to receive message: " << strerror(err) << std::endl;
            // TODO: stop all threads
            break;
        }

        buffer[bytes] = '\0';
        std::string msgRow(buffer);
        char addrBuf[INET_ADDRSTRLEN] = {0};
        const char *addrPtr = inet_ntop(AF_INET, &sockAddr.sin_addr, addrBuf, sizeof(addrBuf));
        std::string senderAddr = addrPtr ? std::string(addrBuf) : std::string();

        try
        {
            const json msgJson = json::parse(msgRow);
            // check message
            if (!msgJson.contains("SESSION") || msgJson["SESSION"] != "SESSION")
                continue;

            const std::string msgType = msgJson.value("type", std::string());
            if (msgType.empty())
                continue;

            const std::string peerId = msgJson.value("from", std::string());
            if (peerId.empty() || peerId == id)
                continue;

            if (msgType == "ping")
            {
                const auto now = std::chrono::steady_clock::now();
                {
                    std::scoped_lock lock(peersMutex);
                    peers[peerId].id = peerId;
                    peers[peerId].address = senderAddr;
                    peers[peerId].lastSeen = now;
                    peers[peerId].isReachable = true;
                }

                sendPong(peerId);
            }
            else if (msgType == "pong")
            {
                const int msgReceivePort = msgJson.value("receivePort", -1);
                const bool msgSending = msgJson.value("sending", false);
                const bool msgReceiving = msgJson.value("receiving", false);
                const bool msgWantToSend = msgJson.value("wantToSend", false);
                const bool msgWantToReceive = msgJson.value("wantToReceive", false);

                const auto now = std::chrono::steady_clock::now();
                Peer peerCopy;
                {
                    std::scoped_lock lock(peersMutex);
                    peers[peerId].id = peerId;
                    peers[peerId].address = senderAddr;
                    peers[peerId].sendPortTo = msgReceivePort;
                    peers[peerId].lastSeen = now;
                    peers[peerId].isReachable = true;
                    peerCopy = peers[peerId];
                }

                bool shouldSend = peerCopy.wantToSendTo && msgWantToReceive && peerCopy.sendPortTo > 0;
                bool shouldReceive = peerCopy.wantToReceiveFrom && msgWantToSend;

                if (peerCopy.sendingTo != shouldSend && onSendRequest)
                    onSendRequest(peerId, shouldSend, peerCopy.address, peerCopy.sendPortTo);
                if (peerCopy.receivingFrom != shouldReceive && onReceiveRequest)
                    onReceiveRequest(peerId, shouldReceive, peerCopy.address);
            }
        }
        catch (json::parse_error &e)
        {
            // ignore
        }
    }
}

void SessionManager::sendPong(const std::string &toId)
{
    Peer peerCopy;
    {
        std::shared_lock lock(peersMutex);
        auto it = peers.find(toId);
        if (it == peers.end())
            return;
        peerCopy = it->second;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(peerCopy.address.c_str());

    json j;
    j["SESSION"] = "SESSION"; // for check if message is from this program.
    j["type"] = "pong";
    j["from"] = id;
    j["to"] = peerCopy.id;
    j["receivePort"] = peerCopy.receivePortFrom;
    j["sending"] = peerCopy.sendingTo;
    j["receiving"] = peerCopy.receivingFrom;
    j["wantToSend"] = peerCopy.wantToSendTo;
    j["wantToReceive"] = peerCopy.wantToReceiveFrom;

    std::string message = j.dump();
    if (sendto(socketFd, message.c_str(), message.size(), 0, (sockaddr *)&addr, sizeof(addr)) < 0)
        std::cerr << "[SessionManager] Failed to send pong message" << std::endl;
}

void SessionManager::setWantToSendTo(const std::string &peerId, bool want)
{
    {
        std::scoped_lock lock(peersMutex);
        auto it = peers.find(peerId);
        if (it == peers.end())
            return;
        it->second.wantToSendTo = want;
    }

    sendPong(peerId);
}

void SessionManager::setWantToReceiveFrom(const std::string &peerId, bool want)
{
    {
        std::scoped_lock lock(peersMutex);
        auto it = peers.find(peerId);
        if (it == peers.end())
            return;
        it->second.wantToReceiveFrom = want;
    }

    sendPong(peerId);
}

void SessionManager::updateSendingState(const std::string &peerId, bool sending)
{
    {
        std::scoped_lock lock(peersMutex);
        auto it = peers.find(peerId);
        if (it == peers.end())
            return;
        it->second.sendingTo = sending;
    }

    sendPong(peerId);
}

void SessionManager::updateReceivingState(const std::string &peerId, bool receiving, int port)
{
    {
        std::scoped_lock lock(peersMutex);
        auto it = peers.find(peerId);
        if (it == peers.end())
            return;
        it->second.receivingFrom = receiving;
        it->second.receivePortFrom = receiving ? port : -1;
    }

    sendPong(peerId);
}