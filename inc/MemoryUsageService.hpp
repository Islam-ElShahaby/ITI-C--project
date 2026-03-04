#pragma once

#include <v1/omnimetron/memory/MemoryUsageDataStubDefault.hpp>
#include <CommonAPI/CommonAPI.hpp>
#include <mutex>

namespace service {

class MemoryUsageService : public v1::omnimetron::memory::MemoryUsageDataStubDefault {
public:
    MemoryUsageService();
    ~MemoryUsageService() override = default;

    void requestMemoryUsageData(
        const std::shared_ptr<CommonAPI::ClientId> _client,
        requestMemoryUsageDataReply_t _reply) override;

private:
    float readMemoryUsageMB();
    std::mutex m_mutex;
};

}
