#include "LogManager.hpp"
#include "LogSinks.hpp"
#include "LogMessage.hpp"
#include "TelemetrySources.hpp"
#include "ThreadPool.hpp"

#include "LogFormatter.hpp"
#include "Policys.hpp"
#include <regex>
#include <chrono>


std::string extractValue(const std::string& text) {
    std::regex valRegex(R"([0-9]*\.?[0-9]+)"); 
    std::smatch match;
    if (std::regex_search(text, match, valRegex)) {
        return match.str();
    }
    return text;
}

int main() {
    auto pool = std::make_shared<ThreadPool>(4);

    // Use Builder to create LogManager
    LogManagerBuilder builder(pool);
    
    // Use Factory to create Sinks
    auto consoleSink = LogSinkFactory::createSink(LogSinkType::Console);
    auto fileSink = LogSinkFactory::createSink(LogSinkType::File, "app_log.txt");
    
    // Add sinks to builder
    if (consoleSink) builder.addSink(std::move(consoleSink));
    if (fileSink)    builder.addSink(std::move(fileSink));
    
    auto logger = builder.build();

    logger->log(LogMessage("Core", "Main", "System Started - Async Logging Demo", LogSeverity::INFO));

    CpuTelemetrySource cpuSource;
    MemoryTelemetrySource memSource;
    
    // Formatters
    LogFormatter<CpuPolicy> cpuFormatter;
    LogFormatter<RamPolicy> ramFormatter;

    // Use ThreadPool to process telemetry concurrently
    auto cpuFuture = pool->enqueue([&]() {
        if (cpuSource.openSource()) {
            std::string cpuData;
            if (cpuSource.readSource(cpuData)) {
                auto msgOpt = cpuFormatter.formatDataToLogMsg(extractValue(cpuData));
                if (msgOpt) {
                    logger->log(*msgOpt);
                } else {
                    logger->log(LogMessage("Telemetry", "CPU", "Failed to format data: " + cpuData, LogSeverity::ERROR));
                }
            }
        } else {
            logger->log(LogMessage("Telemetry", "CPU", "Failed to open CPU source", LogSeverity::ERROR));
        }
    });

    auto memFuture = pool->enqueue([&]() {
        if (memSource.openSource()) {
            std::string memData;
            if (memSource.readSource(memData)) {
                auto msgOpt = ramFormatter.formatDataToLogMsg(extractValue(memData));
                if (msgOpt) {
                    logger->log(*msgOpt);
                } else {
                    logger->log(LogMessage("Telemetry", "Memory", "Failed to format data: " + memData, LogSeverity::ERROR));
                }
            }
        } else {
            logger->log(LogMessage("Telemetry", "Memory", "Failed to open Memory source", LogSeverity::ERROR));
        }
    });

    // Demo: Concurrent logging from main thread
    for (int i = 0; i < 5; ++i) {
        logger->log(LogMessage("Demo", "Main", "Async log message #" + std::to_string(i + 1), LogSeverity::INFO));
    }

    // Wait for telemetry tasks to complete
    cpuFuture.get();
    memFuture.get();

    logger->log(LogMessage("Core", "Main", "All tasks completed - Shutting down", LogSeverity::INFO));

    // Graceful shutdown
    logger->shutdown();
    pool->shutdown();

    return 0;
}