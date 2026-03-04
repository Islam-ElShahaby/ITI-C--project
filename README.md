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
  - [Phase 7: Qt6 GUI Dashboard](#phase-7-qt6-gui-dashboard)
- [Libraries & Technologies](#libraries--technologies)
- [Design Patterns Utilized](#design-patterns-utilized)
- [Build Instructions](#build-instructions)
- [Running the System](#running-the-system)
- [Configuration Guide](#configuration-guide-configapp_configjson)

---

## Overview

This repository contains a robust telemetry ingestion and logging software system originally built throughout structured developmental phases. It handles continuous data polling from multiple local components (such as CPU usage, CPU temperature, and RAM directly fetched from native Linux `/proc` and `/sys` variables) and remote, interconnected modules (like GPU, CPU, Memory, and CPU Temperature metrics over **vSOME/IP** using CommonAPI).

The module formats these metrics according to component-specific policies, infers and predicts warning/critical severity thresholds, and routes them completely asynchronously to multiple active storage or display sinks. A full **Qt6 GUI Dashboard** provides real-time charting, live log viewing, and an interactive configuration editor.

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
   ┌───────────────────────────────────────────────────────────────────┐
   │ ┌────────────────┐ ┌───────────────────┐ ┌──────────────────────┐ │
   │ │  CpuTelemetry  │ │  MemoryTelemetry  │ │  SomeIPTelemetry     │ │
   │ │     Source     │ │       Source      │ │  SourceAdapter       │ │
   │ └───────┬────────┘ └─────────┬─────────┘ └────────┬─────────────┘ │
   │         │ poll               │ poll               │ receive       │
   │  ┌──────▼──────┐      ┌──────▼──────┐     ┌───────▼───────┐      │
   │  │ /proc/stat  │      │/proc/meminfo│     │ SomeIP Client │      │
   │  │             │      │             │     │ (Singleton)   │      │
   │  └─────────────┘      └─────────────┘     └───────────────┘      │
   │                                                                   │
   │ ┌─────────────────┐ ┌────────────────────┐                        │
   │ │ GpuTelemetry    │ │ CpuTempTelemetry   │                        │
   │ │    Source       │ │     Source          │                        │
   │ └───────┬─────────┘ └────────┬───────────┘                        │
   │         │ sysfs              │ /sys/class/thermal                 │
   └─────────┴────────────────────┴────────────────────────────────────┘
          │                       │                    │             
          │           [ LAYER 3: FORMATTING ]          │
          ▼                       ▼                    ▼
   ┌──────────────┐      ┌────────────────┐    ┌───────────────┐
   │ LogFormatter │      │  LogFormatter  │    │ LogFormatter  │
   │ <CpuPolicy>  │      │  <RamPolicy>   │    │ <GpuPolicy>   │
   │ (Rules > 75%)│      │ (Rules > 8GB)  │    │ (Rules > 80%) │
   └──────┬───────┘      └────────┬───────┘    └───────┬───────┘
          │                       │                    │
          │      ┌────────────────┤                    │
          │      │  LogFormatter  │                    │
          │      │<CpuTempPolicy> │                    │
          │      │ (Rules > 75°C) │                    │
          │      └────────┬───────┘                    │
          │               │                            │
          └───────────────┼──────────┐ ┌───────────────┘
                          ▼          ▼ ▼
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
   │ │ (std::cout)    │  │ (app_log.log)  │  │ (Unix Socket)     │ │
   │ └────────────────┘  └────────────────┘  └───────────────────┘ │
   │ ┌────────────────┐                                            │
   │ │  QtSinkImpl    │  (Qt Signal/Slot bridge to GUI)            │
   │ └────────────────┘                                            │
   └───────────────────────────────────────────────────────────────┘
                                  │
                                  ▼
 [ LAYER 6: Qt6 GUI DASHBOARD (Phase 7) ]
   ┌───────────────────────────────────────────────────────────────┐
   │                        MainWindow                             │
   │ ┌────────────────────┐ ┌────────────────┐ ┌────────────────┐  │
   │ │ ConfigEditorWidget │ │ LogViewerWidget│ │TelemetryChart  │  │
   │ │ (Live JSON Editor) │ │ (Filtered Log) │ │   Widget       │  │
   │ │ Toggle switches,   │ │ Severity filter│ │ (4x Realtime   │  │
   │ │ interval spinboxes │ │ Search bar     │ │  Line Charts)  │  │
   │ └────────────────────┘ └────────────────┘ └────────────────┘  │
   └───────────────────────────────────────────────────────────────┘
```

The architecture emphasizes modularity and asynchronous, high-throughput data paths. The kernel relies on `LogManager` which operates on a background thread leveraging a lock-free/thread-safe `RingBuffer` and a functional `ThreadPool` to multiplex incoming logs globally to initialized handlers (Console, File, System Socket, Qt GUI).

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

- **Policy-Based Design (`LogFormatter<Policy>`)**: Zero-cost abstraction for formatting context. Structures like `CpuPolicy`, `RamPolicy`, `GpuPolicy`, or `CpuTempPolicy` inject domain contexts and defined thresholds dynamically.
- **Abstractions**: Incorporates `LogSinkFactory` for sink instantiation logic and `LogManagerBuilder` to streamline the complex internal builder pipeline of the central runtime logger.

### Phase 4: Asynchronous Logging

- **Thread-safe `RingBuffer`**: A robust, generic continuous ring buffer driving the main logging pipeline using minimal blocking methodologies. Utilizes `std::optional` to identify payloads safely.
- **`ThreadPool`**: A scalable, `std::future`-oriented Thread Pool controlling task queues instantiated by `std::function`, `std::invoke_result_t`, and `std::packaged_task`. Helps offload specific asynchronous telemetry bursts from the core ingestion loop.

### Phase 5: Telemetry Over Network

- **vSOME/IP & CommonAPI Services**: Full service/client architecture for all four telemetry components (CPU, Memory, GPU, CPU Temperature). Each service is defined via Franca IDL (`.fidl` + `.fdepl`) and auto-generated into CommonAPI stubs and proxies.
- **Dedicated `TelemetryService` Binary**: A standalone executable (`app/service_main.cpp`) that registers and hosts all four CommonAPI services over vSOME/IP, waiting for client poll requests.
- **`SomeIPTelemetrySourceAdapter`**: Connects the custom `ITelemetrySource` backbone to the vSOME/IP environment utilizing the Adaptor pattern. Each telemetry component can be configured individually to use either `"local"` (direct ProcFS/sysfs) or `"someip"` (remote CommonAPI proxy) data sources.
- **Singleton Bridges**: Assuring only a strict single instance interacts securely with network resources over its runtime lifecycle.

### Phase 6: System Wrap Up & Configuration

- **Application Façade**: Re-routes internal class setup complexities directly into a master `Application` unit initialized merely by `start()` / `stop()` functions. Supports sink injection via `addSink()` for pluggable output targets like the Qt GUI.
- **Dynamic Live Config Management**: Powered natively by `nlohmann::json` and POSIX `inotify`, the daemon continuously polls `config/app_config.json`. Intervals, source toggles, source types (`local`/`someip`), and sink routing trees can be fundamentally modified while the application actively runs, without any restart.
- **Per-Component Source Selection**: Each telemetry component has a `"source"` field in the config (`"local"` or `"someip"`), allowing fine-grained control over whether data is read directly from the local system or fetched remotely via vSOME/IP.

### Phase 7: Qt6 GUI Dashboard

A full-featured **Qt6 Widgets** desktop application (`LoggerQtApp`) providing real-time monitoring and control:

- **`MainWindow`**: Top-level window orchestrating three major panels in a splitter layout with a custom purple-themed dark stylesheet.
- **`TelemetryChartWidget`**: Four real-time line charts (`RealtimeLineChart`) rendered with `QPainter` — CPU Usage (%), Memory (MB), GPU Usage (%), and CPU Temperature (°C). Each chart supports configurable warning/critical threshold lines, auto-scaling, and rolling 120-point data windows (~2 minutes at 1s interval).
- **`LogViewerWidget`**: Live scrolling log display with severity-based color coding, severity dropdown filter, text search bar, auto-scroll toggle, and log count statistics. Maintains up to 10,000 entries.
- **`ConfigEditorWidget`**: Interactive configuration editor with iOS-style `ToggleSwitch` widgets, interval spin boxes, sink checkboxes, and source type dropdowns for each telemetry component. Changes are saved directly to `app_config.json` and trigger hot-reload.
- **`QtSinkImpl`**: A custom `ILogSink` implementation that bridges the core logging engine to the Qt signal/slot system, emitting structured log messages to the GUI widgets in a thread-safe manner.

---

## Libraries & Technologies

- **Modern C++17 Capabilities**: Extensive inclusion of `std::mutex`, `std::condition_variable`, `std::future`, `std::optional`, `std::string_view`, advanced templates, callables, and dynamic casting.
- **[Qt6 Widgets](https://doc.qt.io/qt-6/)**: Desktop GUI framework for real-time charts, log viewing, and configuration editing.
- **[vSOME/IP](https://github.com/COVESA/vsomeip)** & **CommonAPI**: Critical networking standards implemented reliably for embedded/automotive IPC structures.
- **[nlohmann/json](https://github.com/nlohmann/json)**: JSON deserialization utilized for seamless, hierarchical config generation.
- **[magic_enum](https://github.com/Neargye/magic_enum)**: Enum reflection library.
- **POSIX API OS Functions**: Including native `unistd.h`, `fcntl.h`, socket mechanisms, and `inotify` daemon configurations.

---

## Design Patterns Utilized

- **Creational**: Builder (`LogManagerBuilder`), Factory (`LogSinkFactory`), Singleton (`SomeIPTelemetrySourceImpl`).
- **Structural**: Adaptor (`SomeIPTelemetrySourceAdapter`), Façade (`Application`), Proxy (`vSOME/IP Proxy`).
- **Behavioural**: Strategy (`ILogSink`), Observer (vSOME/IP event subscriptions utilizing `std::function` callback trees, Qt signal/slot connections).
- **Concurrency**: Thread Pool (`ThreadPool`).

---

## Build Instructions

**Requirements**: CMake (3.10+), GCC/Clang with C++17 support, vsomeip3, CommonAPI Core/SomeIP binaries. Optionally Qt6 Widgets for the GUI application.

### Method 1: CMake

```bash
mkdir build && cd build
cmake ..
make
```

This produces three executables (if Qt6 is found):

| Target | Description |
|---|---|
| `LoggerApp` | CLI telemetry client with async logging |
| `TelemetryService` | vSOME/IP service hosting CPU, Memory, GPU & CPU Temp data |
| `LoggerQtApp` | Qt6 GUI dashboard (requires Qt6 Widgets) |

### Method 2: Bazel

```bash
bazel build //...
```

### Cross-Compilation (Raspberry Pi 3B+)

A cross-compilation script is provided for building the `TelemetryService` for aarch64:

```bash
./scripts/cross_build_service.sh
```

This uses a CMake toolchain file at `cmake/rpi3-toolchain.cmake` and outputs the binary to `build-rpi/`.

---

## Running the System

Helper scripts with pre-configured vSOME/IP environment variables are provided in `scripts/`:

### 1. Start the Telemetry Service (vSOME/IP server)

```bash
./scripts/run_service.sh
```

Registers and hosts all four CommonAPI services (CPU, Memory, GPU, CPU Temperature) over vSOME/IP.

### 2. Start the CLI Client

```bash
./scripts/run_client.sh
```

Runs the headless `LoggerApp` that polls telemetry and routes logs to configured sinks.

### 3. Start the Qt GUI Dashboard

```bash
./scripts/run_qt_app.sh
```

Launches the `LoggerQtApp` with real-time charts, log viewer, and config editor.

### 4. Cross-deploy to Raspberry Pi

```bash
./scripts/run_service_rpi.sh
```

---

## Configuration Guide (`config/app_config.json`)

The entire event logic and routing matrices live functionally inside `app_config.json`. Thanks to POSIX `inotify`, editing `app_config.json` while the software is active automatically triggers an intelligent context reload.

```json
{
    "general": {
        "global_interval": 1000
    },
    "sinks": {
        "console": { "enabled": true },
        "file": { "enabled": true, "path": "app_log.log" }
    },
    "telemetry": {
        "cpu": {
            "enabled": true,
            "interval": 100,
            "sinks": ["file"],
            "source": "local"
        },
        "cpu_temp": {
            "enabled": true,
            "interval": 100,
            "sinks": ["console", "file"],
            "source": "local"
        },
        "gpu": {
            "enabled": true,
            "interval": 100,
            "sinks": ["console", "file"],
            "source": "local"
        },
        "memory": {
            "enabled": false,
            "interval": 100,
            "sinks": ["console"],
            "source": "local"
        }
    }
}
```

- **enabled**: Turn any independent telemetry module or system sink dynamically on/off.
- **interval**: Specify exact execution/polling intervals in milliseconds for each thread module.
- **sinks**: Re-route data streams specifically for every metric toward `"console"`, `"file"`, or the `"socket"` dynamically built endpoints.
- **source**: Choose the data source on a per-component basis — `"local"` reads directly from ProcFS/sysfs, `"someip"` fetches from a remote vSOME/IP `TelemetryService`.
