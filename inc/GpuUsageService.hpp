#pragma once

#include <v1/omnimetron/gpu/GpuUsageDataStubDefault.hpp>
#include <CommonAPI/CommonAPI.hpp>
#include <mutex>
#include <atomic>

namespace service {

class GpuUsageService : public v1::omnimetron::gpu::GpuUsageDataStubDefault {
public:
    GpuUsageService();
    ~GpuUsageService() override = default;

    void requestGpuUsageData(
        const std::shared_ptr<CommonAPI::ClientId> _client,
        requestGpuUsageDataReply_t _reply) override;

private:
    float readSystemGpuUsage();
    std::atomic<float> m_lastUsage;
    std::mutex m_mutex;
};

}
