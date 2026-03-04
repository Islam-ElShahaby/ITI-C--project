#include "CpuUsageService.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

namespace service {

CpuUsageService::CpuUsageService() {
    std::cout << "[CpuService] Service instance created" << std::endl;
}

void CpuUsageService::requestCpuUsageData(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    requestCpuUsageDataReply_t _reply)
{
    (void)_client;

    float usage = readCpuUsage();

    std::cout << "[CpuService] Request received, returning CPU usage: "
              << usage << "%" << std::endl;

    _reply(usage);
}

float CpuUsageService::readCpuUsage() {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::ifstream file("/proc/stat");
    if (!file.is_open()) return -1.0f;

    std::string line;
    std::getline(file, line);
    file.close();

    std::istringstream iss(line);
    std::string cpu;
    iss >> cpu;

    std::vector<long> times;
    long val;
    while (iss >> val) times.push_back(val);

    if (times.size() < 4) return -1.0f;

    long idle = times[3];
    if (times.size() > 4) idle += times[4];

    long total = 0;
    for (long t : times) total += t;

    long diffIdle = idle - m_prevIdle;
    long diffTotal = total - m_prevTotal;

    m_prevIdle = idle;
    m_prevTotal = total;

    if (diffTotal == 0) return 0.0f;

    float usage = 100.0f * (1.0f - static_cast<float>(diffIdle) / static_cast<float>(diffTotal));
    return usage;
}

}
