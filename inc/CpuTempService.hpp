#pragma once

#include <v1/omnimetron/cpu_temp/CpuTempDataStubDefault.hpp>
#include <CommonAPI/CommonAPI.hpp>

namespace service {

class CpuTempService : public v1::omnimetron::cpu_temp::CpuTempDataStubDefault {
public:
    CpuTempService();
    ~CpuTempService() override = default;

    void requestCpuTempData(
        const std::shared_ptr<CommonAPI::ClientId> _client,
        requestCpuTempDataReply_t _reply) override;

private:
    float readCpuTemp();
};

}
