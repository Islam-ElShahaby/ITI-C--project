#pragma once
#include <string>

class ITelemetrySource
{
public:
    virtual ~ITelemetrySource() = default;

    virtual bool openSource() = 0;

    virtual bool readSource(std::string& out) = 0;
};