#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>
#include "audio_streamer.hpp"

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <port> [sessionId]" << std::endl;
        std::cerr << "  port: local port for session discovery and audio" << std::endl;
        std::cerr << "  sessionId: unique identifier for this instance (default: auto-generated)" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[1]);
    std::string sessionId = (argc > 2) ? argv[2] : "node_" + std::to_string(port);

    try {
        AudioStreamer streamer(port, sessionId);
        std::cout << "[Main] Starting AudioStreamer on port " << port 
                  << " with ID: " << sessionId << std::endl;
        streamer.start();

        // Interactive mode: commands for testing
        std::cout << "\n=== Commands ===\n"
                  << "send <peerId> <peerHost>  - Start sending to peer\n"
                  << "recv <peerId>             - Start receiving from peer\n"
                  << "stop-send <peerId>        - Stop sending to peer\n"
                  << "stop-recv <peerId>        - Stop receiving from peer\n"
                  << "quit                      - Exit\n"
                  << "==================\n\n";

        std::string command;
        while (std::getline(std::cin, command)) {
            if (command.empty()) continue;

            // Simple command parsing
            std::istringstream iss(command);
            std::string cmd, peerId, peerHost;

            iss >> cmd >> peerId >> peerHost;

            if (cmd == "send") {
                if (peerId.empty() || peerHost.empty()) {
                    std::cerr << "Usage: send <peerId> <peerHost>" << std::endl;
                    continue;
                }
                std::cout << "[Main] Requesting to send to peer: " << peerId 
                          << " (" << peerHost << ")" << std::endl;
                streamer.startSendingTo(peerId, peerHost);
            } 
            else if (cmd == "recv") {
                if (peerId.empty()) {
                    std::cerr << "Usage: recv <peerId>" << std::endl;
                    continue;
                }
                std::cout << "[Main] Requesting to receive from peer: " << peerId << std::endl;
                streamer.startReceivingFrom(peerId);
            } 
            else if (cmd == "stop-send") {
                if (peerId.empty()) {
                    std::cerr << "Usage: stop-send <peerId>" << std::endl;
                    continue;
                }
                std::cout << "[Main] Stopping send to peer: " << peerId << std::endl;
                streamer.stopSendingTo(peerId);
            } 
            else if (cmd == "stop-recv") {
                if (peerId.empty()) {
                    std::cerr << "Usage: stop-recv <peerId>" << std::endl;
                    continue;
                }
                std::cout << "[Main] Stopping receive from peer: " << peerId << std::endl;
                streamer.stopReceivingFrom(peerId);
            } 
            else if (cmd == "quit") {
                std::cout << "[Main] Exiting..." << std::endl;
                break;
            } 
            else {
                std::cout << "Unknown command: " << cmd << std::endl;
            }
        }

        streamer.stop();
        std::cout << "[Main] AudioStreamer stopped." << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "[Error] " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}