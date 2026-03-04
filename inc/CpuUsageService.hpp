#pragma once

#include <v1/omnimetron/cpu/CpuUsageDataStubDefault.hpp>
#include <CommonAPI/CommonAPI.hpp>
#include <mutex>

namespace service {

class CpuUsageService : public v1::omnimetron::cpu::CpuUsageDataStubDefault {
public:
    CpuUsageService();
    ~CpuUsageService() override = default;

    void requestCpuUsageData(
        const std::shared_ptr<CommonAPI::ClientId> _client,
        requestCpuUsageDataReply_t _reply) override;

private:
    float readCpuUsage();

    long m_prevIdle = 0;
    long m_prevTotal = 0;
    std::mutex m_mutex;
};

}
