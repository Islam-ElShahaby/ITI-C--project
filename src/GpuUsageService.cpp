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
    (void)_client; // Unused parameter

    float usage = getCurrentGpuUsage();
    
    std::cout << "[GpuService] Request received, returning GPU usage: " 
              << usage << "%" << std::endl;
    
    // Send the reply back to the client
    _reply(usage);
}

void GpuUsageService::broadcastGpuUsage(float usage) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastUsage = usage;
    
    // Fire the broadcast event
    fireNotifyGpuUsageDataChangeEvent(usage);
    
    std::cout << "[GpuService] Broadcast sent: GPU usage = " << usage << "%" << std::endl;
}

float GpuUsageService::getCurrentGpuUsage() {
    return readSystemGpuUsage();
}

float GpuUsageService::readSystemGpuUsage() {
    // Try to read actual GPU usage from various sources
    
    // 1. Try AMD/Intel DRM (Linux)
    std::vector<std::string> gpuPaths = {
        "/sys/class/drm/card0/device/gpu_busy_percent",      // AMD
        "/sys/class/drm/card1/device/gpu_busy_percent",      // AMD (secondary)
        "/sys/kernel/debug/dri/0/i915_engine_info"           // Intel (requires parsing)
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

    // 2. Try nvidia-smi (NVIDIA GPUs)
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

    // 3. Fallback: Generate simulated GPU usage
    // This provides a realistic-looking varying load for testing
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> dist(10.0f, 80.0f);
    
    float baseUsage = m_lastUsage > 0 ? m_lastUsage.load() : 30.0f;
    float variation = (dist(gen) - 45.0f) * 0.2f;  // Small variation around current value
    float newUsage = std::max(0.0f, std::min(100.0f, baseUsage + variation));
    
    m_lastUsage = newUsage;
    return newUsage;
}

} // namespace service
