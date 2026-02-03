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
#include <optional>
#include <stop_token>
#include <condition_variable>
#include <mutex>
#include "udp_socket.hpp"
#include "audio_device.hpp"

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
    std::optional<AudioDevice> inputDevice;
    std::optional<AudioDevice> outputDevice;
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
    void setWantToSendTo(const std::string &peerId, bool want, const std::optional<AudioDevice> &inputDevice = std::nullopt);
    void setWantToReceiveFrom(const std::string &peerId, bool want, const std::optional<AudioDevice> &outputDevice = std::nullopt);

    // Sender / Receiver triggered state changes
    void updateSendingState(const std::string &peerId, bool sending, const std::optional<AudioDevice> &inputDevice = std::nullopt);
    void updateReceivingState(const std::string &peerId, bool receiving, int port, const std::optional<AudioDevice> &outputDevice = std::nullopt);

    // Callback for trigger Sender or Receiver
    void setOnSendRequest(std::function<void(const std::string &, bool, const std::string &, int, const std::optional<AudioDevice> &)> cb) { onSendRequest = std::move(cb); }
    void setOnReceiveRequest(std::function<void(const std::string &, bool, const std::string &, const std::optional<AudioDevice> &)> cb) { onReceiveRequest = std::move(cb); }

    std::string getId() const { return id; }

private:
    int port;
    std::string id;

    std::unique_ptr<UdpSocket> udp;
    // callbacks for sending/receiving state updates per-peer: (peerId, newStates...)
    std::function<void(const std::string &, bool, const std::string &, int, const std::optional<AudioDevice> &)> onSendRequest;
    std::function<void(const std::string &, bool, const std::string &, const std::optional<AudioDevice> &)> onReceiveRequest;

    std::unique_ptr<std::thread> healthCheckThread;
    std::atomic<bool> healthCheckThreadRunning{false};

    std::unique_ptr<std::thread> pingThread;
    std::unique_ptr<std::thread> receiveThread;
    std::atomic<bool> pingThreadRunning{false};
    std::atomic<bool> receiveThreadRunning{false};

    std::stop_source stopSource;
    std::condition_variable_any sleepCv;
    std::mutex sleepMutex;

    std::unordered_map<std::string, Peer> peers; // id, Peer
    std::shared_mutex peersMutex;

    void initSocket();
    void pingThreadLoop(std::stop_token st);
    void receiveThreadLoop(std::stop_token st);
    void healthCheckThreadLoop(std::stop_token st);
    void sendPong(const std::string &id);
};

#endif // session_manager_hpp
