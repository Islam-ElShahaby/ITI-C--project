#include "SomeIPTelemetrySource.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>

namespace telemetry {

// wait for proxy availability
template<typename ProxyT>
static bool waitForProxy(std::shared_ptr<ProxyT>& proxy, uint32_t timeoutMs = 3000) {
    if (!proxy) return false;
    if (proxy->isAvailable()) return true;

    auto start = std::chrono::steady_clock::now();
    while (true) {
        if (proxy->isAvailable()) return true;
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (static_cast<uint32_t>(elapsed) >= timeoutMs) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// SomeIPGpuSource

bool SomeIPGpuSource::openSource() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized && m_proxy && m_proxy->isAvailable()) return true;

    try {
        m_runtime = CommonAPI::Runtime::get();
        if (!m_runtime) return false;

        m_proxy = m_runtime->buildProxy<v1::omnimetron::gpu::GpuUsageDataProxy>(
            "local", "omnimetron.gpu.GpuUsageData");
        if (!m_proxy) return false;

        m_initialized = true;
        return waitForProxy(m_proxy);
    } catch (...) {
        return false;
    }
}

bool SomeIPGpuSource::readSource(std::string& out) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_proxy || !m_proxy->isAvailable()) return false;

    try {
        CommonAPI::CallStatus status;
        float usage = 0.0f;
        m_proxy->requestGpuUsageData(status, usage);

        if (status != CommonAPI::CallStatus::SUCCESS) return false;

        std::ostringstream oss;
        if (usage < 0.0f) {
            oss << "GPU Load: Unavailable";
        } else {
            oss << "GPU Load: " << std::fixed << std::setprecision(1) << usage << "%";
        }
        out = oss.str();
        return true;
    } catch (...) {
        return false;
    }
}

// SomeIPCpuSource

bool SomeIPCpuSource::openSource() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized && m_proxy && m_proxy->isAvailable()) return true;

    try {
        m_runtime = CommonAPI::Runtime::get();
        if (!m_runtime) return false;

        m_proxy = m_runtime->buildProxy<v1::omnimetron::cpu::CpuUsageDataProxy>(
            "local", "omnimetron.cpu.CpuUsageData");
        if (!m_proxy) return false;

        m_initialized = true;
        return waitForProxy(m_proxy);
    } catch (...) {
        return false;
    }
}

bool SomeIPCpuSource::readSource(std::string& out) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_proxy || !m_proxy->isAvailable()) return false;

    try {
        CommonAPI::CallStatus status;
        float usage = 0.0f;
        m_proxy->requestCpuUsageData(status, usage);

        if (status != CommonAPI::CallStatus::SUCCESS) return false;

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << usage;
        out = oss.str();
        return true;
    } catch (...) {
        return false;
    }
}

// SomeIPMemorySource

bool SomeIPMemorySource::openSource() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized && m_proxy && m_proxy->isAvailable()) return true;

    try {
        m_runtime = CommonAPI::Runtime::get();
        if (!m_runtime) return false;

        m_proxy = m_runtime->buildProxy<v1::omnimetron::memory::MemoryUsageDataProxy>(
            "local", "omnimetron.memory.MemoryUsageData");
        if (!m_proxy) return false;

        m_initialized = true;
        return waitForProxy(m_proxy);
    } catch (...) {
        return false;
    }
}

bool SomeIPMemorySource::readSource(std::string& out) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_proxy || !m_proxy->isAvailable()) return false;

    try {
        CommonAPI::CallStatus status;
        float usageMB = 0.0f;
        m_proxy->requestMemoryUsageData(status, usageMB);

        if (status != CommonAPI::CallStatus::SUCCESS) return false;

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << usageMB;
        out = oss.str();
        return true;
    } catch (...) {
        return false;
    }
}

// SomeIPCpuTempSource

bool SomeIPCpuTempSource::openSource() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized && m_proxy && m_proxy->isAvailable()) return true;

    try {
        m_runtime = CommonAPI::Runtime::get();
        if (!m_runtime) return false;

        m_proxy = m_runtime->buildProxy<v1::omnimetron::cpu_temp::CpuTempDataProxy>(
            "local", "omnimetron.cpu_temp.CpuTempData");
        if (!m_proxy) return false;

        m_initialized = true;
        return waitForProxy(m_proxy);
    } catch (...) {
        return false;
    }
}

bool SomeIPCpuTempSource::readSource(std::string& out) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_proxy || !m_proxy->isAvailable()) return false;

    try {
        CommonAPI::CallStatus status;
        float temp = 0.0f;
        m_proxy->requestCpuTempData(status, temp);

        if (status != CommonAPI::CallStatus::SUCCESS) return false;

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << temp;
        out = oss.str();
        return true;
    } catch (...) {
        return false;
    }
}

}