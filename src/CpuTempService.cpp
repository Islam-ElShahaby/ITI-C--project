#include "CpuTempService.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

namespace service {

CpuTempService::CpuTempService() {
    std::cout << "[CpuTempService] Service instance created" << std::endl;
}

void CpuTempService::requestCpuTempData(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    requestCpuTempDataReply_t _reply)
{
    (void)_client;

    float temp = readCpuTemp();

    std::cout << "[CpuTempService] Request received, returning CPU temp: "
              << temp << "°C" << std::endl;

    _reply(temp);
}

float CpuTempService::readCpuTemp() {
    // Try thermal_zone first
    std::vector<std::string> candidates = {
        "/sys/class/thermal/thermal_zone0/temp"
    };

    // Scan hwmon for known CPU temp drivers
    const std::vector<std::string> knownDrivers = {
        "k10temp", "zenpower", "coretemp", "cpu_thermal"
    };
    for (int i = 0; i < 20; ++i) {
        std::string nameFile = "/sys/class/hwmon/hwmon" + std::to_string(i) + "/name";
        std::ifstream nf(nameFile);
        if (!nf.is_open()) continue;
        std::string driverName;
        std::getline(nf, driverName);
        nf.close();
        while (!driverName.empty() && std::isspace(driverName.back()))
            driverName.pop_back();
        for (const auto& drv : knownDrivers) {
            if (driverName == drv) {
                candidates.push_back("/sys/class/hwmon/hwmon" + std::to_string(i) + "/temp1_input");
                break;
            }
        }
    }

    for (const auto& path : candidates) {
        std::ifstream file(path);
        if (file.is_open()) {
            long millideg = 0;
            file >> millideg;
            file.close();
            return static_cast<float>(millideg) / 1000.0f;
        }
    }

    return -1.0f;
}

}
