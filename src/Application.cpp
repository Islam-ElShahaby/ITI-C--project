#include "Application.hpp"
#include "LogSinks.hpp"
#include "Policys.hpp"
#include <iostream>
#include <regex>
#include <csignal>
#include <sys/inotify.h>
#include <unistd.h>
#include <limits.h>

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

std::unique_ptr<ITelemetrySource> Application::createSource(const std::string& component,
                                                             const std::string& sourceType) {
    if (sourceType == "vsomeip") {
        if (component == "gpu") {
            return std::make_unique<telemetry::SomeIPGpuSource>();
        } else if (component == "cpu") {
            return std::make_unique<telemetry::SomeIPCpuSource>();
        } else if (component == "memory") {
            return std::make_unique<telemetry::SomeIPMemorySource>();
        } else if (component == "cpu_temp") {
            return std::make_unique<telemetry::SomeIPCpuTempSource>();
        }
        return nullptr;
    }
    // Default: local sources
    if (component == "cpu") {
        return std::make_unique<CpuTelemetrySource>();
    } else if (component == "memory") {
        return std::make_unique<MemoryTelemetrySource>();
    } else if (component == "gpu") {
        return std::make_unique<GpuTelemetrySource>();
    } else if (component == "cpu_temp") {
        return std::make_unique<CpuTempTelemetrySource>();
    }
    return nullptr;
}

void Application::setupTelemetry() {
    for (const auto& [name, cfg] : m_config.telemetry) {
        if (!cfg.enabled) continue;

        auto source = createSource(name, cfg.source);
        if (!source) {
            m_logger->log(LogMessage("Telemetry", name, 
                "No source available for component '" + name + "' with type '" + cfg.source + "'",
                LogSeverity::WARNING));
            continue;
        }

        std::string upperName = name;
        for (char& c : upperName) c = std::toupper(static_cast<unsigned char>(c));

        m_logger->log(LogMessage("Telemetry", upperName, 
            "Initializing " + name + " source (type: " + cfg.source + ")", LogSeverity::INFO));

        if (source->openSource()) {
            m_logger->log(LogMessage("Telemetry", upperName, 
                name + " source connected", LogSeverity::INFO));
        } else {
            m_logger->log(LogMessage("Telemetry", upperName, 
                name + " source unavailable", LogSeverity::WARNING));
        }

        std::lock_guard<std::mutex> lock(m_sourcesMutex);
        m_sources[name] = std::move(source);
    }
}

void Application::start() {
    m_logger->log(LogMessage("Core", "Main", "Application Started", LogSeverity::INFO));
    
    m_watchConfig = true;
    m_configWatcherThread = std::thread(&Application::watchConfig, this);

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
    std::map<std::string, std::chrono::steady_clock::time_point> lastPoll;

    while (m_running) {
        auto now = std::chrono::steady_clock::now();

        // Snapshot config under lock
        std::map<std::string, TelemetryConfig> telConfig;
        {
            std::lock_guard<std::mutex> lock(m_configMutex);
            telConfig = m_config.telemetry;
        }

        for (const auto& [name, cfg] : telConfig) {
            if (!cfg.enabled) continue;

            // Initialize last poll time if first time
            if (lastPoll.find(name) == lastPoll.end()) {
                lastPoll[name] = now;
            }

            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - lastPoll[name]).count();
            if (elapsed < cfg.interval) continue;

            // Access the source under lock
            std::lock_guard<std::mutex> srcLock(m_sourcesMutex);
            auto it = m_sources.find(name);
            if (it == m_sources.end() || !it->second) continue;

            ITelemetrySource* src = it->second.get();
            if (src->openSource()) {
                std::string data;
                if (src->readSource(data)) {
                    // Use the appropriate formatter based on component name
                    if (name == "cpu") {
                        auto msgOpt = m_cpuFormatter.formatDataToLogMsg(extractValue(data));
                        if (msgOpt) m_logger->log(*msgOpt);
                    } else if (name == "memory") {
                        auto msgOpt = m_ramFormatter.formatDataToLogMsg(extractValue(data));
                        if (msgOpt) m_logger->log(*msgOpt);
                    } else if (name == "cpu_temp") {
                        auto msgOpt = m_cpuTempFormatter.formatDataToLogMsg(extractValue(data));
                        if (msgOpt) m_logger->log(*msgOpt);
                    } else {
                        // For GPU and any other source, log the raw data
                        std::string upperName = name;
                        for (char& c : upperName) c = std::toupper(static_cast<unsigned char>(c));
                        m_logger->log(LogMessage("Telemetry", upperName, data, LogSeverity::INFO));
                    }
                }
            }
            lastPoll[name] = now;
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
            AppConfig oldConfig;
            {
                std::lock_guard<std::mutex> lock(m_configMutex);
                oldConfig = m_config;
                m_config = newConfig;
            }

            // Re-apply the routing table so sink changes take effect
            if (m_logger) {
                m_logger->clearRouting();
                for (const auto& [comp, cfg] : newConfig.telemetry) {
                    if (!cfg.sinks.empty()) {
                        m_logger->configureRouting(comp, cfg.sinks);
                    }
                }
                // Re-inject the Qt GUI sink
                std::lock_guard<std::mutex> lock(m_extraSinksMutex);
                for (const auto& name : m_extraSinkNames) {
                    m_logger->addSinkToAllRoutes(name);
                }
            }

            // Recreate sources if source type changed or new sources appeared
            {
                std::lock_guard<std::mutex> srcLock(m_sourcesMutex);
                for (const auto& [name, cfg] : newConfig.telemetry) {
                    if (!cfg.enabled) {
                        m_sources.erase(name);
                        continue;
                    }
                    // Check if source needs recreation (new or type changed)
                    bool needsNew = (m_sources.find(name) == m_sources.end());
                    if (!needsNew) {
                        // Compare old and new source type
                        auto oldIt = oldConfig.telemetry.find(name);
                        if (oldIt != oldConfig.telemetry.end() && oldIt->second.source != cfg.source) {
                            needsNew = true;
                        }
                    }
                    if (needsNew) {
                        auto source = createSource(name, cfg.source);
                        if (source) {
                            source->openSource();
                            m_sources[name] = std::move(source);
                        }
                    }
                }
            }
            
            if (m_logger) m_logger->log(LogMessage("Core", "Config", "Config loaded successfully", LogSeverity::INFO));

            // Re-add watch
            inotify_add_watch(fd, m_configPath.c_str(), IN_MODIFY);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    inotify_rm_watch(fd, wd);
    close(fd);
}
