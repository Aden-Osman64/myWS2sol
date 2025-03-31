#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H

#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Array.h>
#include <Poco/Dynamic/Var.h>
#include "sim/socket.h"

class MessageHandler {
public:
    MessageHandler(Poco::JSON::Array::Ptr& ebikes) : _ebikes(ebikes) {}

    // Handle incoming messages and return an appropriate response
    const char* handleMessage(const char* message, const char* clientIp, uint16_t clientPort) {
        std::cout << "Handling message from " << clientIp << ":" << clientPort << " - " << message << std::endl;
        
        try {
            // Parse the incoming JSON message
            Poco::JSON::Parser parser;
            Poco::Dynamic::Var result = parser.parse(message);
            Poco::JSON::Object::Ptr jsonObject = result.extract<Poco::JSON::Object::Ptr>();
            
            // Check if this is a position update
            if (jsonObject->has("type") && jsonObject->getValue<std::string>("type") == "position") {
                processPositionUpdate(jsonObject, clientIp);
                return "OK";
            }
            
            // Check if this is a maintenance request
            if (jsonObject->has("type") && jsonObject->getValue<std::string>("type") == "maintenance") {
                return processMaintenanceRequest(jsonObject);
            }
            
            // Unknown message type
            return "ERROR: Unknown message type";
        } catch (const std::exception& e) {
            std::cerr << "Error parsing message: " << e.what() << std::endl;
            return "ERROR: Invalid message format";
        }
    }

    void sendResponse(sim::socket* serverSocket, const char* response, const struct sockaddr_in& clientAddr) {
        ssize_t sent = serverSocket->sendto(response, strlen(response), 0, clientAddr);

        // Print the response sent to the client
        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, sizeof(clientIp));
        uint16_t clientPort = ntohs(clientAddr.sin_port);

        std::cout << "Server: " << response << " sent to client: " << clientIp << ":" << clientPort << std::endl;

        if (sent > 0) {
            std::cout << "Response sent to client: " << response << std::endl;
        }
    }

private:
    Poco::JSON::Array::Ptr& _ebikes;

    // Process position update from an eBike
    void processPositionUpdate(Poco::JSON::Object::Ptr& jsonObject, const char* clientIp) {
        int id = jsonObject->getValue<int>("id");
        double lat = jsonObject->getValue<double>("lat");
        double lon = jsonObject->getValue<double>("lon");
        std::string status = jsonObject->has("status") ? 
            jsonObject->getValue<std::string>("status") : "unlocked";
        
        // Create timestamp
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::localtime(&now_time_t);
        char timeBuffer[26];
        strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", now_tm);
        
        // Create the GeoJSON Feature for the eBike
        Poco::JSON::Object::Ptr feature = new Poco::JSON::Object;
        Poco::JSON::Object::Ptr geometry = new Poco::JSON::Object;
        Poco::JSON::Array::Ptr coordinates = new Poco::JSON::Array;
        Poco::JSON::Object::Ptr properties = new Poco::JSON::Object;
        
        // Set the geometry (Point with coordinates)
        geometry->set("type", "Point");
        coordinates->add(lon);
        coordinates->add(lat);
        geometry->set("coordinates", coordinates);
        
        // Set the properties
        properties->set("id", id);
        properties->set("status", status);
        properties->set("timestamp", std::string(timeBuffer));
        
        // Assemble the feature
        feature->set("type", "Feature");
        feature->set("geometry", geometry);
        feature->set("properties", properties);
        
        // Update or add the eBike to the array
        bool updated = false;
        for (int i = 0; i < _ebikes->size(); i++) {
            Poco::JSON::Object::Ptr obj = _ebikes->getObject(i);
            Poco::JSON::Object::Ptr props = obj->getObject("properties");
            
            if (props->getValue<int>("id") == id) {
                // Update existing eBike
                _ebikes->set(i, feature);
                updated = true;
                break;
            }
        }
        
        // If no existing eBike was found, add a new one
        if (!updated) {
            _ebikes->add(feature);
        }
        
        std::cout << "Updated eBike ID " << id << " at " << lat << ", " << lon << 
            " with status " << status << std::endl;
    }

    // Process maintenance request
    const char* processMaintenanceRequest(Poco::JSON::Object::Ptr& jsonObject) {
        if (!jsonObject->has("id")) {
            return "ERROR: Missing eBike ID";
        }
        
        int id = jsonObject->getValue<int>("id");
        std::string action = jsonObject->has("action") ? 
            jsonObject->getValue<std::string>("action") : "";
        
        if (action == "lock") {
            // Process lock request
            updateEBikeStatus(id, "locked");
            return "OK: eBike locked";
        } else if (action == "unlock") {
            // Process unlock request
            updateEBikeStatus(id, "unlocked");
            return "OK: eBike unlocked";
        } else {
            return "ERROR: Unknown maintenance action";
        }
    }

    // Update eBike status in the array
    void updateEBikeStatus(int id, const std::string& status) {
        for (int i = 0; i < _ebikes->size(); i++) {
            Poco::JSON::Object::Ptr feature = _ebikes->getObject(i);
            Poco::JSON::Object::Ptr properties = feature->getObject("properties");
            
            if (properties->getValue<int>("id") == id) {
                properties->set("status", status);
                
                // Update timestamp
                auto now = std::chrono::system_clock::now();
                auto now_time_t = std::chrono::system_clock::to_time_t(now);
                std::tm* now_tm = std::localtime(&now_time_t);
                char timeBuffer[26];
                strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", now_tm);
                properties->set("timestamp", std::string(timeBuffer));
                
                std::cout << "Updated eBike ID " << id << " status to " << status << std::endl;
                break;
            }
        }
    }
};

#endif // MESSAGEHANDLER_H