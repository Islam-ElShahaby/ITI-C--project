#include "LogMessage.hpp"

std::string severityToString(LogSeverity severity)
{
    switch (severity)
    {
        case LogSeverity::INFO: return "INFO";
        case LogSeverity::WARNING: return "WARNING";
        case LogSeverity::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

LogMessage::LogMessage(const std::string& app, const std::string& ctx, 
                       const std::string& txt, LogSeverity sev)
    : appName(app), context(ctx), text(txt), severity(sev) {
    timestamp = std::time(nullptr);
}

std::ostream& operator<<(std::ostream& os, const LogMessage& msg) {
    std::string timeStr = std::ctime(&msg.timestamp);
    if (!timeStr.empty()) 
    {
        timeStr.pop_back();
    }

    os << "[" << timeStr << "] "
       << "[" << msg.appName << "::" << msg.context << "] "
       << "[" << severityToString(msg.severity) << "] "
       << msg.text;
    return os;
}