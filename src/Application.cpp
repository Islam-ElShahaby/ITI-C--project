#include "Application.hpp"
#include "LogSinks.hpp"
#include "Policys.hpp"
#include <iostream>
#include <regex>
#include <csignal>
#include <sys/inotify.h>
#include <unistd.h>
#include <limits.h>
// Pulls the first number out of a string like "CPU Load: 42.5%" so it can be passed to formatters
static std::string extractValue(const std::string& text) {
    std::regex valRegex(R"([0-9]*\.?[0-9]+)"); 
    std::smatch match;
    if (std::regex_search(text, match, valRegex)) {
        return match.str();
    }
    return text;
}

Application::Application(const std::string& configPath)
    : m_configPath(configPath)
{
    m_config = AppConfig::load(m_configPath);
    m_threadPool = std::make_shared<ThreadPool>(4);
    
    setupLogging();
    setupTelemetry();
}

Application::~Application() {
    stop();
}

void Application::setupLogging() {
    LogManagerBuilder builder(m_threadPool);
    
    // Read the enabled sinks from config and register them with the builder
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

    // Tell the logger which components should route to which sinks
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
    
    m_watchConfig = true;
    m_configWatcherThread = std::thread(&Application::watchConfig, this);

    // Signal handling is owned by main() rather than here, so we just trust that
    // stop() will be called externally when a shutdown signal arrives.
    runTelemetryLoop();
}

void Application::stop() {
    m_running = false;
    m_watchConfig = false;
    
    if (m_configWatcherThread.joinable()) {
        m_configWatcherThread.join();
    }

    if (m_logger) {
        m_logger->log(LogMessage("Core", "Main", "Stopping Application...", LogSeverity::INFO));
        m_logger->shutdown();
    }
    if (m_threadPool) {
        m_threadPool->shutdown();
    }
}

void Application::runTelemetryLoop() {
    
    // Track when we last polled each source so we can respect per-source intervals
    auto lastCpu = std::chrono::steady_clock::now();
    auto lastMem = std::chrono::steady_clock::now();
    auto lastGpu = std::chrono::steady_clock::now();

    while (m_running) {
        auto now = std::chrono::steady_clock::now();
        
        bool cpuEnabled, memEnabled, gpuEnabled;
        int cpuInterval, memInterval, gpuInterval;
        
        {
            std::lock_guard<std::mutex> lock(m_configMutex);
            cpuEnabled = m_config.telemetry["cpu"].enabled;
            cpuInterval = m_config.telemetry["cpu"].interval;
            memEnabled = m_config.telemetry["memory"].enabled;
            memInterval = m_config.telemetry["memory"].interval;
            gpuEnabled = m_config.telemetry["gpu"].enabled;
            gpuInterval = m_config.telemetry["gpu"].interval;
        }
        
        // --- CPU ---
        if (cpuEnabled) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCpu).count();
            if (elapsed >= cpuInterval) {
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

        // --- Memory ---
        if (memEnabled) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastMem).count();
            if (elapsed >= memInterval) {
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

        // --- GPU ---
        if (gpuEnabled) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastGpu).count();
            if (elapsed >= gpuInterval) {
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

void Application::watchConfig() {
    int fd = inotify_init1(IN_NONBLOCK);
    if (fd < 0) {
        if (m_logger) m_logger->log(LogMessage("Core", "Config", "Failed to initialize inotify", LogSeverity::ERROR));
        return;
    }

    int wd = inotify_add_watch(fd, m_configPath.c_str(), IN_MODIFY);
    if (wd < 0) {
        if (m_logger) m_logger->log(LogMessage("Core", "Config", "Failed to watch config file: " + m_configPath, LogSeverity::ERROR));
        close(fd);
        return;
    }

    if (m_logger) m_logger->log(LogMessage("Core", "Config", "Started watching config file for changes", LogSeverity::INFO));

    char buffer[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));
    
    while (m_watchConfig) {
        ssize_t len = read(fd, buffer, sizeof(buffer));
        if (len > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            if (m_logger) m_logger->log(LogMessage("Core", "Config", "Config modification detected. Reloading...", LogSeverity::INFO));
            
            AppConfig newConfig = AppConfig::load(m_configPath);
            {
                std::lock_guard<std::mutex> lock(m_configMutex);
                m_config = newConfig;
            }

            // Re-apply the routing table so sink changes take effect immediately
            if (m_logger) {
                m_logger->clearRouting();
                for (const auto& [comp, cfg] : newConfig.telemetry) {
                    if (!cfg.sinks.empty()) {
                        m_logger->configureRouting(comp, cfg.sinks);
                    }
                }
            }
            
            if (m_logger) m_logger->log(LogMessage("Core", "Config", "Config loaded successfully", LogSeverity::INFO));
            
            // Re-add watch as some editors replace the file instead of modifying it
            inotify_add_watch(fd, m_configPath.c_str(), IN_MODIFY);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    inotify_rm_watch(fd, wd);
    close(fd);
}
