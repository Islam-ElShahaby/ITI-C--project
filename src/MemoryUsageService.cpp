#include "MemoryUsageService.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

namespace service {

MemoryUsageService::MemoryUsageService() {
    std::cout << "[MemoryService] Service instance created" << std::endl;
}

void MemoryUsageService::requestMemoryUsageData(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    requestMemoryUsageDataReply_t _reply)
{
    (void)_client;

    float usage = readMemoryUsageMB();

    std::cout << "[MemoryService] Request received, returning Memory usage: "
              << usage << " MB" << std::endl;

    _reply(usage);
}

float MemoryUsageService::readMemoryUsageMB() {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::ifstream file("/proc/meminfo");
    if (!file.is_open()) return -1.0f;

    long memTotal = 0, memAvailable = 0;
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        long value;
        iss >> key >> value;

        if (key == "MemTotal:") memTotal = value;
        else if (key == "MemAvailable:") memAvailable = value;

        if (memTotal > 0 && memAvailable > 0) break;
    }
    file.close();

    if (memTotal == 0) return -1.0f;

    long usedKB = memTotal - memAvailable;
    return static_cast<float>(usedKB) / 1024.0f;
}

}
