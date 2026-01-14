#include "LogManager.hpp"
#include "LogSinks.hpp"
#include "LogMessage.hpp"
#include "TelemetrySources.hpp"

int main() {
    LogManager logger;

    ConsoleSinkImpl consoleSink;
    FileSinkImpl fileSink("app_log.txt");

    logger.addSink(&consoleSink);
    logger.addSink(&fileSink);

    logger.log(LogMessage("Core", "Main", "System Started", LogSeverity::INFO));

    CpuTelemetrySource cpuSource;
    MemoryTelemetrySource memSource;

    if (cpuSource.openSource()) {
        std::string cpuData;
        if (cpuSource.readSource(cpuData)) {
            logger.log(LogMessage("Telemetry", "CPU", cpuData, LogSeverity::INFO));
        }
    } else {
        logger.log(LogMessage("Telemetry", "CPU", "Failed to open CPU source", LogSeverity::ERROR));
    }

    if (memSource.openSource()) {
        std::string memData;
        if (memSource.readSource(memData)) {
            logger.log(LogMessage("Telemetry", "Memory", memData, LogSeverity::INFO));
        }
    } else {
        logger.log(LogMessage("Telemetry", "Memory", "Failed to open Memory source", LogSeverity::ERROR));
    }

    return 0;
}