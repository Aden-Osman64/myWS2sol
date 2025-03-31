#ifndef GPS_SENSOR_H
#define GPS_SENSOR_H

#include <string>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <vector>
#include <cstdint>
#include "hal/ISensor.h"

class GPSSensor : public ISensor {
private:
    std::string sensorId;
    std::string currentReading;
    
    // Helper function to get current timestamp
    std::string getCurrentTimestamp() {
        std::time_t now = std::time(nullptr);
        std::tm* nowTm = std::localtime(&now);
        
        std::ostringstream oss;
        oss << std::put_time(nowTm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

public:
    GPSSensor(const std::string& id = "GPS_001") : sensorId(id) {}

    // Implement IDevice interface method
    int getId() const override {
        // For the GPS sensor, we'll explicitly return 0 
        // to match the column indices in the generated CSV
        return 0;
    }

    // Implement ISensor interface methods
    int getDimension() const override {
        return 2; // Latitude and Longitude
    }

    std::string format(std::vector<uint8_t> reading) override {
        // Convert byte vector to string
        std::string readingStr(reading.begin(), reading.end());
        
        // Replace ';' with "; " for more readable format
        size_t semicolonPos = readingStr.find(';');
        if (semicolonPos != std::string::npos) {
            readingStr.replace(semicolonPos, 1, "; ");
        }
        
        return getCurrentTimestamp() + " GPS: " + readingStr;
    }

    bool connect(const std::string& reading) {
        // Find the comma separator
        size_t commaPos = reading.find(',');
        
        if (commaPos != std::string::npos) {
            std::string lat = reading.substr(0, commaPos);
            std::string lon = reading.substr(commaPos + 1);
            
            currentReading = getCurrentTimestamp() + " GPS: " + 
                             lat + "; " + lon;
            return true;
        }
        return false;
    }

    std::string read() {
        return currentReading;
    }

    void disconnect() {
        currentReading.clear();
    }
};

#endif // GPS_SENSOR_H