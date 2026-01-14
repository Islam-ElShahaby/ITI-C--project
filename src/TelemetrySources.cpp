#include "TelemetrySources.hpp"

#include <iostream>
#include <sstream>
#include <memory>
#include <vector>
#include <fcntl.h>


FileTelemetrySourceImpl::FileTelemetrySourceImpl(const std::string& path, size_t bufferSize) 
    : m_path(path), m_bufferSize(bufferSize) 
{
}

bool FileTelemetrySourceImpl::openSource()
{
    try {
        m_file = std::make_unique<SafeFile>(m_path, O_RDONLY);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "File Open Error: " << e.what() << std::endl;
        return false;
    }
}

bool FileTelemetrySourceImpl::readSource(std::string& out)
{
    if (!m_file) return false;

    try {
        m_file->read(out, m_bufferSize);
        return true;
    }
    catch (...) {
        return false;
    }
}

SocketTelemetrySourceImpl::SocketTelemetrySourceImpl(const std::string& path)
    : m_targetPath(path)
{   }

bool SocketTelemetrySourceImpl::openSource()
{
    try {
        m_socket.connect(m_targetPath);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Socket Connect Error: " << e.what() << std::endl;
        return false;
    }
}

bool SocketTelemetrySourceImpl::readSource(std::string& out)
{
    try {
        m_socket.receive(out);
        return true;
    }
    catch (...) {
        return false;
    }
}

// CpuTelemetrySource
CpuTelemetrySource::CpuTelemetrySource() 
    : m_fileSource("/proc/stat", 1024)
{
}

bool CpuTelemetrySource::openSource() {
    return m_fileSource.openSource();
}

bool CpuTelemetrySource::readSource(std::string& out) {
    if (!m_fileSource.openSource()) return false; 
    
    std::string content;
    if (!m_fileSource.readSource(content)) return false;

    std::stringstream ss(content);
    std::string cpuLabel;
    long user, nice, system, idle, iowait, irq, softirq, steal;
    ss >> cpuLabel >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

    long total = user + nice + system + idle + iowait + irq + softirq + steal;
    long totalIdle = idle + iowait;

    long diffTotal = total - m_prevTotal;
    long diffIdle = totalIdle - m_prevIdle;

    double usage = 0.0;
    if (diffTotal > 0) {
        usage = (double)(diffTotal - diffIdle) / diffTotal * 100.0;
    }

    m_prevTotal = total;
    m_prevIdle = totalIdle;

    out = "CPU Load: " + std::to_string(usage) + "%";
    return true;
}

// MemoryTelemetrySource
MemoryTelemetrySource::MemoryTelemetrySource() 
    : m_fileSource("/proc/meminfo", 2048)
{
}

bool MemoryTelemetrySource::openSource() {
    return m_fileSource.openSource();
}

bool MemoryTelemetrySource::readSource(std::string& out) {
    if (!m_fileSource.openSource()) return false;

    std::string content;
    if (!m_fileSource.readSource(content)) return false;

    std::stringstream ss(content);
    std::string line;
    long totalMem = 0;
    long availableMem = 0;

    while (std::getline(ss, line)) {
        if (line.find("MemTotal:") == 0) {
            std::stringstream ls(line);
            std::string label, unit;
            ls >> label >> totalMem >> unit;
        } else if (line.find("MemAvailable:") == 0) {
            std::stringstream ls(line);
            std::string label, unit;
            ls >> label >> availableMem >> unit;
        }
    }

    if (totalMem > 0) {
        double usedPercent = (double)(totalMem - availableMem) / totalMem * 100.0;
        out = "Memory Usage: " + std::to_string(usedPercent) + "% (" + 
              std::to_string(totalMem - availableMem) + "/" + std::to_string(totalMem) + " kB)";
    } else {
        out = "Memory Usage: Unknown";
    }

    return true;
}