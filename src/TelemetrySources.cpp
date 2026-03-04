#include "TelemetrySources.hpp"

#include <iostream>
#include <sstream>
#include <fstream>
#include <memory>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <fcntl.h>


FileTelemetrySourceImpl::FileTelemetrySourceImpl(const std::string& path, size_t bufferSize) 
    : m_path(path), m_bufferSize(bufferSize) 
{
}

bool FileTelemetrySourceImpl::openSource()
{
    if (m_file) return true; // File is already open

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
    if (!m_file && !openSource()) return false;

    try {
        m_file->rewind();
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

CpuTelemetrySource::CpuTelemetrySource() 
    : m_fileSource("/proc/stat", 1024)
{
}

bool CpuTelemetrySource::openSource() {
    if (!m_fileSource.openSource()) return false;
    
    // Seed initial values to establish an accurate baseline if not already established
    if (m_prevTotal == 0 && m_prevIdle == 0) {
        std::string content;
        if (m_fileSource.readSource(content)) {
            std::stringstream ss(content);
            std::string cpuLabel;
            long user, nice, system, idle, iowait, irq, softirq, steal;
            if (ss >> cpuLabel >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal) {
                m_prevTotal = user + nice + system + idle + iowait + irq + softirq + steal;
                m_prevIdle = idle + iowait;
            }
        }
    }

    return true;
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
        double usedMB = (double)(totalMem - availableMem) / 1024.0;
        out = "Memory Usage: " + std::to_string(usedMB) + " MB Used (" + std::to_string(usedPercent) + "%)";
    } else {
        out = "Memory Usage: Unknown";
    }

    return true;
}

bool GpuTelemetrySource::openSource() {
    // Nothing to open — we try sysfs/nvidia-smi on each read
    return true;
}

bool GpuTelemetrySource::readSource(std::string& out) {
    // Try AMD/Intel sysfs paths
    std::vector<std::string> gpuPaths = {
        "/sys/class/drm/card0/device/gpu_busy_percent",
        "/sys/class/drm/card1/device/gpu_busy_percent",
    };

    for (const auto& path : gpuPaths) {
        std::ifstream file(path);
        if (file.is_open()) {
            float usage = 0.0f;
            file >> usage;
            if (file.good() || file.eof()) {
                out = "GPU Load: " + std::to_string(usage) + "%";
                return true;
            }
        }
    }

    // Try nvidia-smi
    FILE* pipe = popen("nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits 2>/dev/null", "r");
    if (pipe) {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            float usage = std::atof(buffer);
            pclose(pipe);
            out = "GPU Load: " + std::to_string(usage) + "%";
            return true;
        }
        pclose(pipe);
    }

    out = "GPU Load: Unavailable";
    return true;
}

bool CpuTempTelemetrySource::openSource() {
    if (!m_tempPath.empty()) return true;

    // Try thermal_zone first
    std::vector<std::string> candidates = {
        "/sys/class/thermal/thermal_zone0/temp"
    };

    // Scan hwmon devices for known CPU temp drivers
    const std::vector<std::string> knownDrivers = {
        "k10temp", "zenpower", "coretemp", "cpu_thermal"
    };
    for (int i = 0; i < 20; ++i) {
        std::string nameFile = "/sys/class/hwmon/hwmon" + std::to_string(i) + "/name";
        std::ifstream nf(nameFile);
        if (!nf.is_open()) continue;
        std::string driverName;
        std::getline(nf, driverName);
        nf.close();
        while (!driverName.empty() && std::isspace(driverName.back()))
            driverName.pop_back();

        for (const auto& drv : knownDrivers) {
            if (driverName == drv) {
                candidates.push_back("/sys/class/hwmon/hwmon" + std::to_string(i) + "/temp1_input");
                break;
            }
        }
    }

    for (const auto& path : candidates) {
        std::ifstream test(path);
        if (test.is_open()) {
            test.close();
            m_tempPath = path;
            return true;
        }
    }

    return false;
}

bool CpuTempTelemetrySource::readSource(std::string& out) {
    if (m_tempPath.empty() && !openSource()) return false;

    std::ifstream file(m_tempPath);
    if (!file.is_open()) return false;

    try {
        long millideg = 0;
        file >> millideg;
        file.close();
        double tempC = millideg / 1000.0;
        out = "CPU Temp: " + std::to_string(tempC) + "°C";
    } catch (...) {
        out = "CPU Temp: Unavailable";
    }

    return true;
}