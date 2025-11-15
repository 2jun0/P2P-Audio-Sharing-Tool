#ifndef audio_streamer_hpp
#define audio_streamer_hpp

#include <string>
#include <memory>
#include <unordered_map>
#include <shared_mutex>
#include <vector>
#include "audio_sender.hpp"
#include "audio_receiver.hpp"
#include "session_manager.hpp"

class AudioStreamer
{
public:
    AudioStreamer(int port, const std::string &myId);
    ~AudioStreamer();

    void start();
    void stop();

    // User API
    void startSendingTo(const std::string &peerId);
    void stopSendingTo(const std::string &peerId);
    void startReceivingFrom(const std::string &peerId);
    void stopReceivingFrom(const std::string &peerId);
    std::vector<std::string> getPeerIds() const { return sessionMgr->getPeerIds(); }

private:
    int port;

    std::unique_ptr<SessionManager> sessionMgr;

    std::unordered_map<std::string, std::unique_ptr<AudioSender>> senders;     // peerId, AudioSender
    std::unordered_map<std::string, std::unique_ptr<AudioReceiver>> receivers; // peerId, AudioReceiver
    std::shared_mutex audiosMutex;

    // SessionManager callbacks
    void updateSender(const std::string &peerId, bool shouldSend, const std::string &host, int port);
    void updateReceiver(const std::string &peerId, bool shouldReceive, const std::string &host);
};

#endif /* audio_streamer_hpp */