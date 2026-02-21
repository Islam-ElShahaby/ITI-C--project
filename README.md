# C++ Telemetry & Logging System

A scalable, multi-threaded, and highly configurable telemetry and logging system written in Modern C++ (C++17). Designed with industry-standard architectural best practices, various software design patterns, and modern libraries for optimal performance and connectivity in embedded environments.

---

## Table of Contents

- [Overview](#overview)
- [System Architecture](#system-architecture)
- [Features & Phases Breakdown](#features--phases-breakdown)
  - [Phase 1: Core Synchronous Logging Foundation](#phase-1-core-synchronous-logging-foundation)
  - [Phase 2: Data Sources & Smart Resource Management](#phase-2-data-sources--smart-resource-management)
  - [Phase 3: Formatter & Threshold Logic](#phase-3-formatter--threshold-logic)
  - [Phase 4: Asynchronous Logging](#phase-4-asynchronous-logging)
  - [Phase 5: Telemetry Over Network](#phase-5-telemetry-over-network)
  - [Phase 6: System Wrap Up & Configuration](#phase-6-system-wrap-up--configuration)
- [Libraries & Technologies](#libraries--technologies)
- [Design Patterns Utilized](#design-patterns-utilized)
- [Build Instructions](#build-instructions)
- [Configuration Guide](#configuration-guide-configapp_configjson)

---

## Overview

This repository contains a robust telemetry ingestion and logging software system originally built throughout structured developmental phases. It handles continuous data polling from multiple local components (such as CPU and RAM directly fetched from native Linux `/proc` variables) and remote, interconnected modules (like GPU metrics over **vSOME/IP**).

The module formats these metrics according to component-specific policies, infers and predicts warning/critical severity thresholds, and routes them completely asynchronously to multiple active storage or display sinks.

## System Architecture

```text
┌────────────────────────────────────────────────────────────────────────┐
│                        TELEMETRY & LOGGING SYSTEM                      │
│                           Core Architecture                            │
└────────────────────────────────────────────────────────────────────────┘

 [ LAYER 1: CONFIGURATION & FAÇADE (Phase 6) ]
                      ┌──────────────────────┐
    app_config.json   │                      │   JSON Parsing -> AppConfig
   ──────────────────▶│  Application Façade  │   (inotify watch & dynamic
                      │                      │    Hot - reloading logic)
                      └──────────┬───────────┘
                                 │ configures & manages
                                 ▼
 [ LAYER 2: TELEMETRY INGESTION (ITelemetrySource) ]
   ┌───────────────────────────────────────────────────────────────┐
   │ ┌────────────────┐ ┌───────────────────┐ ┌──────────────────┐ │
   │ │  CpuTelemetry  │ │  MemoryTelemetry  │ │ SomeIPTelemetry  │ │
   │ │     Source     │ │       Source      │ │  SourceAdapter   │ │
   │ └───────┬────────┘ └─────────┬─────────┘ └────────┬─────────┘ │
   │         │ poll               │ poll               │ receive   │
   │  ┌──────▼──────┐      ┌──────▼──────┐     ┌───────▼───────┐   │
   │  │ /proc/stat  │      │/proc/meminfo│     │ SomeIP Client │   │
   │  │             │      │             │     │ (Singleton)   │   │
   │  └─────────────┘      └─────────────┘     └───────────────┘   │
   └───────────────────────────────────────────────────────────────┘
          │                       │                    │             
          │           [ LAYER 3: FORMATTING ]          │
          ▼                       ▼                    ▼
   ┌──────────────┐      ┌────────────────┐    ┌───────────────┐
   │ LogFormatter │      │  LogFormatter  │    │ LogFormatter  │
   │ <CpuPolicy>  │      │  <RamPolicy>   │    │ <GpuPolicy>   │
   │ (Rules > 75%)│      │ (Rules > 8GB)  │    │ (Rules > 80%) │
   └──────┬───────┘      └────────┬───────┘    └───────┬───────┘
          │                       │                    │
          └─────────────────────┐ │ ┌──────────────────┘
                                ▼ ▼ ▼
                           [ LogMessage ]
                                  │
 [ LAYER 4: ASYNCHRONOUS ENGINE ] │
   ┌──────────────────────────────┼────────────────────────────────┐
   │                              ▼                                │
   │  ┌─────────────────────────────────────────────────────────┐  │
   │  │ RingBuffer<LogMessage> (Lock-Free Thread-Safe Queue)    │  │
   │  │ [ msg | msg | msg | ... |     |     |     |     ]       │  │
   │  └─────────────────────────────────────────────────────────┘  │
   │                              │                                │
   │                     LogManager handles pop                    │
   │                              │                                │
   │                              ▼                                │
   │  ┌─────────────────────────────────────────────────────────┐  │
   │  │                      ThreadPool                         │  │
   │  │ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐     │  │
   │  │ │ Worker 1 │ │ Worker 2 │ │ Worker 3 │ │ Worker 4 │     │  │
   │  │ └──────────┘ └──────────┘ └──────────┘ └──────────┘     │  │
   │  └─────────────────────────────────────────────────────────┘  │
   └──────────────────────────────┬────────────────────────────────┘
                                  │ Wait & Dispatch (std::future)
                                  │
 [ LAYER 5: OUTPUT SINKS (ILogSink) ]
                                  ▼
   ┌───────────────────────────────────────────────────────────────┐
   │ ┌────────────────┐  ┌────────────────┐  ┌───────────────────┐ │
   │ │ConsoleSinkImpl │  │  FileSinkImpl  │  │ SocketSinkImpl    │ │
   │ │ (std::cout)    │  │ (app_log.txt)  │  │ (Unix Socket)     │ │
   │ └────────────────┘  └────────────────┘  └───────────────────┘ │
   └───────────────────────────────────────────────────────────────┘
```

The architecture emphasizes modularity and asynchronous, high-throughput data paths. The kernel relies on `LogManager` which operates on a background thread leveraging a lock-free/thread-safe `RingBuffer` and a functional `ThreadPool` to multiplex incoming logs globally to initialized handlers (Console, File, System Socket).

---

## Features & Phases Breakdown

### Phase 1: Core Synchronous Logging Foundation

- **`LogMessage`**: Encapsulates telemetry metadata, severity statuses, context, and automatic timestamps.
- **`ILogSink`**: A uniform strategy interface allowing flexible endpoints.
- **Specific Sinks**: Developed ready-to-use implementations of `ConsoleSinkImpl`, `FileSinkImpl`, and `SocketSinkImpl`.

### Phase 2: Data Sources & Smart Resource Management

- **Interfacing (`ITelemetrySource`)**: Provides a universal interface for continuous generic data stream ingest.
- **RAII Strict Compliance**: Native system wrappers like `SafeFile` and `SafeSocket` guarantee memory safety and process release, enforcing **move-only semantics** and adopting the Rule of Zero mechanics.
- **ProcFS Parsing**: Parsing system-level `/proc/stat` and `/proc/meminfo` files to iteratively measure active CPU/Memory usage.

### Phase 3: Formatter & Threshold Logic

- **Policy-Based Design (`LogFormatter<Policy>`)**: Zero-cost abstraction for formatting context. Structures like `CpuPolicy` or `RamPolicy` inject domain contexts and defined thresholds dynamically.
- **Abstractions**: Incorporates `LogSinkFactory` for sink instantiation logic and `LogManagerBuilder` to streamline the complex internal builder pipeline of the central runtime logger.

### Phase 4: Asynchronous Logging

- **Thread-safe `RingBuffer`**: A robust, generic continuous ring buffer driving the main logging pipeline using minimal blocking methodologies. Utilizes `std::optional` to identify payloads safely.
- **`ThreadPool`**: A scalable, `std::future`-oriented Thread Pool controlling task queues instantiated by `std::function`, `std::invoke_result_t`, and `std::packaged_task`. Helps offload specific asynchronous telemetry bursts from the core ingestion loop.

### Phase 5: Telemetry Over Network

- **vSOME/IP & CommonAPI Services**: `SomeIPTelemetrySourceImpl` represents an active proxy client using both *synchronous request/response* and *broadcasting asynchronous events* to natively interface with external IPC vehicle/service bus elements.
- **Adaptor & Singleton Bridges**: Connects the custom `ITelemetrySource` backbone to the vSOME/IP environment utilizing the Adaptor pattern, assuring only a strict single instance interacts securely with network resources over its runtime lifecycle.

### Phase 6: System Wrap Up & Configuration

- **Application Façade**: Re-routes internal class setup complexities directly into a master `Application` unit initialized merely by `start()` / `stop()` functions.
- **Dynamic Live Config Management**: Powered natively by `nlohmann::json` and POSIX `inotify`, the daemon continuously polls `config/app_config.json`. Intervals, source toggles, and sink routing trees can be fundamentally modified while the application actively runs, without any restart.

---

## Libraries & Technologies

- **Modern C++17 Capabilities**: Extensive inclusion of `std::mutex`, `std::condition_variable`, `std::future`, `std::optional`, `std::string_view`, advanced templates, callables, and dynamic casting.
- **[vSOME/IP](https://github.com/COVESA/vsomeip)** & **CommonAPI**: Critical networking standards implemented reliably for embedded/automotive IPC structures.
- **[nlohmann/json](https://github.com/nlohmann/json)**: JSON deserialization utilized for seamless, hierarchical config generation.
- **[magic_enum](https://github.com/Neargye/magic_enum)**: Enum reflection library.
- **POSIX API OS Functions**: Including native `unistd.h`, `fcntl.h`, socket mechanisms, and `inotify` daemon configurations.

---

## Design Patterns Utilized

- **Creational**: Builder (`LogManagerBuilder`), Factory (`LogSinkFactory`), Singleton (`SomeIPTelemetrySourceImpl`).
- **Structural**: Adaptor (`SomeIPTelemetrySourceAdapter`), Façade (`Application`), Proxy (`vSOME/IP Proxy`).
- **Behavioural**: Strategy (`ILogSink`), Observer (vSOME/IP event subscriptions utilizing `std::function` callback trees).
- **Concurrency**: Thread Pool (`ThreadPool`).

---

## Build Instructions

**Requirements**: CMake (3.10+), Supported GCC/Clang with C++17 limits optionally tested, vsomeip3, CommonAPI Core/SomeIP binaries locally discovered.

### Method 1: CMake

```bash
mkdir build && cd build
cmake ..
make
```

### Method 2: Bazel

```bash
bazel build //...
```

---

## Configuration Guide (`config/app_config.json`)

The entire event logic and routing matrices live functionally inside `app_config.json`. Thanks to POSIX `inotify`, editing `app_config.json` while the software is active automatically triggers an intelligent context reload.

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
      "enabled": true, 
      "interval": 2000, 
      "sinks": ["console", "socket"] 
    }
  },
  "sinks": {
    "console": { "enabled": true },
    "file": { "enabled": true, "path": "logs/app_log.txt" },
    "socket": { "enabled": false, "path": "/tmp/telemetry.sock"}
  },
  "general": {
    "global_interval": 1000
  }
}
```

- **enabled**: Turn any independent telemetry module or system sink dynamically on/off.
- **interval**: Specify exact execution/polling intervals sequentially in milliseconds for each thread module.
- **sinks**: Re-route data streams specifically for every metric toward `"console"`, `"file"`, or the `"socket"` dynamically built endpoints.
