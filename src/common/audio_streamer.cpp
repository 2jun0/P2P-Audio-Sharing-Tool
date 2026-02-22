#include "audio_streamer.hpp"
#include <iostream>
#include <gst/gst.h>
#include <vector>
#include <shared_mutex>

AudioStreamer::AudioStreamer(int port, const std::string &myId, const std::string &myName, const std::string &myType)
    : port(port)
{
    gst_init(nullptr, nullptr);

#if defined(_WIN32) || defined(__APPLE__)
    audioDeviceManager.setDefaultOutputDeviceChangeCallback(
        [this](AudioDevice device)
        {
            this->handleDefaultOutputDeviceChange(device);
        });
#endif

    sessionMgr = std::make_unique<SessionManager>(port, myId, myName, myType);

    // Register callbacks from SessionManager
    sessionMgr->setOnSendRequest(
        [this](const std::string &peerId, bool shouldSend, const std::string &host, int port, const std::optional<AudioDevice> &inputDevice)
        {
            this->updateSender(peerId, shouldSend, host, port, inputDevice);
        });

    sessionMgr->setOnReceiveRequest(
        [this](const std::string &peerId, bool shouldReceive, const std::string &host, const std::optional<AudioDevice> &outputDevice)
        {
            this->updateReceiver(peerId, shouldReceive, host, outputDevice);
        });
}

AudioStreamer::~AudioStreamer()
{
    stop();
}

void AudioStreamer::start()
{
    std::cout << "[AudioStreamer] Starting (session port=" << port << ")" << std::endl;
    sessionMgr->start();
}

void AudioStreamer::stop()
{
    std::cout << "[AudioStreamer] Stopping streamer" << std::endl;
    {
        std::scoped_lock lock(audiosMutex);
        senders.clear();
        receivers.clear();
    }

    sessionMgr->stop();
}

#if defined(__ANDROID__)
void AudioStreamer::submitCapturedAudio(const int16_t *pcmFrames, size_t frameCount, int sampleRate, int channelCount)
{
    if (!pcmFrames || frameCount == 0 || sampleRate <= 0 || channelCount <= 0)
        return;

    std::shared_lock lock(audiosMutex);
    for (auto &entry : senders)
    {
        entry.second->pushPcmFrame(pcmFrames, frameCount, sampleRate, channelCount);
    }
}
#endif

void AudioStreamer::startSendingTo(const std::string &peerId, const std::optional<std::string> &inputDeviceUID)
{
    std::optional<AudioDevice> inputDevice = findAudioDeviceByUID(inputDeviceUID);
    std::cout << "[AudioStreamer] Request startSendingTo peer=" << peerId << " inputDeviceUID=" << (inputDeviceUID.has_value() ? inputDeviceUID.value() : "<default>") << std::endl;
    sessionMgr->setWantToSendTo(peerId, true, inputDevice);
}

void AudioStreamer::stopSendingTo(const std::string &peerId)
{
    std::cout << "[AudioStreamer] Request stopSendingTo peer=" << peerId << std::endl;
    sessionMgr->setWantToSendTo(peerId, false);
}

void AudioStreamer::startReceivingFrom(const std::string &peerId, const std::optional<std::string> &outputDeviceUID)
{
    std::optional<AudioDevice> outputDevice = findAudioDeviceByUID(outputDeviceUID);
    std::cout << "[AudioStreamer] Request startReceivingFrom peer=" << peerId << " outputDeviceUID=" << (outputDeviceUID.has_value() ? outputDeviceUID.value() : "<default>") << std::endl;
    sessionMgr->setWantToReceiveFrom(peerId, true, outputDevice);
}

void AudioStreamer::stopReceivingFrom(const std::string &peerId)
{
    std::cout << "[AudioStreamer] Request stopReceivingFrom peer=" << peerId << std::endl;
    sessionMgr->setWantToReceiveFrom(peerId, false, std::nullopt);
}

void AudioStreamer::changeOutputDevice(const std::string &peerId, const std::optional<std::string> &outputDeviceUID)
{
    std::optional<AudioDevice> outputDevice = findAudioDeviceByUID(outputDeviceUID);
    std::scoped_lock lock(audiosMutex);

    auto it = receivers.find(peerId);
    if (it == receivers.end())
    {
        std::cerr << "[AudioStreamer] changeOutputDevice ignored; receiver not found for peer=" << peerId << std::endl;
        return;
    }

    std::cout << "[AudioStreamer] Changing output device for peer=" << peerId << std::endl;
    it->second->updateOutputDevice(outputDevice);
}

void AudioStreamer::changeInputDevice(const std::string &peerId, const std::optional<std::string> &inputDeviceUID)
{
    std::optional<AudioDevice> inputDevice = findAudioDeviceByUID(inputDeviceUID);
    std::scoped_lock lock(audiosMutex);

    auto it = senders.find(peerId);
    if (it == senders.end())
    {
        std::cerr << "[AudioStreamer] changeInputDevice ignored; sender not found for peer=" << peerId << std::endl;
        return;
    }

    std::cout << "[AudioStreamer] Changing input device for peer=" << peerId << std::endl;
    it->second->updateInputDevice(inputDevice);
}

std::vector<Peer> AudioStreamer::getPeers()
{
    return sessionMgr->getPeers();
}

AudioDeviceManager &AudioStreamer::getAudioDeviceManager()
{
    return audioDeviceManager;
}

void AudioStreamer::updateSender(const std::string &peerId, bool shouldSend, const std::string &host, int port, const std::optional<AudioDevice> &inputDevice)
{
    std::scoped_lock lock(audiosMutex);
    auto it = senders.find(peerId);

    std::cout << "[AudioStreamer] updateSender peer=" << peerId
              << " shouldSend=" << (shouldSend ? "true" : "false")
              << " host=" << host
              << " port=" << port
              << std::endl;

    if (shouldSend)
    {
        if (it != senders.end())
        {
            std::cout << "[AudioStreamer] Stopping send to peer: " << peerId << std::endl;
            it->second->stop();
            senders.erase(it);
        }

        try
        {
            std::cout << "[AudioStreamer] Starting send to peer: " << peerId << std::endl;
            auto sender = std::make_unique<AudioSender>(host, port, inputDevice);
            sender->setOnStateUpdate(
                [this, peerId](bool started, const std::optional<AudioDevice> &inputDevice)
                {
                    sessionMgr->updateSendingState(peerId, started, inputDevice);
                });
            sender->start();
            senders.emplace(peerId, std::move(sender));
            std::cout << "[AudioStreamer] Send pipeline started successfully for peer=" << peerId << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "[AudioStreamer] Failed to start sender for peer=" << peerId << ": " << e.what() << std::endl;
            sessionMgr->updateSendingState(peerId, false, inputDevice);
        }
    }
    else
    {
        if (it == senders.end())
        {
            std::cout << "[AudioStreamer] Sender already stopped for peer=" << peerId << std::endl;
            return;
        }

        std::cout << "[AudioStreamer] Stopping send to peer: " << peerId << std::endl;
        it->second->stop();
        senders.erase(it);
    }
}

void AudioStreamer::updateReceiver(const std::string &peerId, bool shouldReceive, const std::string &host, const std::optional<AudioDevice> &outputDevice)
{
    std::scoped_lock lock(audiosMutex);
    auto it = receivers.find(peerId);

    std::cout << "[AudioStreamer] updateReceiver peer=" << peerId
              << " shouldReceive=" << (shouldReceive ? "true" : "false")
              << " host=" << host
              << std::endl;

    if (shouldReceive)
    {
        if (it != receivers.end())
        {
            std::cout << "[AudioStreamer] Stopping receive from peer: " << peerId << std::endl;
            it->second->stop();
            receivers.erase(it);
        }

        try
        {
            std::cout << "[AudioStreamer] Starting receive from peer: " << peerId << std::endl;
            auto receiver = std::make_unique<AudioReceiver>(host, outputDevice);
            receiver->setOnStateUpdate(
                [this, peerId](bool receiving, int receivePort, const std::optional<AudioDevice> &outputDevice)
                {
                    sessionMgr->updateReceivingState(peerId, receiving, receivePort, outputDevice);
                });
            receiver->start();
            receivers.emplace(peerId, std::move(receiver));
            std::cout << "[AudioStreamer] Receive pipeline started successfully for peer=" << peerId << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "[AudioStreamer] Failed to start receiver for peer=" << peerId << ": " << e.what() << std::endl;
            sessionMgr->updateReceivingState(peerId, false, -1, outputDevice);
        }
    }
    else
    {
        if (it == receivers.end())
        {
            std::cout << "[AudioStreamer] Receiver already stopped for peer=" << peerId << std::endl;
            return;
        }

        std::cout << "[AudioStreamer] Stopping receive from peer: " << peerId << std::endl;
        it->second->stop();
        receivers.erase(it);
    }
}

void AudioStreamer::handleDefaultOutputDeviceChange(const AudioDevice &device)
{
    std::scoped_lock lock(audiosMutex);
    for (auto &entry : receivers)
    {
        if (!entry.second->isUsingDefaultOutput())
            continue;

        entry.second->updateOutputDevice(std::nullopt);
    }
}

std::optional<AudioDevice> AudioStreamer::findAudioDeviceByUID(const std::optional<std::string> &uid)
{
    if (!uid.has_value())
        return std::nullopt;

    auto devices = audioDeviceManager.findAllAudioDevices();
    for (const auto &device : devices)
    {
        if (device.uid == uid)
        {
            std::cout << "[AudioStreamer] Resolved audio device UID=" << uid.value() << std::endl;
            return device;
        }
    }

    std::cerr << "[AudioStreamer] Audio device not found for UID=" << uid.value() << ", using default device" << std::endl;
    return std::nullopt;
}
