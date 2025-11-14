#include <unordered_map>
#include <thread>
#include <string>
#include <memory>
#include <chrono>
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

    // User-triggered state changes (intent)
    // Sets wantToSendTo for a peer and broadcasts to notify them
    void setWantToSendTo(const std::string &peerId, bool want);
    // Sets wantToReceiveFrom for a peer and broadcasts to notify them
    void setWantToReceiveFrom(const std::string &peerId, bool want);

    // External stop trigger (e.g., gstreamer timeout on receiver side)
    // Stops receiving from peer without waiting for peer message
    void stopReceivingFrom(const std::string &peerId);

    // Callbacks for state updates
    void setOnConnectionLoss(std::function<void(const std::string &)> cb) { onConnectionLoss = std::move(cb); }
    // callback for sendingTo/receivingFrom state updates per-peer: (peerId, newState)
    void setOnSendingStateUpdate(std::function<void(const std::string &, bool)> cb) { onSendingStateUpdate = std::move(cb); }
    void setOnReceivingStateUpdate(std::function<void(const std::string &, bool)> cb) { onReceivingStateUpdate = std::move(cb); }

private:
    int port;
    std::string id;

    int socketFd;
    sockaddr_in broadcastAddr{};
    sockaddr_in localAddr{};
    std::function<void(const std::string &)> onConnectionLoss;
    // callbacks for localSending/localReceiving state updates per-peer: (peerId, newState)
    std::function<void(const std::string &, bool)> onSendingStateUpdate;
    std::function<void(const std::string &, bool)> onReceivingStateUpdate;

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
