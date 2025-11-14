#include "audio_streamer.hpp"
#include <iostream>

AudioStreamer::AudioStreamer(int port, const std::string& myId)
    : port(port), myId(myId)
{
    sessionMgr = std::make_unique<SessionManager>(port, myId);
    
    // Register callbacks from SessionManager
    sessionMgr->setOnSendingStateUpdate(
        [this](const std::string& peerId, bool shouldSend) {
            this->onSendingStateUpdate(peerId, shouldSend);
        }
    );
    
    sessionMgr->setOnReceivingStateUpdate(
        [this](const std::string& peerId, bool shouldReceive) {
            this->onReceivingStateUpdate(peerId, shouldReceive);
        }
    );
    
    sessionMgr->setOnConnectionLoss(
        [this](const std::string& peerId) {
            this->onConnectionLoss(peerId);
        }
    );
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

void AudioStreamer::startSendingTo(const std::string& peerId, const std::string& peerHost)
{
    // {
    //     std::scoped_lock lock(audiosMutex);
    //     // Create sender if not exists
    //     if (senders.find(peerId) == senders.end()) {
    //         senders[peerId] = std::make_unique<AudioSender>(peerHost, port);
    //     }
    // }
    
    sessionMgr->setWantToSendTo(peerId, true);
}

void AudioStreamer::stopSendingTo(const std::string& peerId)
{
    sessionMgr->setWantToSendTo(peerId, false);
    
    // {
    //     std::scoped_lock lock(audiosMutex);
    //     auto it = senders.find(peerId);
    //     if (it != senders.end()) {
    //         it->second->stop();
    //         senders.erase(it);
    //     }
    // }
}

void AudioStreamer::startReceivingFrom(const std::string& peerId)
{
    // {
    //     std::scoped_lock lock(audiosMutex);
    //     // Create receiver if not exists
    //     if (receivers.find(peerId) == receivers.end()) {
    //         receivers[peerId] = std::make_unique<AudioReceiver>(port);
    //     }
    // }

    sessionMgr->setWantToReceiveFrom(peerId, true);
}

void AudioStreamer::stopReceivingFrom(const std::string& peerId)
{
    sessionMgr->setWantToReceiveFrom(peerId, false);
    
    // Stop the receiver if active
    // {
    //     std::scoped_lock lock(audiosMutex);
    //     auto it = receivers.find(peerId);
    //     if (it != receivers.end()) {
    //         it->second->stop();
    //         receivers.erase(it);
    //     }
    // }
}

void AudioStreamer::onSendingStateUpdate(const std::string& peerId, bool shouldSend)
{
    std::scoped_lock lock(audiosMutex);
    auto it = senders.find(peerId);
    
    if (shouldSend) {
        // Start sender if not already running
        if (it != senders.end()) {
            std::cout << "[AudioStreamer] Starting send to peer: " << peerId << std::endl;
            it->second->start();
        }
    } else {
        // Stop sender
        if (it != senders.end()) {
            std::cout << "[AudioStreamer] Stopping send to peer: " << peerId << std::endl;
            it->second->stop();
        }
    }
}

void AudioStreamer::onReceivingStateUpdate(const std::string& peerId, bool shouldReceive)
{
    std::scoped_lock lock(audiosMutex);
    auto it = receivers.find(peerId);
    
    if (shouldReceive) {
        // Start receiver if not already running
        if (it != receivers.end()) {
            std::cout << "[AudioStreamer] Starting receive from peer: " << peerId << std::endl;
            it->second->start();
        }
    } else {
        // Stop receiver
        if (it != receivers.end()) {
            std::cout << "[AudioStreamer] Stopping receive from peer: " << peerId << std::endl;
            it->second->stop();
        }
    }
}

void AudioStreamer::onConnectionLoss(const std::string& peerId)
{
    std::cout << "[AudioStreamer] Connection lost with peer: " << peerId << std::endl;
    
    // Clean up resources for this peer
    {
        std::scoped_lock lock(audiosMutex);
        senders.erase(peerId);
        receivers.erase(peerId);
    }
}