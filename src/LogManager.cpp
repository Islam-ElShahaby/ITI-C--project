#include "LogManager.hpp"
#include "ILogSink.hpp"

void LogManager::addSink(ILogSink* sink) 
{
    sinks.push_back(sink);
}

void LogManager::log(const LogMessage& msg) 
{
    messageBuffer.push_back(msg);

    for (auto* sink : sinks) 
    {
        sink->write(msg);
    }
}