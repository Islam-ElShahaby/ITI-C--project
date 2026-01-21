#include "LogMessage.hpp"
#include <magic_enum/magic_enum.hpp>

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
       << "[" << magic_enum::enum_name(msg.severity) << "] "
       << msg.text;
    return os;
}   