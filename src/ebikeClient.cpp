#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include "hal/CSVHALManager.h"
#include "GPSSensor.h"

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

                // Connect sensor with the reading
                if (gpsSensor->connect(readingStr)) {
                    // Print the actual GPS data (timestamp + coordinates)
                    std::cout << gpsSensor->read() << std::endl;
                } else {
                    std::cerr << "Failed to connect reading " << readCount << std::endl;
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
