#pragma once
#include <vector>
#include "LogMessage.hpp"

class ILogSink; 

class LogManager {
private:
    std::vector<LogMessage> messageBuffer;
    std::vector<ILogSink*> sinks; 

public:
    void addSink(ILogSink* sink);
    void log(const LogMessage& msg);
};