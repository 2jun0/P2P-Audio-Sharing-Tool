#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>
#include <random>
#include "audio_streamer.hpp"

int main(int argc, char *argv[])
{
    int port = 6000; // default discovery/audio port
    std::string peerId;

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

    peerId = (argc > 2) ? argv[2] : makePeerId();

    if (argc <= 1)
    {
        std::cout << "Usage: " << argv[0] << " [port] [peerId]" << std::endl;
        std::cout << "  port: local port for session discovery/audio (default: 6000)" << std::endl;
        std::cout << "  peerId: 6-char hex id (default: random)" << std::endl;
        std::cout << std::endl;
    }

    try
    {
        AudioStreamer streamer(port, peerId);
        std::cout << "[Main] Starting AudioStreamer on port " << port
                  << " with ID: " << peerId << std::endl;
        streamer.start();

        // Interactive mode: commands for testing
        std::cout << "\n=== Commands ===\n"
                  << "send <peerId>             - Start sending to peer\n"
                  << "recv <peerId>             - Start receiving from peer\n"
                  << "stop-send <peerId>        - Stop sending to peer\n"
                  << "stop-recv <peerId>        - Stop receiving from peer\n"
                  << "list                      - Show known peer IDs\n"
                  << "quit                      - Exit\n"
                  << "==================\n\n";

        std::string command;
        while (std::getline(std::cin, command))
        {
            if (command.empty())
                continue;

            // Simple command parsing
            std::istringstream iss(command);
            std::string cmd, peerId;

            iss >> cmd >> peerId;

            if (cmd == "send")
            {
                if (peerId.empty())
                {
                    std::cerr << "Usage: send <peerId>" << std::endl;
                    continue;
                }
                std::cout << "[Main] Requesting to send to peer: " << peerId << std::endl;
                streamer.startSendingTo(peerId);
            }
            else if (cmd == "recv")
            {
                if (peerId.empty())
                {
                    std::cerr << "Usage: recv <peerId>" << std::endl;
                    continue;
                }
                std::cout << "[Main] Requesting to receive from peer: " << peerId << std::endl;
                streamer.startReceivingFrom(peerId, std::nullopt);
            }
            else if (cmd == "stop-send")
            {
                if (peerId.empty())
                {
                    std::cerr << "Usage: stop-send <peerId>" << std::endl;
                    continue;
                }
                std::cout << "[Main] Stopping send to peer: " << peerId << std::endl;
                streamer.stopSendingTo(peerId);
            }
            else if (cmd == "stop-recv")
            {
                if (peerId.empty())
                {
                    std::cerr << "Usage: stop-recv <peerId>" << std::endl;
                    continue;
                }
                std::cout << "[Main] Stopping receive from peer: " << peerId << std::endl;
                streamer.stopReceivingFrom(peerId);
            }
            else if (cmd == "list")
            {
                auto peers = streamer.getPeers();
                if (peers.empty())
                {
                    std::cout << "[Main] No peers discovered yet." << std::endl;
                }
                else
                {
                    std::cout << "[Main] Known peers:" << std::endl;
                    for (const auto &peer : peers)
                        std::cout << "  - " << peer.id << std::endl;
                }
            }
            else if (cmd == "quit")
            {
                std::cout << "[Main] Exiting..." << std::endl;
                break;
            }
            else
            {
                std::cout << "Unknown command: " << cmd << std::endl;
            }
        }

        streamer.stop();
        std::cout << "[Main] AudioStreamer stopped." << std::endl;
    }
    catch (const std::exception &ex)
    {
        std::cerr << "[Error] " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
