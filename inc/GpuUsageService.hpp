#pragma once

#include <v1/omnimetron/gpu/GpuUsageDataStubDefault.hpp>
#include <CommonAPI/CommonAPI.hpp>
#include <memory>
#include <atomic>
#include <mutex>

namespace service {

class GpuUsageService : public v1::omnimetron::gpu::GpuUsageDataStubDefault {
public:
    GpuUsageService();
    ~GpuUsageService() override = default;

    void requestGpuUsageData(
        const std::shared_ptr<CommonAPI::ClientId> _client,
        requestGpuUsageDataReply_t _reply) override;

    void broadcastGpuUsage(float usage);

    float getCurrentGpuUsage();

private:
    std::atomic<float> m_lastUsage;
    std::mutex m_mutex;
    
    float readSystemGpuUsage();
};

} // namespace service
