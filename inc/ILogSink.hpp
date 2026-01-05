#pragma once

struct LogMessage;

class ILogSink 
{
    public:
    virtual ~ILogSink() = default;
    virtual void write(const LogMessage& msg) = 0;
};