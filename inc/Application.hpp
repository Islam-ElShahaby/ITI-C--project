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
#include "ILogSink.hpp"

class Application {
public:
    explicit Application(const std::string& configPath);
    ~Application();

    void start();
    void stop();
    
    // Allow injecting additional sinks (e.g. the Qt GUI sink)
    void addSink(std::unique_ptr<ILogSink> sink) {
        if (m_logger) {
            std::string name = sink->getName();
            m_logger->addSink(std::move(sink));
            m_logger->addSinkToAllRoutes(name);
            // Remember the name so watchConfig() can re-inject after reload
            std::lock_guard<std::mutex> lock(m_extraSinksMutex);
            m_extraSinkNames.push_back(name);
        }
    }

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

    // Names of sinks added via addSink() — replayed after config reloads
    std::vector<std::string> m_extraSinkNames;
    std::mutex m_extraSinksMutex;

    void watchConfig();
};
