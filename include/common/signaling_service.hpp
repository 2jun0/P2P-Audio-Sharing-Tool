#ifndef signaling_service_hpp
#define signaling_service_hpp

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <unordered_map>

class SignalingService
{
public:
    // Called when an offer is received. Return listen port to accept, or -1 to reject.
    using OfferReceivedCallback = std::function<int(const std::string &peerId, const std::string &peerAddress)>;

    // Called when our offer is accepted by a peer.
    using AcceptReceivedCallback = std::function<void(const std::string &peerId, const std::string &peerAddress, int port)>;

    // Called when a peer's TCP connection drops (remote close or error).
    using DisconnectedCallback = std::function<void(const std::string &peerId)>;

    SignalingService(int port, const std::string &myId,
                     OfferReceivedCallback onOfferReceived,
                     AcceptReceivedCallback onAccepted,
                     DisconnectedCallback onDisconnected);
    ~SignalingService();

    void start();
    void stop();

    void sendOffer(const std::string &peerAddress);
    void disconnect(const std::string &peerId);

private:
    int port;
    std::string myId;

    int serverFd = -1;
    std::unique_ptr<std::thread> acceptThread;
    std::atomic<bool> running{false};

    const OfferReceivedCallback onOfferReceived;
    const AcceptReceivedCallback onAccepted;
    const DisconnectedCallback onDisconnected;

    std::unordered_map<std::string, int> connections; // peerId → fd
    std::mutex connectionsMutex;
    std::atomic<int> activeMonitors{0};

    void acceptThreadLoop();
    void handleClient(int clientFd, const std::string &clientAddr);
    void addConnection(const std::string &peerId, int fd);
    void monitorConnection(const std::string &peerId, int fd);
};

#endif // signaling_service_hpp
