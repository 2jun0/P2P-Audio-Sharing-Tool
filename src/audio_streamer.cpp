#include "audio_streamer.hpp"
#include <iostream>
#include <gst/gst.h>
#include <vector>

AudioStreamer::AudioStreamer(int port, const std::string &myId)
    : port(port)
{
    gst_init(nullptr, nullptr);

#if defined(_WIN32)
    audioDeviceManager.setDefaultOutputDeviceChangeCallback(
        [this](AudioDevice device)
        {
            std::scoped_lock lock(audiosMutex);
            for (auto &p : receivers)
            {
                p.second->updateOutputDevice(std::optional<AudioDevice>(device));
            }
        });
#elif defined(__APPLE__)
    audioDeviceManager.setDefaultOutputDeviceChangeCallback(
        [this](AudioDevice device)
        {
            std::scoped_lock lock(audiosMutex);
            for (auto &p : receivers)
            {
                p.second->updateOutputDevice(std::optional<AudioDevice>(device));
            }
        });
#endif

    sessionMgr = std::make_unique<SessionManager>(port, myId);

    // Register callbacks from SessionManager
    sessionMgr->setOnSendRequest(
        [this](const std::string &peerId, bool shouldSend, const std::string &host, int port)
        {
            this->updateSender(peerId, shouldSend, host, port);
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

void AudioStreamer::startSendingTo(const std::string &peerId)
{
    sessionMgr->setWantToSendTo(peerId, true);
}

void AudioStreamer::stopSendingTo(const std::string &peerId)
{
    sessionMgr->setWantToSendTo(peerId, false);
}

void AudioStreamer::startReceivingFrom(const std::string &peerId, const std::optional<AudioDevice> &outputDevice)
{
    sessionMgr->setWantToReceiveFrom(peerId, true, outputDevice);
}

void AudioStreamer::stopReceivingFrom(const std::string &peerId)
{
    sessionMgr->setWantToReceiveFrom(peerId, false, std::nullopt);
}

void AudioStreamer::updateSender(const std::string &peerId, bool shouldSend, const std::string &host, int port)
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
        auto sender = std::make_unique<AudioSender>(host, port);
        sender->setOnStateUpdate(
            [this, peerId](bool started)
            {
                sessionMgr->updateSendingState(peerId, started);
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