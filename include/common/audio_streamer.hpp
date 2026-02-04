#ifndef audio_streamer_hpp
#define audio_streamer_hpp

#include <string>
#include <memory>
#include <unordered_map>
#include <shared_mutex>
#include <vector>
#include <cstdint>
#include <optional>
#include "audio_sender.hpp"
#include "audio_receiver.hpp"
#include "session_manager.hpp"
#include "export.hpp"

#if defined(_WIN32)
#include "win_audio_device_manager.hpp"
#elif defined(__APPLE__)
#include "mac_audio_device_manager.hpp"
#endif

class AUDIO_API AudioStreamer
{
public:
    AudioStreamer(int port, const std::string &myId, const std::string &myName, const std::string &myType);
    ~AudioStreamer();

    void start();
    void stop();

    // User API
    void startSendingTo(const std::string &peerId, const std::optional<std::string> &inputDeviceUID = std::nullopt);
    void stopSendingTo(const std::string &peerId);
    void startReceivingFrom(const std::string &peerId, const std::optional<std::string> &outputDeviceUID = std::nullopt);
    void stopReceivingFrom(const std::string &peerId);
    void changeOutputDevice(const std::string &peerId, const std::optional<std::string> &outputDeviceUID);
    void changeInputDevice(const std::string &peerId, const std::optional<std::string> &inputDeviceUID);
    std::vector<Peer> getPeers();
#if defined(_WIN32) || defined(__APPLE__)
    AudioDeviceManager &getAudioDeviceManager();
#endif

#if defined(__ANDROID__)
    void submitCapturedAudio(const int16_t *pcmFrames, size_t frameCount, int sampleRate, int channelCount);
#endif

private:
    int port;

    std::unique_ptr<SessionManager> sessionMgr;

    std::unordered_map<std::string, std::unique_ptr<AudioSender>> senders;     // peerId, AudioSender
    std::unordered_map<std::string, std::unique_ptr<AudioReceiver>> receivers; // peerId, AudioReceiver
    std::shared_mutex audiosMutex;

#if defined(_WIN32) || defined(__APPLE__)
    AudioDeviceManager audioDeviceManager;
#endif
    // SessionManager callbacks
    void updateSender(const std::string &peerId, bool shouldSend, const std::string &host, int port, const std::optional<AudioDevice> &inputDevice);
    void updateReceiver(const std::string &peerId, bool shouldReceive, const std::string &host, const std::optional<AudioDevice> &outputDevice);

    void handleDefaultOutputDeviceChange(const AudioDevice &device);
    std::optional<AudioDevice> findAudioDeviceByUID(const std::optional<std::string> &uid);
};

#endif /* audio_streamer_hpp */