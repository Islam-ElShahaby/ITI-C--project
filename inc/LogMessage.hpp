#pragma once

#include <ctime>
#include <string>
#include <iostream>
#include "LogTypes.hpp"

struct LogMessage 
{
    std::string appName;
    std::string context;
    std::string text;
    std::time_t timestamp;
    LogSeverity severity;

    LogMessage(const std::string& app, const std::string& ctx, const std::string& txt, LogSeverity sev);
    ~LogMessage() = default;
    
    LogMessage(const LogMessage&) = default;
    LogMessage& operator=(const LogMessage&) = default;
    
    LogMessage(LogMessage&&) = default;
    LogMessage& operator=(LogMessage&&) = default;
    
    friend std::ostream& operator<<(std::ostream& os, const LogMessage& msg);
};