#include "lookup_service.hpp"
#include "udp_socket.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <cerrno>
#include <cstring>

using json = nlohmann::json;

LookupService::LookupService(int port, const std::string &myId, const std::string &myName, const std::string &myType)
    : port(port), myId(myId), myName(myName), myType(myType)
{
}

LookupService::~LookupService()
{
    stop();
}

void LookupService::initSocket()
{
    udp = std::make_unique<UdpSocket>();
    udp->openBroadcast(port);
    udp->setRecvTimeout(std::chrono::milliseconds(1000));
}

void LookupService::start()
{
    stopSource = std::stop_source();
    initSocket();

    std::cout << "[LookupService] Starting on port=" << port << " id=" << myId << std::endl;

    receiveThreadRunning = true;
    receiveThread = std::make_unique<std::thread>(&LookupService::receiveThreadLoop, this, stopSource.get_token());

    pingThreadRunning = true;
    pingThread = std::make_unique<std::thread>(&LookupService::pingThreadLoop, this, stopSource.get_token());
}

void LookupService::stop()
{
    std::cout << "[LookupService] Stopping" << std::endl;

    stopSource.request_stop();
    sleepCv.notify_all();

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

    if (udp)
    {
        udp->close();
        udp.reset();
    }
}

std::vector<PeerInfo> LookupService::getPeers()
{
    std::vector<PeerInfo> peerList;
    std::shared_lock lock(peersMutex);
    peerList.reserve(peers.size());
    for (const auto &kv : peers)
        peerList.push_back(kv.second);
    return peerList;
}

void LookupService::pingThreadLoop(std::stop_token st)
{
    while (pingThreadRunning && !st.stop_requested())
    {
        json j;
        j["LOOKUP"] = "LOOKUP";
        j["type"] = "ping";
        j["from"] = myId;
        j["peerName"] = myName;
        j["peerType"] = myType;

        std::string message = j.dump();
        if (!udp->sendTo("255.255.255.255", port, message.c_str(), message.size()))
            std::cerr << "[LookupService] Failed to send ping" << std::endl;

        std::unique_lock<std::mutex> lock(sleepMutex);
        sleepCv.wait_for(lock, st, std::chrono::seconds(1), []()
                         { return false; });
    }
}

void LookupService::receiveThreadLoop(std::stop_token st)
{
    char buffer[2048];

    while (receiveThreadRunning && !st.stop_requested())
    {
        int err = 0;
        std::string senderAddr;
        int bytes = udp->recvFrom(buffer, sizeof(buffer) - 1, senderAddr, err);
        if (bytes < 0)
        {
            if (err == EAGAIN || err == EWOULDBLOCK || err == EINTR)
                continue;

            std::cerr << "[LookupService] Failed to receive message: " << strerror(err) << std::endl;
            break;
        }

        buffer[bytes] = '\0';
        std::string msgRaw(buffer);

        try
        {
            const json msgJson = json::parse(msgRaw);
            if (!msgJson.contains("LOOKUP") || msgJson["LOOKUP"] != "LOOKUP")
                continue;

            const std::string msgType = msgJson.value("type", std::string());
            if (msgType.empty())
                continue;

            const std::string peerId = msgJson.value("from", std::string());
            if (peerId.empty() || peerId == myId)
                continue;

            const std::string peerName = msgJson.value("peerName", std::string());
            const std::string peerType = msgJson.value("peerType", std::string());
            const auto now = std::chrono::steady_clock::now();

            if (msgType == "ping")
            {
                std::scoped_lock lock(peersMutex);
                peers[peerId] = {peerId, peerName, peerType, senderAddr, now};
            }
        }
        catch (json::parse_error &e)
        {
            std::cerr << "[LookupService] Failed to parse message from " << senderAddr << ": " << e.what() << std::endl;
        }
    }
}
