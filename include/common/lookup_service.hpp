#ifndef lookup_service_hpp
#define lookup_service_hpp

#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <shared_mutex>
#include <atomic>
#include <stop_token>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <memory>
#include "udp_socket.hpp"

struct PeerInfo
{
    std::string id;
    std::string name;
    std::string type; // MacOS, Windows, Linux, Android, iOS, Others
    std::string address;
    std::chrono::steady_clock::time_point lastSeen;
};

class LookupService
{
public:
    LookupService(int port, const std::string &myId, const std::string &myName, const std::string &myType);
    ~LookupService();

    void start();
    void stop();

    std::vector<PeerInfo> getPeers();

    std::string getMyId() const { return myId; }

private:
    int port;
    std::string myId;
    std::string myName;
    std::string myType;

    std::unique_ptr<UdpSocket> udp;

    std::unique_ptr<std::thread> pingThread;
    std::unique_ptr<std::thread> receiveThread;
    std::atomic<bool> pingThreadRunning{false};
    std::atomic<bool> receiveThreadRunning{false};

    std::stop_source stopSource;
    std::condition_variable_any sleepCv;
    std::mutex sleepMutex;

    std::unordered_map<std::string, PeerInfo> peers;
    std::shared_mutex peersMutex;

    void initSocket();
    void pingThreadLoop(std::stop_token st);
    void receiveThreadLoop(std::stop_token st);
};

#endif // lookup_service_hpp
