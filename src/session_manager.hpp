#include <unordered_map>
#include <thread>
#include <string>
#include <memory>
#include <chrono>

struct Peer
{
    std::string id;
    std::string address;
    bool sending = false;
    bool receiving = false;
    std::chrono::steady_clock::time_point lastSeen;
};

class SessionManager
{
public:
    SessionManager(int port, const std::string &id);
    ~SessionManager();

    void start();
    void stop();
    
private:
    int port;
    std::string id;

    int socketFd;
    sockaddr_in broadcastAddr{};
    sockaddr_in localAddr{};

    std::unique_ptr<std::thread> pingThread;
    std::unique_ptr<std::thread> receiveThread;
    bool pingThreadRunning = false;
    bool receiveThreadRunning = false;

    std::unordered_map<std::string, Peer> peers; // id, Peer

    void initBroadcastSocket();
    void pingThreadLoop();
    void receiveThreadLoop();
    void sendPong(const std::string &id);
};
