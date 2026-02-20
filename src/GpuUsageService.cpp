#include "GpuUsageService.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
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
    (void)_client; // The client ID isn't used here; we reply to whoever called us

    float usage = getCurrentGpuUsage();
    
    std::cout << "[GpuService] Request received, returning GPU usage: " 
              << usage << "%" << std::endl;
    
    // Send the GPU usage value back to the client that called
    _reply(usage);
}

void GpuUsageService::broadcastGpuUsage(float usage) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastUsage = usage;
    
    // Publish the new value so any subscribed clients receive it immediately
    fireNotifyGpuUsageDataChangeEvent(usage);
    
    std::cout << "[GpuService] Broadcast sent: GPU usage = " << usage << "%" << std::endl;
}

float GpuUsageService::getCurrentGpuUsage() {
    return readSystemGpuUsage();
}

float GpuUsageService::readSystemGpuUsage() {
    // Try each known sysfs path where AMD/Intel GPUs expose their usage
    std::vector<std::string> gpuPaths = {
        "/sys/class/drm/card0/device/gpu_busy_percent",      // AMD (primary GPU)
        "/sys/class/drm/card1/device/gpu_busy_percent",      // AMD (secondary GPU)
        "/sys/kernel/debug/dri/0/i915_engine_info"           // Intel (needs parsing)
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

    // If sysfs didn't work, try nvidia-smi for NVIDIA GPUs
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

    // No real GPU found. Report as unavailable (-1.0f).
    m_lastUsage = -1.0f;
    return -1.0f;
}

} // namespace service
