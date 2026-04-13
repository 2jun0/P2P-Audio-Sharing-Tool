#include "stream_manager.hpp"
#include <iostream>
#include <gst/gst.h>

StreamManager::StreamManager()
{
    gst_init(nullptr, nullptr);
}

StreamManager::~StreamManager()
{
    removeAllSenders();
    removeAllReceivers();
}

void StreamManager::addSender(const std::string &id, const std::string &targetHost, int targetPort, const std::optional<AudioDevice> &inputDevice)
{
    std::scoped_lock lock(mutex);

    auto it = senders.find(id);
    if (it != senders.end())
    {
        std::cout << "[StreamManager] Replacing sender id=" << id << std::endl;
        it->second->stop();
        senders.erase(it);
    }

    std::cout << "[StreamManager] Adding sender id=" << id << " target=" << targetHost << ":" << targetPort << std::endl;
    auto sender = std::make_unique<AudioSender>(targetHost, targetPort, inputDevice);
    sender->start();
    senders.emplace(id, std::move(sender));
}

void StreamManager::removeSender(const std::string &id)
{
    std::scoped_lock lock(mutex);

    auto it = senders.find(id);
    if (it == senders.end())
        return;

    std::cout << "[StreamManager] Removing sender id=" << id << std::endl;
    it->second->stop();
    senders.erase(it);
}

void StreamManager::removeAllSenders()
{
    std::scoped_lock lock(mutex);
    for (auto &kv : senders)
        kv.second->stop();
    senders.clear();
}

void StreamManager::addReceiver(const std::string &id, const std::optional<AudioDevice> &outputDevice)
{
    std::scoped_lock lock(mutex);

    auto it = receivers.find(id);
    if (it != receivers.end())
    {
        std::cout << "[StreamManager] Replacing receiver id=" << id << std::endl;
        it->second->stop();
        receivers.erase(it);
    }

    std::cout << "[StreamManager] Adding receiver id=" << id << std::endl;
    auto receiver = std::make_unique<AudioReceiver>(0, outputDevice);
    receiver->start();
    std::cout << "[StreamManager] Receiver id=" << id << " listening on port=" << receiver->getPort() << std::endl;
    receivers.emplace(id, std::move(receiver));
}

void StreamManager::removeReceiver(const std::string &id)
{
    std::scoped_lock lock(mutex);

    auto it = receivers.find(id);
    if (it == receivers.end())
        return;

    std::cout << "[StreamManager] Removing receiver id=" << id << std::endl;
    it->second->stop();
    receivers.erase(it);
}

void StreamManager::removeAllReceivers()
{
    std::scoped_lock lock(mutex);
    for (auto &kv : receivers)
        kv.second->stop();
    receivers.clear();
}

int StreamManager::getReceiverPort(const std::string &id)
{
    std::shared_lock lock(mutex);
    auto it = receivers.find(id);
    if (it == receivers.end())
        return -1;
    return it->second->getPort();
}

bool StreamManager::hasSender(const std::string &id)
{
    std::shared_lock lock(mutex);
    return senders.count(id) > 0;
}

bool StreamManager::hasReceiver(const std::string &id)
{
    std::shared_lock lock(mutex);
    return receivers.count(id) > 0;
}

#if defined(__ANDROID__)
void StreamManager::submitCapturedAudio(const int16_t *pcmFrames, size_t frameCount, int sampleRate, int channelCount)
{
    if (!pcmFrames || frameCount == 0 || sampleRate <= 0 || channelCount <= 0)
        return;

    std::shared_lock lock(mutex);
    for (auto &entry : senders)
        entry.second->pushPcmFrame(pcmFrames, frameCount, sampleRate, channelCount);
}
#endif
