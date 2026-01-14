#pragma once
#include "ILogSink.hpp"
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