#pragma once
#include "LogTypes.hpp"
#include <string_view>

struct CpuPolicy
{
    static constexpr LogTelemetrySrc context = LogTelemetrySrc::Cpu;
    static constexpr std::string_view unit = "%";

    static constexpr float WARNING = 75.0f;
    static constexpr float CRITICAL = 90.0f;

    static constexpr LogSeverity inferSeverity(float val) noexcept {
        return (val > CRITICAL) ? LogSeverity::CRITICAL
             : (val > WARNING)  ? LogSeverity::WARNING
             :                    LogSeverity::INFO;
    }
};

struct GpuPolicy
{
    static constexpr LogTelemetrySrc context = LogTelemetrySrc::GPU;
    static constexpr std::string_view unit = "%";

    static constexpr float WARNING = 80.0f;
    static constexpr float CRITICAL = 95.0f;

    static constexpr LogSeverity inferSeverity(float val) noexcept {
        return (val > CRITICAL) ? LogSeverity::CRITICAL
             : (val > WARNING)  ? LogSeverity::WARNING
             :                    LogSeverity::INFO;
    }
};

struct RamPolicy
{
    static constexpr LogTelemetrySrc context = LogTelemetrySrc::Memory;
    static constexpr std::string_view unit = "MB";

    static constexpr float WARNING = 1024.0f * 8;
    static constexpr float CRITICAL = 1024.0f * 12;

    static constexpr LogSeverity inferSeverity(float val) noexcept {
        return (val > CRITICAL) ? LogSeverity::CRITICAL
             : (val > WARNING)  ? LogSeverity::WARNING
             :                    LogSeverity::INFO;
    }
};