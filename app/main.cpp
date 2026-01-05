#include "LogManager.hpp"
#include "LogSinks.hpp"
#include "LogMessage.hpp"

int main() {
    LogManager logger;

    ConsoleSinkImpl consoleSink;
    FileSinkImpl fileSink("app_log.txt");

    logger.addSink(&consoleSink);
    logger.addSink(&fileSink);

    logger.log(LogMessage("Core", "Main", "System Started", LogSeverity::INFO));

    return 0;
}