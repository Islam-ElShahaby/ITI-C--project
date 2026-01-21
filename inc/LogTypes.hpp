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
    GPU,
    Memory,
    Disk,
    Network
};

