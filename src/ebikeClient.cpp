#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <chrono>
#include <iomanip>
#include <sstream>
#include "hal/CSVHALManager.h"
#include "GPSSensor.h"

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* tm_ptr = std::localtime(&now_time);
    std::ostringstream oss;
    oss << "[" << (tm_ptr->tm_year + 1900) << "-"
        << std::setw(2) << std::setfill('0') << (tm_ptr->tm_mon + 1) << "-"
        << std::setw(2) << std::setfill('0') << tm_ptr->tm_mday << " "
        << std::setw(2) << std::setfill('0') << tm_ptr->tm_hour << ":"
        << std::setw(2) << std::setfill('0') << tm_ptr->tm_min << ":"
        << std::setw(2) << std::setfill('0') << tm_ptr->tm_sec << "]";
    return oss.str();
}

int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <csv_file_path> <port_number>" << std::endl;
        return 1;
    }

    std::string csvFilePath = argv[1];
    int portNumber = std::stoi(argv[2]);

    // Create HAL Manager
    CSVHALManager halManager(1); // Initialize with 1 port

    // Create GPS Sensor as a shared pointer
    std::shared_ptr<GPSSensor> gpsSensor = std::make_shared<GPSSensor>();

    try {
        // Attach sensor to HAL
        halManager.attachDevice(portNumber, gpsSensor);
        
        // Initialize the CSV file
        halManager.initialise(csvFilePath);

        // Read and process GPS data
        int readCount = 0;
        while (true) {
            try {
                // Read data from the sensor
                std::vector<uint8_t> reading = halManager.read(portNumber);
                
                // Convert byte vector to string for GPS reading
                std::string readingStr;
                for (uint8_t byte : reading) {
                    readingStr += static_cast<char>(byte);
                }

                // Extract timestamp and coordinates
                size_t delimiterPos = readingStr.find(';');
                if (delimiterPos != std::string::npos) {
                    std::string timestamp = getCurrentTimestamp();
                    std::string coordinates = readingStr.substr(0, delimiterPos) + ", " + readingStr.substr(delimiterPos + 1);
                    
                    std::cout << timestamp << " | GPS: " << coordinates << std::endl;
                } else {
                    std::cerr << "Invalid GPS data format in reading " << readCount << std::endl;
                }
                
                readCount++;
            }
            catch (const std::out_of_range& e) {
                // No more data available
                std::cout << "Reached end of data after " << readCount << " readings." << std::endl;
                break;
            }
        }

        // Release sensor from HAL
        halManager.releaseDevice(portNumber);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
