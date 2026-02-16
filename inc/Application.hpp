#pragma once
#include <memory>
#include <atomic>
#include <string>
#include "LogManager.hpp"
#include "TelemetrySources.hpp"
#include "SomeIPTelemetrySource.hpp"
#include "LogFormatter.hpp"
#include "Policys.hpp"
#include "Config.hpp"
#include "ThreadPool.hpp"

class Application {
public:
    explicit Application(const std::string& configPath);
    ~Application();

    void start();
    void stop();

private:
    void setupLogging();
    void setupTelemetry();
    void runTelemetryLoop();

    AppConfig m_config;
    std::shared_ptr<ThreadPool> m_threadPool;
    std::unique_ptr<LogManager> m_logger;
    
    // Telemetry Sources
    CpuTelemetrySource m_cpuSource;
    MemoryTelemetrySource m_memSource;
    telemetry::SomeIPTelemetrySourceAdapter m_gpuSource{false}; // Default init, re-configured in ctor if needed

    // Formatters
    LogFormatter<CpuPolicy> m_cpuFormatter;
    LogFormatter<RamPolicy> m_ramFormatter;

    std::atomic<bool> m_running{true};
};
