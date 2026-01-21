#include "LogManager.hpp"
#include "LogSinks.hpp"
#include "LogMessage.hpp"
#include "TelemetrySources.hpp"

#include "LogFormatter.hpp"
#include "Policys.hpp"
#include <regex>


std::string extractValue(const std::string& text) {
    std::regex valRegex(R"([0-9]*\.?[0-9]+)"); 
    std::smatch match;
    if (std::regex_search(text, match, valRegex)) {
        return match.str();
    }
    return text;
}

int main() {
    // Use Builder to create LogManager
    LogManagerBuilder builder;
    
    // Use Factory to create Sinks
    auto consoleSink = LogSinkFactory::createSink(LogSinkType::Console);
    auto fileSink = LogSinkFactory::createSink(LogSinkType::File, "app_log.txt");
    
    // Add sinks to builder
    if (consoleSink) builder.addSink(std::move(consoleSink));
    if (fileSink)    builder.addSink(std::move(fileSink));
    
    auto logger = builder.build();

    logger->log(LogMessage("Core", "Main", "System Started with Factory and Builder", LogSeverity::INFO));

    CpuTelemetrySource cpuSource;
    MemoryTelemetrySource memSource;
    
    // Formatters
    LogFormatter<CpuPolicy> cpuFormatter;
    LogFormatter<RamPolicy> ramFormatter;

    if (cpuSource.openSource()) {
        std::string cpuData;
        if (cpuSource.readSource(cpuData)) {
            // Use Formatter to create LogMessage
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

    if (memSource.openSource()) {
        std::string memData;
        if (memSource.readSource(memData)) {
            // Use Formatter to create LogMessage
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

    return 0;
}