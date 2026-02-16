# Telemetry Logger System

## Overview

High-performance, configurable telemetry logging system for embedded Linux. Designed with a modular architecture enabling seamless integration of local system metrics (CPU, RAM) and remote telemetry via **vSOME/IP** (GPU).

## Features

- **Runtime Configuration**: JSON-based configuration for enabling/disabling sources, adjusting polling rates, and routing logs.
- **Façade Pattern**: Simplified `Application` class managing initialization and lifecycle.
- **Modular Sinks**: Support for Console, File, and Socket log sinks.
- **Protocol Support**: Integrated **vSOME/IP** for inter-process communication (IPC) and **CommonAPI** for service interaction.
- **Efficient Logging**: Thread-safe logging with `RingBuffer` and `ThreadPool`.

## Architecture

![Architecture]

## Prerequisites

- **CMake** (3.10+)
- **GCC/Clang** (C++17 support)
- **vsomeip3**
- **CommonAPI** & **CommonAPI-SomeIP**
- **nlohmann_json**
- **magic_enum**

## Build Instructions

```bash
# 1. Clone the repository
git clone <repository_url>
cd <repository_folder>

# 2. Configure project
mkdir build && cd build
cmake ..

# 3. Build
make
```

## Running the Application

Ensure the configuration file is present in `config/app_config.json`.

```bash
./LoggerApp
```

To run the GPU Service (Simulated):

```bash
./GpuService
```

## Configuration Guide (`config/app_config.json`)

The system uses a JSON file for runtime configuration.

```json
{
  "telemetry": {
    "cpu": { 
      "enabled": true, 
      "interval": 1000, 
      "sinks": ["console", "file"] 
    },
    "memory": { 
      "enabled": true, 
      "interval": 5000, 
      "sinks": ["file"] 
    },
    "gpu": { 
      "enabled": false, 
      "interval": 2000, 
      "sinks": ["console"] 
    }
  },
  "sinks": {
    "console": { "enabled": true },
    "file": { "enabled": true, "path": "app_log.txt" }
  },
  "general": {
    "global_interval": 1000
  }
}
```

- **enabled**: Toggle specific sources or sinks.
- **interval**: Polling rate in milliseconds.
- **sinks**: List of sinks to route data to (`"console"`, `"file"`, `"socket"`).
