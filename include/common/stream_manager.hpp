#ifndef stream_manager_hpp
#define stream_manager_hpp

#include <string>
#include <memory>
#include <unordered_map>
#include <shared_mutex>
#include <optional>
#include "audio_sender.hpp"
#include "audio_receiver.hpp"
#include "audio_device.hpp"
#include "export.hpp"

class AUDIO_API StreamManager
{
public:
    StreamManager();
    ~StreamManager();

    // Sender management
    void addSender(const std::string &id, const std::string &targetHost, int targetPort, const std::optional<AudioDevice> &inputDevice = std::nullopt);
    void removeSender(const std::string &id);
    void removeAllSenders();

    // Receiver management
    void addReceiver(const std::string &id, const std::optional<AudioDevice> &outputDevice = std::nullopt);
    void removeReceiver(const std::string &id);
    void removeAllReceivers();

    // Query
    int getReceiverPort(const std::string &id);
    bool hasSender(const std::string &id);
    bool hasReceiver(const std::string &id);

#if defined(__ANDROID__)
    void submitCapturedAudio(const int16_t *pcmFrames, size_t frameCount, int sampleRate, int channelCount);
#endif

private:
    std::unordered_map<std::string, std::unique_ptr<AudioSender>> senders;
    std::unordered_map<std::string, std::unique_ptr<AudioReceiver>> receivers;
    std::shared_mutex mutex;
};

#endif // stream_manager_hpp
