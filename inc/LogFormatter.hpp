#pragma once
#include "LogTypes.hpp"
#include "LogMessage.hpp"
#include <string>
#include <optional>
#include <string_view>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <iostream>

template <typename Policy>
class LogFormatter
{
public:
    LogFormatter() = default;
    virtual ~LogFormatter() = default;

    std::optional<LogMessage> formatDataToLogMsg(const std::string& raw)
    {
        try {
            float val = std::stof(raw);
            LogSeverity sev = Policy::inferSeverity(val);
            std::string desc = msgDescription(val);
            
            std::string ctxStr = "Unknown";
            switch (Policy::context)
            {
                case LogTelemetrySrc::Cpu: ctxStr = "CPU"; break;
                case LogTelemetrySrc::GPU: ctxStr = "GPU"; break;
                case LogTelemetrySrc::Memory: ctxStr = "Memory"; break;
                case LogTelemetrySrc::Disk: ctxStr = "Disk"; break;
                case LogTelemetrySrc::Network: ctxStr = "Network"; break;
            }

            return LogMessage("Telemetry", ctxStr, desc, sev);
        } catch (...) {
            return std::nullopt;
        }
    }

    std::string msgDescription(float val)
    {
        std::stringstream ss;
        ss << "Value: " << val << Policy::unit;
        return ss.str();
    }
    
    std::string currentTimeStamp()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm = *std::localtime(&now_c);
        
        std::stringstream ss;
        ss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};