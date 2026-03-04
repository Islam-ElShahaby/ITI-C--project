#include "GpuUsageService.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

namespace service {

GpuUsageService::GpuUsageService()
    : m_lastUsage(0.0f)
{
    std::cout << "[GpuService] Service instance created" << std::endl;
}

void GpuUsageService::requestGpuUsageData(
    const std::shared_ptr<CommonAPI::ClientId> _client,
    requestGpuUsageDataReply_t _reply)
{
    (void)_client;

    float usage = readSystemGpuUsage();

    std::cout << "[GpuService] Request received, returning GPU usage: "
              << usage << "%" << std::endl;

    _reply(usage);
}

float GpuUsageService::readSystemGpuUsage() {
    std::vector<std::string> gpuPaths = {
        "/sys/class/drm/card0/device/gpu_busy_percent",
        "/sys/class/drm/card1/device/gpu_busy_percent",
        "/sys/kernel/debug/dri/0/i915_engine_info"
    };

    for (const auto& path : gpuPaths) {
        std::ifstream file(path);
        if (file.is_open()) {
            float usage = 0.0f;
            file >> usage;
            if (file.good() || file.eof()) {
                m_lastUsage = usage;
                return usage;
            }
        }
    }

    FILE* pipe = popen("nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits 2>/dev/null", "r");
    if (pipe) {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            float usage = std::atof(buffer);
            pclose(pipe);
            m_lastUsage = usage;
            return usage;
        }
        pclose(pipe);
    }

    m_lastUsage = -1.0f;
    return -1.0f;
}

}
