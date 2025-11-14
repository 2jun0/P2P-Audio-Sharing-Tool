#ifndef audio_streamer_hpp
#define audio_streamer_hpp

#include <string>
#include <memory>
#include <unordered_map>
#include "audio_sender.hpp"
#include "audo_receiver.hpp"
#include "session_manager.hpp"

class AudioStreamer {
public:
    AudioStreamer(int port, const std::string& myId);
    ~AudioStreamer();

    void start();
    void stop();
    
    // User API
    void startSendingTo(const std::string& peerId, const std::string& peerHost);
    void stopSendingTo(const std::string& peerId);
    void startReceivingFrom(const std::string& peerId);
    void stopReceivingFrom(const std::string& peerId);

private:
    int port;
    
    std::unique_ptr<SessionManager> sessionMgr;
    
    std::unordered_map<std::string, std::unique_ptr<AudioSender>> senders; // peerId, AudioSender
    std::unordered_map<std::string, std::unique_ptr<AudioReceiver>> receivers; // peerId, AudioReceiver
    std::shared_mutex audiosMutex;
    
    // SessionManager callbacks
    void onSendingStateUpdate(const std::string& peerId, bool shouldSend);
    void onReceivingStateUpdate(const std::string& peerId, bool shouldReceive);
    void onConnectionLoss(const std::string& peerId);
};

#endif /* audio_streamer_hpp */