#include "audio_streamer.hpp"
#include <iostream>
#include <gst/gst.h>

AudioStreamer::AudioStreamer(int port, const std::string &myId)
    : port(port)
{
    gst_init(nullptr, nullptr);

    sessionMgr = std::make_unique<SessionManager>(port, myId);

    // Register callbacks from SessionManager
    sessionMgr->setOnSendRequest(
        [this](const std::string &peerId, bool shouldSend, const std::string &host, int port)
        {
            this->updateSender(peerId, shouldSend, host, port);
        });

    sessionMgr->setOnReceiveRequest(
        [this](const std::string &peerId, bool shouldReceive, const std::string &host)
        {
            this->updateReceiver(peerId, shouldReceive, host);
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

void AudioStreamer::startReceivingFrom(const std::string &peerId)
{
    sessionMgr->setWantToReceiveFrom(peerId, true);
}

void AudioStreamer::stopReceivingFrom(const std::string &peerId)
{
    sessionMgr->setWantToReceiveFrom(peerId, false);
}

void AudioStreamer::updateSender(const std::string &peerId, bool shouldSend, const std::string &host, int port)
{
    std::scoped_lock lock(audiosMutex);
    auto it = senders.find(peerId);

    if (shouldSend)
    {
        if (it != senders.end())
            return;

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

void AudioStreamer::updateReceiver(const std::string &peerId, bool shouldReceive, const std::string &host)
{
    std::scoped_lock lock(audiosMutex);
    auto it = receivers.find(peerId);

    if (shouldReceive)
    {
        if (it != receivers.end())
            return;

        std::cout << "[AudioStreamer] Starting receive from peer: " << peerId << std::endl;
        auto receiver = std::make_unique<AudioReceiver>(host);
        receiver->setOnStateUpdate(
            [this, peerId](bool receiving, int receivePort)
            {
                sessionMgr->updateReceivingState(peerId, receiving, receivePort);
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