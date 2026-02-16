#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

struct TelemetryConfig {
    bool enabled = true;
    int interval = 1000;
    std::vector<std::string> sinks;
};

struct SinkConfig {
    bool enabled = true;
    std::string path; // Only for file/socket sinks
};

struct GeneralConfig {
    int globalInterval = 1000;
};

struct AppConfig {
    std::map<std::string, TelemetryConfig> telemetry;
    std::map<std::string, SinkConfig> sinks;
    GeneralConfig general;

    static AppConfig load(const std::string& path) {
        AppConfig config;
        try {
            std::ifstream f(path);
            if (!f.is_open()) {
                std::cerr << "Failed to open config file: " << path << ". Using defaults." << std::endl;
                return config;
            }
            nlohmann::json j = nlohmann::json::parse(f);

            // Parse Telemetry
            if (j.contains("telemetry")) {
                for (auto& [key, val] : j["telemetry"].items()) {
                    TelemetryConfig tc;
                    if (val.contains("enabled")) tc.enabled = val["enabled"];
                    if (val.contains("interval")) tc.interval = val["interval"];
                    if (val.contains("sinks")) tc.sinks = val["sinks"].get<std::vector<std::string>>();
                    config.telemetry[key] = tc;
                }
            }

            // Parse Sinks
            if (j.contains("sinks")) {
                for (auto& [key, val] : j["sinks"].items()) {
                    SinkConfig sc;
                    if (val.contains("enabled")) sc.enabled = val["enabled"];
                    if (val.contains("path")) sc.path = val["path"];
                    config.sinks[key] = sc;
                }
            }

            // Parse General
            if (j.contains("general")) {
                auto& g = j["general"];
                if (g.contains("global_interval")) config.general.globalInterval = g["global_interval"];
            }

        } catch (const std::exception& e) {
            std::cerr << "Error parsing config: " << e.what() << ". Using defaults/partial config." << std::endl;
        }
        return config;
    }
};
