#include <iostream>
#include <string>
#include <sstream>
#include <random>
#include "lookup_service.hpp"
#include "signaling_service.hpp"
#include "stream_manager.hpp"
#include "audio_link_core_ffi.h"

int main(int argc, char *argv[])
{
    int port = 6000;

    if (argc > 1)
    {
        try
        {
            port = std::stoi(argv[1]);
        }
        catch (const std::exception &)
        {
            std::cerr << "Invalid port argument: " << argv[1] << std::endl;
            return 1;
        }
    }

    auto makePeerId = []()
    {
        static constexpr char alphabet[] = "0123456789abcdef";
        std::string id(6, '0');
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(0, 15);
        for (char &c : id)
            c = alphabet[dist(gen)];
        return id;
    };

    const std::string peerId = (argc > 2) ? argv[2] : makePeerId();
    const std::string myName = (argc > 3) ? argv[3] : peerId;
    const std::string myType = (argc > 4) ? argv[4] : "Others";

    if (argc <= 1)
    {
        std::cout << "Usage: " << argv[0] << " [port] [peerId] [name] [type]" << std::endl;
        std::cout << "  port   : discovery port (default: 6000)" << std::endl;
        std::cout << "  peerId : 6-char hex id (default: random)" << std::endl;
        std::cout << std::endl;
    }

    try
    {
        alc_gst_init();

        LookupService discovery(port, peerId, myName, myType);
        StreamManager streams;

        SignalingService signaling(
            port + 1, peerId,
            [&](const std::string &fromId, const std::string &fromAddr) -> int
            {
                std::cout << "\n[Main] Offer from " << fromId << " (" << fromAddr << "), auto-accepting..." << std::endl;
                streams.addReceiver(fromId);
                int recvPort = streams.getReceiverPort(fromId);
                std::cout << "[Main] Created receiver for " << fromId << " on port " << recvPort << std::endl;
                return recvPort;
            },
            [&](const std::string &fromId, const std::string &fromAddr, int acceptPort)
            {
                std::cout << "\n[Main] Offer accepted by " << fromId << " (" << fromAddr << ":" << acceptPort << ")" << std::endl;
                streams.addSender(fromId, fromAddr, acceptPort);
                std::cout << "[Main] Created sender to " << fromId << " at " << fromAddr << ":" << acceptPort << std::endl;
            },
            [&](const std::string &disconnectedPeerId)
            {
                std::cout << "\n[Main] Peer " << disconnectedPeerId << " disconnected, cleaning up streams" << std::endl;
                streams.removeSender(disconnectedPeerId);
                streams.removeReceiver(disconnectedPeerId);
            });

        std::cout << "[Main] Starting discovery on port " << port << " with ID: " << peerId << std::endl;
        discovery.start();
        signaling.start();

        int sendCounter = 0;
        int recvCounter = 0;

        std::cout << "\n=== Commands ===\n"
                  << "offer <peer_address>      - Send offer to peer (auto sender on accept)\n"
                  << "send <ip> <port>          - Start sending audio to ip:port directly\n"
                  << "recv                      - Start receiving audio (auto port)\n"
                  << "\n=== Commands ===\n"
                  << "stop-send <id>            - Stop a sender by id\n"
                  << "stop-recv <id>            - Stop a receiver by id\n"
                  << "stop-all                  - Stop all senders and receivers\n"
                  << "list                      - Show discovered peers\n"
                  << "quit                      - Exit\n"
                  << "==================\n\n";

        std::string line;
        while (std::getline(std::cin, line))
        {
            if (line.empty())
                continue;

            std::istringstream iss(line);
            std::string cmd;
            iss >> cmd;

            if (cmd == "offer")
            {
                std::string peerAddr;
                iss >> peerAddr;
                if (peerAddr.empty())
                {
                    std::cerr << "Usage: offer <peer_address>" << std::endl;
                    continue;
                }
                signaling.sendOffer(peerAddr);
            }
            else if (cmd == "send")
            {
                std::string ip;
                int targetPort = 0;
                iss >> ip >> targetPort;
                if (ip.empty() || targetPort <= 0)
                {
                    std::cerr << "Usage: send <ip> <port>" << std::endl;
                    continue;
                }

                std::string id = "s" + std::to_string(++sendCounter);
                streams.addSender(id, ip, targetPort);
                std::cout << "[Main] Sending audio to " << ip << ":" << targetPort << " (id=" << id << ")" << std::endl;
            }
            else if (cmd == "recv")
            {
                std::string id = "r" + std::to_string(++recvCounter);
                streams.addReceiver(id);
                std::cout << "[Main] Receiving audio on port " << streams.getReceiverPort(id) << " (id=" << id << ")" << std::endl;
            }
            else if (cmd == "stop-send")
            {
                std::string id;
                iss >> id;
                if (id.empty())
                {
                    std::cerr << "Usage: stop-send <id>" << std::endl;
                    continue;
                }
                streams.removeSender(id);
                std::cout << "[Main] Stopped sender " << id << std::endl;
            }
            else if (cmd == "stop-recv")
            {
                std::string id;
                iss >> id;
                if (id.empty())
                {
                    std::cerr << "Usage: stop-recv <id>" << std::endl;
                    continue;
                }
                streams.removeReceiver(id);
                std::cout << "[Main] Stopped receiver " << id << std::endl;
            }
            else if (cmd == "stop-all")
            {
                streams.removeAllSenders();
                streams.removeAllReceivers();
                std::cout << "[Main] Stopped all streams" << std::endl;
            }
            else if (cmd == "list")
            {
                auto peers = discovery.getPeers();
                if (peers.empty())
                {
                    std::cout << "[Main] No peers discovered yet." << std::endl;
                }
                else
                {
                    std::cout << "[Main] Discovered peers:" << std::endl;
                    for (const auto &peer : peers)
                        std::cout << "  - " << peer.id << " (" << peer.name << ") " << peer.address << std::endl;
                }
            }
            else if (cmd == "quit")
            {
                break;
            }
            else
            {
                std::cout << "Unknown command: " << cmd << std::endl;
            }
        }

        discovery.stop();
        signaling.stop();
        std::cout << "[Main] Exited." << std::endl;
    }
    catch (const std::exception &ex)
    {
        std::cerr << "[Error] " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
