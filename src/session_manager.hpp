#ifndef session_manager_hpp
#define session_manager_hpp

#include <unordered_map>
#include <thread>
#include <string>
#include <memory>
#include <chrono>
#include <vector>
#include <shared_mutex>
#include <functional>
#include <atomic>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define close closesocket
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

struct Peer
{
    std::string id;
    std::string address;
    // I'm sending / receiving / want to ~~~ to this peer
    int sendPortTo = -1;
    int receivePortFrom = -1;
    bool sendingTo = false;
    bool receivingFrom = false;
    bool wantToSendTo = false;
    bool wantToReceiveFrom = false;
    bool isReachable = false;
    std::chrono::steady_clock::time_point lastSeen;
};

class SessionManager
{
public:
    SessionManager(int port, const std::string &id);
    ~SessionManager();

    void start();
    void stop();

    std::vector<std::string> getPeerIds();

    // User-triggered state changes
    void setWantToSendTo(const std::string &peerId, bool want);
    void setWantToReceiveFrom(const std::string &peerId, bool want);

    // Sender / Receiver triggered state changes
    void updateSendingState(const std::string &peerId, bool sending);
    void updateReceivingState(const std::string &peerId, bool receiving, int port);

    // Callback for trigger Sender or Receiver
    void setOnSendRequest(std::function<void(const std::string &, bool, const std::string &, int)> cb) { onSendRequest = std::move(cb); }
    void setOnReceiveRequest(std::function<void(const std::string &, bool, const std::string &)> cb) { onReceiveRequest = std::move(cb); }

    std::string getId() const { return id; }

private:
    int port;
    std::string id;

    int socketFd;
    sockaddr_in broadcastAddr{};
    sockaddr_in localAddr{};
    // callbacks for sending/receiving state updates per-peer: (peerId, newState)
    std::function<void(const std::string &, bool, const std::string &, int)> onSendRequest;
    std::function<void(const std::string &, bool, const std::string &)> onReceiveRequest;

    std::unique_ptr<std::thread> healthCheckThread;
    std::atomic<bool> healthCheckThreadRunning{false};

    std::unique_ptr<std::thread> pingThread;
    std::unique_ptr<std::thread> receiveThread;
    std::atomic<bool> pingThreadRunning{false};
    std::atomic<bool> receiveThreadRunning{false};

    std::unordered_map<std::string, Peer> peers; // id, Peer
    std::shared_mutex peersMutex;

    void initSocket();
    void pingThreadLoop();
    void receiveThreadLoop();
    void healthCheckThreadLoop();
    void sendPong(const std::string &id);
};

#endif // session_manager_hpp
