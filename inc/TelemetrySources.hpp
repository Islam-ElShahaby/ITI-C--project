#pragma once
#include "ITelemetrySource.hpp"
#include "SafeFile.hpp"
#include "SafeSocket.hpp"
#include <memory>

class FileTelemetrySourceImpl : public ITelemetrySource
{
private:
    std::string m_path;
    std::unique_ptr<SafeFile> m_file; 
    size_t m_bufferSize;

public:
    FileTelemetrySourceImpl(const std::string& path, size_t bufferSize = 256);
    
    bool openSource() override;
    bool readSource(std::string& out) override;
};

class SocketTelemetrySourceImpl : public ITelemetrySource
{
private:
    std::string m_targetPath;
    SafeSocket m_socket; 

public:
    SocketTelemetrySourceImpl(const std::string& targetPath);

    bool openSource() override;
    bool readSource(std::string& out) override;
};

class CpuTelemetrySource : public ITelemetrySource
{
private:
    FileTelemetrySourceImpl m_fileSource;
    long m_prevIdle = 0;
    long m_prevTotal = 0;

public:
    CpuTelemetrySource();
    bool openSource() override;
    bool readSource(std::string& out) override;
};

class MemoryTelemetrySource : public ITelemetrySource
{
private:
    FileTelemetrySourceImpl m_fileSource;

public:
    MemoryTelemetrySource();
    bool openSource() override;
    bool readSource(std::string& out) override;
};