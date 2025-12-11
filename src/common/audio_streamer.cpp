#include "audio_streamer.hpp"
#include <iostream>
#include <gst/gst.h>
#include <vector>
#include <shared_mutex>

AudioStreamer::AudioStreamer(int port, const std::string &myId)
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

    sessionMgr = std::make_unique<SessionManager>(port, myId);

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
    sessionMgr->start();
}

void AudioStreamer::stop()
{
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
    sessionMgr->setWantToSendTo(peerId, true, inputDevice);
}

void AudioStreamer::stopSendingTo(const std::string &peerId)
{
    sessionMgr->setWantToSendTo(peerId, false);
}

void AudioStreamer::startReceivingFrom(const std::string &peerId, const std::optional<std::string> &outputDeviceUID)
{
    std::optional<AudioDevice> outputDevice = findAudioDeviceByUID(outputDeviceUID);
    sessionMgr->setWantToReceiveFrom(peerId, true, outputDevice);
}

void AudioStreamer::stopReceivingFrom(const std::string &peerId)
{
    sessionMgr->setWantToReceiveFrom(peerId, false, std::nullopt);
}

void AudioStreamer::changeOutputDevice(const std::string &peerId, const std::optional<std::string> &outputDeviceUID)
{
    std::optional<AudioDevice> outputDevice = findAudioDeviceByUID(outputDeviceUID);
    std::scoped_lock lock(audiosMutex);

    auto it = receivers.find(peerId);
    if (it == receivers.end())
        return;

    it->second->updateOutputDevice(outputDevice);
}

void AudioStreamer::changeInputDevice(const std::string &peerId, const std::optional<std::string> &inputDeviceUID)
{
    std::optional<AudioDevice> inputDevice = findAudioDeviceByUID(inputDeviceUID);
    std::scoped_lock lock(audiosMutex);

    auto it = senders.find(peerId);
    if (it == senders.end())
        return;

    it->second->updateInputDevice(inputDevice);
}

std::vector<std::string> AudioStreamer::getPeerIds()
{
    return sessionMgr->getPeerIds();
}

AudioDeviceManager &AudioStreamer::getAudioDeviceManager()
{
    return audioDeviceManager;
}

void AudioStreamer::updateSender(const std::string &peerId, bool shouldSend, const std::string &host, int port, const std::optional<AudioDevice> &inputDevice)
{
    std::scoped_lock lock(audiosMutex);
    auto it = senders.find(peerId);

    if (shouldSend)
    {
        if (it != senders.end())
        {
            std::cout << "[AudioStreamer] Stopping send to peer: " << peerId << std::endl;
            it->second->stop();
            senders.erase(it);
        }

        std::cout << "[AudioStreamer] Starting send to peer: " << peerId << std::endl;
        auto sender = std::make_unique<AudioSender>(host, port, inputDevice);
        sender->setOnStateUpdate(
            [this, peerId](bool started, const std::optional<AudioDevice> &inputDevice)
            {
                sessionMgr->updateSendingState(peerId, started, inputDevice);
            });
        sender->start();
        senders.emplace(peerId, std::move(sender));
    }
    else
    {
        if (it == senders.end())
            return;

        std::cout << "[AudioStreamer] Stopping send to peer: " << peerId << std::endl;
        it->second->stop();
        senders.erase(it);
    }
}

void AudioStreamer::updateReceiver(const std::string &peerId, bool shouldReceive, const std::string &host, const std::optional<AudioDevice> &outputDevice)
{
    std::scoped_lock lock(audiosMutex);
    auto it = receivers.find(peerId);

    if (shouldReceive)
    {
        if (it != receivers.end())
        {
            std::cout << "[AudioStreamer] Stopping receive from peer: " << peerId << std::endl;
            it->second->stop();
            receivers.erase(it);
        }

        std::cout << "[AudioStreamer] Starting receive from peer: " << peerId << std::endl;
        auto receiver = std::make_unique<AudioReceiver>(host, outputDevice);
        receiver->setOnStateUpdate(
            [this, peerId](bool receiving, int receivePort, const std::optional<AudioDevice> &outputDevice)
            {
                sessionMgr->updateReceivingState(peerId, receiving, receivePort, outputDevice);
            });
        receiver->start();
        receivers.emplace(peerId, std::move(receiver));
    }
    else
    {
        if (it == receivers.end())
            return;

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
            return device;
        }
    }
    return std::nullopt;
}
