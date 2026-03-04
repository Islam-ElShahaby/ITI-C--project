#pragma once

enum class LogSinkType
{
    Console,
    File,
    Socket
};

enum class LogSeverity
{
    INFO,
    WARNING,
    CRITICAL,
    ERROR
};

enum class LogTelemetrySrc
{
    Cpu,
    CpuTemp,
    GPU,
    Memory,
    Disk,
    Network
};

