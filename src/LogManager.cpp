#include "LogManager.hpp"
#include "ILogSink.hpp"

void LogManager::addSink(std::unique_ptr<ILogSink> sink) 
{
    sinks.push_back(std::move(sink));
}

void LogManager::log(const LogMessage& msg) 
{
    LogMessage copy = msg;
    messageBuffer.tryPush(std::move(copy));

    for (auto& sink : sinks) 
    {
        sink->write(msg);
    }
}

LogManagerBuilder::LogManagerBuilder() : m_manager(std::make_unique<LogManager>()) {}

LogManagerBuilder& LogManagerBuilder::addSink(std::unique_ptr<ILogSink> sink)
{
    m_manager->addSink(std::move(sink));
    return *this;
}

std::unique_ptr<LogManager> LogManagerBuilder::build()
{
    return std::move(m_manager);
}