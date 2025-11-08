#include "session_manager.hpp"
#include "nlohmann/json.hpp"
#include <iostream>

using json = nlohmann::json;

SessionManager::SessionManager(int port, const std::string &id) : port(port), id(id)
{
}

SessionManager::~SessionManager()
{
    stop();
}

void SessionManager::initBroadcastSocket()
{
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
}

void SessionManager::start()
{
    initBroadcastSocket();

    receiveThreadRunning = true;
    receiveThread = std::make_unique<std::thread>(&SessionManager::receiveThreadLoop, this);

    pingThreadRunning = true;
    pingThread = std::make_unique<std::thread>(&SessionManager::pingThreadLoop, this);
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

    close(socketFd);
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

void SessionManager::receiveThreadLoop()
{
    char buffer[256];
    sockaddr_in sockAddr{};
    socklen_t addrLen = sizeof(sockAddr);

    while (receiveThreadRunning)
    {
        int bytes = recvfrom(socketFd, buffer, sizeof(buffer) - 1, 0, (sockaddr *)&sockAddr, &addrLen);
        if (bytes < 0)
        {
            std::cerr << "[SessionManager] Failed to receive message" << std::endl;
            receiveThreadRunning = false;
            continue;
        }

        buffer[bytes] = '\0';
        std::string msgRow(buffer);
        std::string senderAddr = inet_ntoa(sockAddr.sin_addr);

        try
        {
            json msgJson = json::parse(msgRow);
            // check message
            if (msgJson.contains("SESSION") && msgJson["SESSION"] != "SESSION")
                continue;

            if (msgJson["type"] == "ping")
            {
                const std::string &peerId = msgJson["from"];

                if (!peers.contains(peerId))
                {
                    peers[peerId] = Peer(peerId, senderAddr, false, false, std::chrono::steady_clock::now());
                }
                else
                {
                    peers[peerId].address = senderAddr;
                    peers[peerId].lastSeen = std::chrono::steady_clock::now();
                }

                sendPong(peerId);
            }

            if (msgJson["type"] == "pong")
            {
                const std::string &peerId = msgJson["from"];
                const bool sending = msgJson["sending"];
                const bool receiving = msgJson["receiving"];

                if (peers[peerId].sending && !receiving)
                {
                    // TODO: 송신 중단
                }

                if (peers[peerId].receiving && !sending)
                {
                    // TODO: 수신 중단
                }
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
    const Peer &toPeer = peers[toId];

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = inet_addr(toPeer.address.c_str());

    json j;
    j["SESSION"] = "SESSION"; // for check if message is from this program.
    j["type"] = "pong";
    j["from"] = id;
    j["to"] = toPeer.id;
    j["sending"] = toPeer.sending;
    j["receiving"] = toPeer.receiving;

    std::string message = j.dump();
    if (sendto(socketFd, message.c_str(), message.size(), 0, (sockaddr *)&addr, sizeof(addr)) < 0)
        std::cerr << "[SessionManager] Failed to send pong message" << std::endl;
}
