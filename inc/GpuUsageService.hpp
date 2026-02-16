#pragma once

#include <v1/omnimetron/gpu/GpuUsageDataStubDefault.hpp>
#include <CommonAPI/CommonAPI.hpp>
#include <memory>
#include <atomic>
#include <mutex>

namespace service {

/**
 * @brief GPU Usage Data Service Implementation
 * 
 * Extends the generated GpuUsageDataStubDefault to provide actual GPU usage data.
 * Supports both method request/response and broadcast events.
 * 
 * Design Pattern: Template Method (from generated base class)
 * - Inherits default implementations from GpuUsageDataStubDefault
 * - Overrides requestGpuUsageData() to provide actual functionality
 */
class GpuUsageService : public v1::omnimetron::gpu::GpuUsageDataStubDefault {
public:
    GpuUsageService();
    ~GpuUsageService() override = default;

    /**
     * @brief Handle requestGpuUsageData method calls from clients
     * @param _client Client identifier
     * @param _reply Reply callback function
     */
    void requestGpuUsageData(
        const std::shared_ptr<CommonAPI::ClientId> _client,
        requestGpuUsageDataReply_t _reply) override;

    /**
     * @brief Fire a broadcast event with current GPU usage
     * @param usage Current GPU usage percentage
     */
    void broadcastGpuUsage(float usage);

    /**
     * @brief Get current GPU usage by reading system info
     * @return GPU usage percentage (0-100)
     */
    float getCurrentGpuUsage();

private:
    std::atomic<float> m_lastUsage;
    std::mutex m_mutex;
    
    /**
     * @brief Read GPU usage from system files
     * @return GPU usage percentage, or simulated value if unavailable
     */
    float readSystemGpuUsage();
};

} // namespace service
