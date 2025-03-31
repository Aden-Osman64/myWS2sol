#ifndef SOCKETSERVER_H
#define SOCKETSERVER_H

#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <csignal>
#include <atomic>
#include <arpa/inet.h>
#include "sim/socket.h"
#include "sim/in.h"
#include "MessageHandler.h"
#include <Poco/JSON/Array.h>

class SocketServer {
public:
    SocketServer(Poco::JSON::Array::Ptr& ebikes, int port = 8081) 
        : _ebikes(ebikes), _port(port), _running(false), _messageHandler(ebikes) {
    }

    ~SocketServer() {
        stop();
    }

    void start() {
        if (_running) {
            std::cout << "Server is already running" << std::endl;
            return;
        }

        _running = true;
        _serverThread = std::thread(&SocketServer::serverLoop, this);
    }

    void stop() {
        if (!_running) {
            return;
        }

        _running = false;

        if (_serverThread.joinable()) {
            _serverThread.join();
        }

        if (_serverSocket) {
            delete _serverSocket;
            _serverSocket = nullptr;
        }

        std::cout << "Socket server stopped" << std::endl;
    }

private:
    Poco::JSON::Array::Ptr& _ebikes;
    int _port;
    std::atomic<bool> _running;
    std::thread _serverThread;
    sim::socket* _serverSocket = nullptr;
    MessageHandler _messageHandler;

    void serverLoop() {
        try {
            // Create the server socket
            _serverSocket = new sim::socket(AF_INET, SOCK_DGRAM, 0);

            // Prepare server address
            struct sockaddr_in serverAddr;
            memset(&serverAddr, 0, sizeof(serverAddr));
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(_port);
            serverAddr.sin_addr.s_addr = INADDR_ANY;

            // Bind the socket to the server address
            _serverSocket->bind(serverAddr);

            std::cout << "Socket server running on port " << _port << " and waiting for messages..." << std::endl;

            // Buffer for receiving messages
            char buffer[1024];
            struct sockaddr_in clientAddr;

            while (_running) {
                // Reset buffer
                memset(buffer, 0, sizeof(buffer));

                // Receive message
                ssize_t bytesReceived = _serverSocket->recvfrom(buffer, sizeof(buffer) - 1, 0, clientAddr);
                
                if (bytesReceived > 0) {
                    buffer[bytesReceived] = '\0'; // Null-terminate the message
                    
                    // Get client IP and port
                    char clientIp[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, sizeof(clientIp));
                    uint16_t clientPort = ntohs(clientAddr.sin_port);
                    
                    // Handle the message
                    const char* response = _messageHandler.handleMessage(buffer, clientIp, clientPort);
                    
                    // Send the response back to the client
                    _messageHandler.sendResponse(_serverSocket, response, clientAddr);
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Socket server error: " << e.what() << std::endl;
        }
    }
};

#endif // SOCKETSERVER_H