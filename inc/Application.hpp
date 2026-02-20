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
    
    CpuTelemetrySource m_cpuSource;
    MemoryTelemetrySource m_memSource;
    telemetry::SomeIPTelemetrySourceAdapter m_gpuSource{false};
    LogFormatter<CpuPolicy> m_cpuFormatter;
    LogFormatter<RamPolicy> m_ramFormatter;

    std::atomic<bool> m_running{true};

    // Hot-reloading state
    std::string m_configPath;
    std::mutex m_configMutex;
    std::atomic<bool> m_watchConfig{true};
    std::thread m_configWatcherThread;

    void watchConfig();
};
