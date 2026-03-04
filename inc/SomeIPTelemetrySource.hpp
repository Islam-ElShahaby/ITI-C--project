#pragma once

#include "ITelemetrySource.hpp"
#include <CommonAPI/CommonAPI.hpp>
#include <v1/omnimetron/gpu/GpuUsageDataProxy.hpp>
#include <v1/omnimetron/cpu/CpuUsageDataProxy.hpp>
#include <v1/omnimetron/memory/MemoryUsageDataProxy.hpp>
#include <v1/omnimetron/cpu_temp/CpuTempDataProxy.hpp>
#include <string>
#include <memory>
#include <mutex>
#include <atomic>

namespace telemetry {

// Generic SomeIP adapter for the GPU service
class SomeIPGpuSource : public ITelemetrySource {
public:
    SomeIPGpuSource() = default;
    bool openSource() override;
    bool readSource(std::string& out) override;
private:
    std::shared_ptr<CommonAPI::Runtime> m_runtime;
    std::shared_ptr<v1::omnimetron::gpu::GpuUsageDataProxy<>> m_proxy;
    std::mutex m_mutex;
    std::atomic<bool> m_initialized{false};
};

// Generic SomeIP adapter for the CPU service
class SomeIPCpuSource : public ITelemetrySource {
public:
    SomeIPCpuSource() = default;
    bool openSource() override;
    bool readSource(std::string& out) override;
private:
    std::shared_ptr<CommonAPI::Runtime> m_runtime;
    std::shared_ptr<v1::omnimetron::cpu::CpuUsageDataProxy<>> m_proxy;
    std::mutex m_mutex;
    std::atomic<bool> m_initialized{false};
};

// Generic SomeIP adapter for the Memory service
class SomeIPMemorySource : public ITelemetrySource {
public:
    SomeIPMemorySource() = default;
    bool openSource() override;
    bool readSource(std::string& out) override;
private:
    std::shared_ptr<CommonAPI::Runtime> m_runtime;
    std::shared_ptr<v1::omnimetron::memory::MemoryUsageDataProxy<>> m_proxy;
    std::mutex m_mutex;
    std::atomic<bool> m_initialized{false};
};

// Generic SomeIP adapter for the CPU Temperature service
class SomeIPCpuTempSource : public ITelemetrySource {
public:
    SomeIPCpuTempSource() = default;
    bool openSource() override;
    bool readSource(std::string& out) override;
private:
    std::shared_ptr<CommonAPI::Runtime> m_runtime;
    std::shared_ptr<v1::omnimetron::cpu_temp::CpuTempDataProxy<>> m_proxy;
    std::mutex m_mutex;
    std::atomic<bool> m_initialized{false};
};

// Backward-compatible alias no need to change everything
using SomeIPTelemetrySourceAdapter = SomeIPGpuSource;

}
