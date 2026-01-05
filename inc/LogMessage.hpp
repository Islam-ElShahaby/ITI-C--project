#pragma once

#include <ctime>
#include <string>
#include <iostream>

enum class LogSeverity { INFO, WARNING, ERROR };

struct LogMessage 
{
    std::string appName;
    std::string context;
    std::string text;
    std::time_t timestamp;
    LogSeverity severity;

    LogMessage(const std::string& app, const std::string& ctx, const std::string& txt, LogSeverity sev);
    
    friend std::ostream& operator<<(std::ostream& os, const LogMessage& msg);
};