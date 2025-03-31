#include "web/WebServer.h"
#include "hal/CSVHALManager.h"
#include "GPSSensor.h"
#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <mutex>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/Dynamic/Var.h>

std::mutex ebikesMutex;

void updateEbikeData(Poco::JSON::Array::Ptr ebikes, CSVHALManager& halManager, std::shared_ptr<GPSSensor> gpsSensor) {
    while (true) {
        try {
            // Read GPS data from the HAL manager
            std::vector<uint8_t> gpsData = halManager.read(0);
            std::string formattedData = gpsSensor->format(gpsData);
            
            // Parse GPS data
            std::string dataStr(gpsData.begin(), gpsData.end());
            std::string::size_type pos = dataStr.find(';');
            if (pos != std::string::npos) {
                std::string lat = dataStr.substr(0, pos);
                std::string lon = dataStr.substr(pos + 1);
                
                // Current timestamp for the reading
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
                coordinates->add(std::stod(lon));
                coordinates->add(std::stod(lat));
                geometry->set("coordinates", coordinates);
                
                // Set the properties
                properties->set("id", 1);
                properties->set("status", "unlocked");
                properties->set("timestamp", std::string(timeBuffer));
                
                // Assemble the feature
                feature->set("type", "Feature");
                feature->set("geometry", geometry);
                feature->set("properties", properties);
                
                // Lock the shared ebikes array while updating
                {
                    std::lock_guard<std::mutex> lock(ebikesMutex);
                    
                    // Check if we have an existing ebike to update
                    bool updated = false;
                    for (int i = 0; i < ebikes->size(); i++) {
                        Poco::JSON::Object::Ptr obj = ebikes->getObject(i);
                        Poco::JSON::Object::Ptr props = obj->getObject("properties");
                        
                        if (props->getValue<int>("id") == 1) {
                            // Update existing ebike
                            ebikes->set(i, feature);
                            updated = true;
                            break;
                        }
                    }
                    
                    // If no existing ebike was found, add a new one
                    if (!updated) {
                        ebikes->add(feature);
                    }
                }
                
                std::cout << formattedData << std::endl;
            }
        } catch (const std::exception& ex) {
            std::cerr << "Error updating eBike data: " << ex.what() << std::endl;
        }
        
        // Sleep for a short period before reading the next data point
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

int main() {
    // Create a reference to a Poco JSON array to store the ebike objects
    Poco::JSON::Array::Ptr ebikes = new Poco::JSON::Array();
    
    try {
        // Create HAL Manager with 1 port for the GPS sensor
        CSVHALManager halManager(1);
        
        // Initialize with the CSV data file (should be in the data directory)
        halManager.initialise("data/sim-eBike-1.csv");
        
        // Create and attach a GPS sensor to port 0
        auto gpsSensor = std::make_shared<GPSSensor>("GPS_001");
        halManager.attachDevice(0, gpsSensor);
        
        std::cout << "Device attached to port 0." << std::endl;
        
        // Replace 0 with your allocated port as per specifications
        int port = 8080;
        
        // Create instance of the server class
        WebServer webServer(ebikes);
        
        // Start the data update thread
        std::thread updateThread(updateEbikeData, ebikes, std::ref(halManager), gpsSensor);
        updateThread.detach();
        
        // Start the web server
        std::cout << "Starting web server on port " << port << std::endl;
        webServer.start(port);
        
        return 0;
    } catch (const Poco::Exception& ex) {
        std::cerr << "Server error (Poco): " << ex.displayText() << std::endl;
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "Server error: " << ex.what() << std::endl;
        return 1;
    }
    
    return 0;
}