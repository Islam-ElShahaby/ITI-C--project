#include "Application.hpp"
#include "LogSinks.hpp"
#include "Policys.hpp"
#include <iostream>
#include <regex>
#include <csignal>

// Helper to extract value
static std::string extractValue(const std::string& text) {
    std::regex valRegex(R"([0-9]*\.?[0-9]+)"); 
    std::smatch match;
    if (std::regex_search(text, match, valRegex)) {
        return match.str();
    }
    return text;
}

Application::Application(const std::string& configPath) 
{
    m_config = AppConfig::load(configPath);
    m_threadPool = std::make_shared<ThreadPool>(4);
    
    setupLogging();
    setupTelemetry();
}

Application::~Application() {
    stop();
}

void Application::setupLogging() {
    LogManagerBuilder builder(m_threadPool);
    
    // Create sinks based on config
    for (const auto& [name, cfg] : m_config.sinks) {
        if (cfg.enabled) {
            LogSinkType type = LogSinkType::Console;
            if (name == "file") type = LogSinkType::File;
            else if (name == "socket") type = LogSinkType::Socket;
            
            auto sink = LogSinkFactory::createSink(type, cfg.path);
            if (sink) builder.addSink(std::move(sink));
        }
    }
    
    m_logger = builder.build();

    // Configure routing
    for (const auto& [comp, cfg] : m_config.telemetry) {
        if (!cfg.sinks.empty()) {
            m_logger->configureRouting(comp, cfg.sinks);
        }
    }
}

void Application::setupTelemetry() {
    // GPU connection
    if (m_config.telemetry["gpu"].enabled) {
        m_logger->log(LogMessage("Telemetry", "GPU", "Connecting to GPU service...", LogSeverity::INFO));
        if (m_gpuSource.openSource()) {
            m_logger->log(LogMessage("Telemetry", "GPU", "Connected to GPU service", LogSeverity::INFO));
        } else {
            m_logger->log(LogMessage("Telemetry", "GPU", "GPU service unavailable", LogSeverity::WARNING));
        }
    }
}

void Application::start() {
    m_logger->log(LogMessage("Core", "Main", "Application Started", LogSeverity::INFO));
    
    // Using a simple signal handling approach implies we might need the handler 
    // to verify if we should stop. But since this is a class, we might keep
    // the signal handling in main or use a static/global flag. 
    // For now, we assume stop() is called from main's signal handler or similar.
    
    runTelemetryLoop();
}

void Application::stop() {
    m_running = false;
    if (m_logger) {
        m_logger->log(LogMessage("Core", "Main", "Stopping Application...", LogSeverity::INFO));
        m_logger->shutdown();
    }
    if (m_threadPool) {
        m_threadPool->shutdown();
    }
}

void Application::runTelemetryLoop() {
    
    // Map of component -> next run time (ticks)
    // To simplify, we'll check elapsed time or use a basic tick counter if intervals are multiples
    // Better approach: Use chrono
    
    auto lastCpu = std::chrono::steady_clock::now();
    auto lastMem = std::chrono::steady_clock::now();
    auto lastGpu = std::chrono::steady_clock::now();

    while (m_running) {
        auto now = std::chrono::steady_clock::now();
        
        // CPU
        if (m_config.telemetry["cpu"].enabled) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCpu).count();
            if (elapsed >= m_config.telemetry["cpu"].interval) {
                if (m_cpuSource.openSource()) {
                    std::string data;
                    if (m_cpuSource.readSource(data)) {
                        auto msgOpt = m_cpuFormatter.formatDataToLogMsg(extractValue(data));
                        if (msgOpt) m_logger->log(*msgOpt);
                    }
                }
                lastCpu = now;
            }
        }

        // Memory
        if (m_config.telemetry["memory"].enabled) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastMem).count();
            if (elapsed >= m_config.telemetry["memory"].interval) {
                if (m_memSource.openSource()) {
                    std::string data;
                    if (m_memSource.readSource(data)) {
                        auto msgOpt = m_ramFormatter.formatDataToLogMsg(extractValue(data));
                        if (msgOpt) m_logger->log(*msgOpt);
                    }
                }
                lastMem = now;
            }
        }

        // GPU
        if (m_config.telemetry["gpu"].enabled) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastGpu).count();
            if (elapsed >= m_config.telemetry["gpu"].interval) {
               std::string data;
               if (m_gpuSource.readSource(data)) {
                   m_logger->log(LogMessage("Telemetry", "GPU", data, LogSeverity::INFO));
               }
               lastGpu = now;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
