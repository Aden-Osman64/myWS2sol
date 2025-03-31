#ifndef CSVHALMANAGER_H
#define CSVHALMANAGER_H

#include "ISensor.h"
#include "IActuator.h"
#include <unordered_map>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <vector>

class CSVHALManager {
private:
    std::vector<std::vector<std::string>> data; // CSV data
    std::unordered_map<int, std::shared_ptr<IDevice> > portDeviceMap; // Port ID -> Device (generic pointer)
    size_t sequence; // Current sequence (row index)
    int numPorts; // Total number of ports

    std::vector<uint8_t> convertToByteVector(const std::vector<std::string>& stringVector) {
        std::vector<uint8_t> byteVector;
        for (size_t i = 0; i < stringVector.size(); ++i) {
            for (char ch : stringVector[i]) {
                byteVector.push_back(static_cast<uint8_t>(ch));
            }
            
            // Add separator between coordinates, but not after the last coordinate
            if (i < stringVector.size() - 1) {
                byteVector.push_back(static_cast<uint8_t>(';'));
            }
        }
        return byteVector;
    }

public:
    // Constructor
   CSVHALManager(int numPorts) : sequence(0), numPorts(numPorts) {
            if (numPorts <= 0) {
                throw std::invalid_argument("Number of ports must be greater than 0.");
            }
    }
    
    // Initialise the CSV file
    void initialise(const std::string& filePath) {
        std::ifstream csvFile(filePath);
        if (!csvFile.is_open()) {
            throw std::runtime_error("Failed to open CSV file: " + filePath);
        }

        std::string line;
        
        while (std::getline(csvFile, line)) {
            std::istringstream lineStream(line);
            std::string cell;
            std::vector<std::string> row;

            while (std::getline(lineStream, cell, ',')) {
                row.push_back(cell);
            }
            data.push_back(row);
        }
        csvFile.close();
    }

     // Get the device attached to a port
    std::shared_ptr<IDevice> getDevice(int portId) const {
        if (portDeviceMap.find(portId) == portDeviceMap.end()) {
            throw std::runtime_error("No device attached to port.");
        }
        auto device = portDeviceMap.at(portId);
        return std::dynamic_pointer_cast<IDevice>(device);
    }

    // Attach a device to a port
    void attachDevice(int portId, const std::shared_ptr<IDevice>& device)  {
        if (portId < 0 || portId >= numPorts) {
            throw std::out_of_range("Invalid port ID.");
        }
        if (portDeviceMap.find(portId) != portDeviceMap.end()) {
            throw std::runtime_error("Port is already busy.");
        }
        portDeviceMap[portId] = device;
        std::cout << "[CSVHALManager] Device attached to port " << portId << ".\n";
    }

    // Release a device from a port
    void releaseDevice(int portId)  {
        if (portDeviceMap.find(portId) == portDeviceMap.end()) {
            throw std::runtime_error("No device attached to port.");
        }
        portDeviceMap.erase(portId);
        std::cout << "[CSVHALManager] Device released from port " << portId << ".\n";
    }

    // Check if a port is busy
    bool isBusy(int portId) const  {
        return portDeviceMap.find(portId) != portDeviceMap.end();
    }

    // Read data from a sensor
    std::vector<uint8_t> read(int portId)  {
        // Check if the port has a device attached
        if (portDeviceMap.find(portId) == portDeviceMap.end()) {
            throw std::runtime_error("No device attached to the specified port.");
        }

        // Check if the attached device is a sensor
        auto sensor = std::dynamic_pointer_cast<ISensor>(portDeviceMap[portId]);
        if (!sensor) {
            throw std::runtime_error("The device attached to the port is not a sensor and cannot read data.");
        }

        // Ensure the sequence is within bounds
        if (sequence >= data.size()) {
            throw std::out_of_range("No more data available.");
        }

        // Read the specified columns from the current row
        std::vector<std::string> result;
        
        int columnIndex;

        for (int i = 0; i < sensor->getDimension(); ++i) {
            
            columnIndex = sensor->getId() + i;

            if (columnIndex >= static_cast<int>(data[0].size())) {
                throw std::out_of_range("Column index out of range.");
            }
            result.push_back(data[sequence][columnIndex]);
        }

        sequence++; // Increment sequence after reading
        
        
        return convertToByteVector(result);
    }

    // Write data to an actuator
    void write(int portId, const std::vector<uint8_t>& data) {
        if (portDeviceMap.find(portId) == portDeviceMap.end()) {
            throw std::runtime_error("No device attached to port.");
        }

        auto device = portDeviceMap[portId];

        // Check if the device is an actuator
        auto actuator = std::dynamic_pointer_cast<IActuator>(device);
        if (actuator) {
            actuator->send(data);
        } else {
            throw std::runtime_error("The device attached to the port is not an actuator and cannot send data.");
        }
    }

};
#endif // CSVHALMANAGER_H