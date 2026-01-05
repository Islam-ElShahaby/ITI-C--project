#pragma once
#include "ILogSink.hpp"
#include <string>

class ConsoleSinkImpl : public ILogSink 
{
public:
    void write(const LogMessage& msg) override;
};

class FileSinkImpl : public ILogSink 
{
private:
    std::string filePath;
public:
    FileSinkImpl(const std::string& path);
    void write(const LogMessage& msg) override;
};