#pragma once

#include <string>

struct LogMessage;

class ILogSink 
{
    public:
    virtual ~ILogSink() = default;
    virtual void write(const LogMessage& msg) = 0;
    virtual std::string getName() const = 0;
};