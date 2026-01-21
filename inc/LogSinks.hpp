#pragma once
#include "ILogSink.hpp"
#include "LogTypes.hpp"
#include <string>
#include <string>

class ConsoleSinkImpl : public ILogSink 
{
public:
    void write(const LogMessage& msg) override;
};

#include "SafeFile.hpp"
#include "SafeSocket.hpp"
#include <memory>
#include <vector>

class FileSinkImpl : public ILogSink 
{
private:
    std::string filePath;
    std::unique_ptr<SafeFile> safeFile;
public:
    FileSinkImpl(const std::string& path);
    void write(const LogMessage& msg) override;
};

class SocketSinkImpl : public ILogSink
{
private:
    std::string socketPath;
    SafeSocket safeSocket;
public:
    SocketSinkImpl(const std::string& path);
    void write(const LogMessage& msg) override;
};

class LogSinkFactory
{
public:
    static std::unique_ptr<ILogSink> createSink(LogSinkType type, const std::string& path = "")
    {
        switch (type)
        {
        case LogSinkType::Console:
            return std::make_unique<ConsoleSinkImpl>();
        case LogSinkType::File:
            return std::make_unique<FileSinkImpl>(path.empty() ? "default.log" : path);
        case LogSinkType::Socket:
            return std::make_unique<SocketSinkImpl>(path.empty() ? "/tmp/default_socket" : path);
        default:
            return nullptr;
        }
    }
};