#pragma once
#include "RingBuffer.hpp"
#include "LogMessage.hpp"
#include <memory>

class ILogSink; 

class LogManager {
private:
    RingBuffer<LogMessage> messageBuffer;
    std::vector<std::unique_ptr<ILogSink>> sinks;

public:
    LogManager(size_t bufferSize = 1024) : messageBuffer(bufferSize) {}
    void addSink(std::unique_ptr<ILogSink> sink);
    void log(const LogMessage& msg);
};

class LogManagerBuilder
{
private:
    std::unique_ptr<LogManager> m_manager;

public:
    LogManagerBuilder();

    LogManagerBuilder& addSink(std::unique_ptr<ILogSink> sink);

    std::unique_ptr<LogManager> build();
};