# 1. Minimum CMake version required 
cmake_minimum_required(VERSION 3.10)

# 2. Project Name and Version
project(LoggerPhaseOne VERSION 1.0 LANGUAGES CXX)

# 3. Specify C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 4. setup external libs
# Setup Magic Enum
add_library(magic_enum_int INTERFACE)
target_include_directories(magic_enum_int INTERFACE 
    "${CMAKE_CURRENT_SOURCE_DIR}/external_libs/magic_enum"
)

# Create the alias "magic_enum::magic_enum" so your linking code works
add_library(magic_enum::magic_enum ALIAS magic_enum_int)

# Setup Nlohmann Json
add_library(nlohmann_json_int INTERFACE)
target_include_directories(nlohmann_json_int INTERFACE 
    "${CMAKE_CURRENT_SOURCE_DIR}/external_libs"
)

# Create the alias "nlohmann_json::nlohmann_json"
add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json_int)

# 5. Find CommonAPI and vsomeip packages
find_package(CommonAPI REQUIRED)
find_package(CommonAPI-SomeIP REQUIRED)
find_package(vsomeip3 REQUIRED)

# 6. Define Source files for base library
set(BASE_SOURCES
    src/LogManager.cpp
    src/LogSinks.cpp
    src/LogMessage.cpp
    src/TelemetrySources.cpp
    src/SafeFile.cpp
    src/SafeSocket.cpp
    src/ThreadPool.cpp
    src/Application.cpp
)

set(BASE_HEADERS
    inc/LogManager.hpp
    inc/LogSinks.hpp
    inc/LogMessage.hpp
    inc/ILogSink.hpp
    inc/LogFormatter.hpp
    inc/LogTypes.hpp
    inc/Policys.hpp
    inc/RingBuffer.hpp
    inc/ThreadPool.hpp
    inc/ITelemetrySource.hpp
    inc/TelemetrySources.hpp
)

# 7. CommonAPI generated sources (SomeIP binding implementation)
set(COMMONAPI_SOMEIP_GEN_SOURCES
    src-gen/v1/omnimetron/gpu/GpuUsageDataSomeIPDeployment.cpp
    src-gen/v1/omnimetron/gpu/GpuUsageDataSomeIPProxy.cpp
    src-gen/v1/omnimetron/gpu/GpuUsageDataSomeIPStubAdapter.cpp
    src-gen/v1/omnimetron/cpu/CpuUsageDataSomeIPDeployment.cpp
    src-gen/v1/omnimetron/cpu/CpuUsageDataSomeIPProxy.cpp
    src-gen/v1/omnimetron/cpu/CpuUsageDataSomeIPStubAdapter.cpp
    src-gen/v1/omnimetron/memory/MemoryUsageDataSomeIPDeployment.cpp
    src-gen/v1/omnimetron/memory/MemoryUsageDataSomeIPProxy.cpp
    src-gen/v1/omnimetron/memory/MemoryUsageDataSomeIPStubAdapter.cpp
    src-gen/v1/omnimetron/cpu_temp/CpuTempDataSomeIPDeployment.cpp
    src-gen/v1/omnimetron/cpu_temp/CpuTempDataSomeIPProxy.cpp
    src-gen/v1/omnimetron/cpu_temp/CpuTempDataSomeIPStubAdapter.cpp
)

set(COMMONAPI_GEN_HEADERS
    src-gen/v1/omnimetron/gpu/GpuUsageData.hpp
    src-gen/v1/omnimetron/gpu/GpuUsageDataProxy.hpp
    src-gen/v1/omnimetron/gpu/GpuUsageDataProxyBase.hpp
    src-gen/v1/omnimetron/gpu/GpuUsageDataStub.hpp
    src-gen/v1/omnimetron/gpu/GpuUsageDataStubDefault.hpp
    src-gen/v1/omnimetron/cpu/CpuUsageData.hpp
    src-gen/v1/omnimetron/cpu/CpuUsageDataProxy.hpp
    src-gen/v1/omnimetron/cpu/CpuUsageDataProxyBase.hpp
    src-gen/v1/omnimetron/cpu/CpuUsageDataStub.hpp
    src-gen/v1/omnimetron/cpu/CpuUsageDataStubDefault.hpp
    src-gen/v1/omnimetron/memory/MemoryUsageData.hpp
    src-gen/v1/omnimetron/memory/MemoryUsageDataProxy.hpp
    src-gen/v1/omnimetron/memory/MemoryUsageDataProxyBase.hpp
    src-gen/v1/omnimetron/memory/MemoryUsageDataStub.hpp
    src-gen/v1/omnimetron/memory/MemoryUsageDataStubDefault.hpp
    # cpu_temp
    src-gen/v1/omnimetron/cpu_temp/CpuTempData.hpp
    src-gen/v1/omnimetron/cpu_temp/CpuTempDataProxy.hpp
    src-gen/v1/omnimetron/cpu_temp/CpuTempDataProxyBase.hpp
    src-gen/v1/omnimetron/cpu_temp/CpuTempDataStub.hpp
    src-gen/v1/omnimetron/cpu_temp/CpuTempDataStubDefault.hpp
)

# 8. SomeIP Telemetry Sources
set(SOMEIP_CLIENT_SOURCES
    src/SomeIPTelemetrySource.cpp
)

set(SOMEIP_SERVICE_SOURCES
    src/GpuUsageService.cpp
    src/CpuUsageService.cpp
    src/MemoryUsageService.cpp
    src/CpuTempService.cpp
)

# 9. Create base library for common code
add_library(LoggerBase STATIC ${BASE_SOURCES} ${BASE_HEADERS})
target_include_directories(LoggerBase PUBLIC 
    inc
    src-gen
    ${COMMONAPI_INCLUDE_DIRS}
    ${COMMONAPI_SOMEIP_INCLUDE_DIRS}
    ${VSOMEIP3_INCLUDE_DIRS}
)
target_link_libraries(LoggerBase PUBLIC 
    magic_enum::magic_enum 
    Threads::Threads
    CommonAPI
    CommonAPI-SomeIP
    vsomeip3
    nlohmann_json::nlohmann_json
)

# 10. Create the main Client Application with SomeIP Telemetry
add_executable(LoggerApp
    app/main.cpp
    ${SOMEIP_CLIENT_SOURCES}
    ${COMMONAPI_SOMEIP_GEN_SOURCES}
    ${COMMONAPI_GEN_HEADERS}
)

target_include_directories(LoggerApp PRIVATE
    inc
    src-gen
    ${COMMONAPI_INCLUDE_DIRS}
    ${COMMONAPI_SOMEIP_INCLUDE_DIRS}
    ${VSOMEIP3_INCLUDE_DIRS}
)

# 11. Create Telemetry Service Application (GPU + CPU + Memory)
add_executable(TelemetryService
    app/service_main.cpp
    ${SOMEIP_SERVICE_SOURCES}
    ${COMMONAPI_SOMEIP_GEN_SOURCES}
    ${COMMONAPI_GEN_HEADERS}
)

target_include_directories(TelemetryService PRIVATE
    inc
    src-gen
    ${COMMONAPI_INCLUDE_DIRS}
    ${COMMONAPI_SOMEIP_INCLUDE_DIRS}
    ${VSOMEIP3_INCLUDE_DIRS}
)

# 12. Find and link required libraries
find_package(Threads REQUIRED)

target_link_libraries(LoggerApp PRIVATE
    LoggerBase
    magic_enum::magic_enum
    Threads::Threads
    CommonAPI
    CommonAPI-SomeIP
    vsomeip3
    nlohmann_json::nlohmann_json
)

target_link_libraries(TelemetryService PRIVATE
    magic_enum::magic_enum
    Threads::Threads
    CommonAPI
    CommonAPI-SomeIP
    vsomeip3
)

# ============================================================
# 13. Qt6 GUI Application — Config Editor + Real-Time Log Viewer
# ============================================================
find_package(Qt6 COMPONENTS Widgets QUIET)
if (Qt6_FOUND)
    message(STATUS "Qt6 Widgets found — building LoggerQtApp")
    set(CMAKE_AUTOMOC ON)

    set(QT_APP_SOURCES
        qt_app/src/qt_main.cpp
        qt_app/src/QtSinkImpl.cpp
        qt_app/src/TelemetryChartWidget.cpp
        qt_app/src/ConfigEditorWidget.cpp
        qt_app/src/LogViewerWidget.cpp
        qt_app/src/MainWindow.cpp
    )

    set(QT_APP_HEADERS
        qt_app/inc/QtSinkImpl.hpp
        qt_app/inc/TelemetryChartWidget.hpp
        qt_app/inc/ConfigEditorWidget.hpp
        qt_app/inc/LogViewerWidget.hpp
        qt_app/inc/MainWindow.hpp
    )

    add_executable(LoggerQtApp
        ${QT_APP_SOURCES}
        ${QT_APP_HEADERS}
        ${SOMEIP_CLIENT_SOURCES}
        ${COMMONAPI_SOMEIP_GEN_SOURCES}
        ${COMMONAPI_GEN_HEADERS}
    )

    target_include_directories(LoggerQtApp PRIVATE
        inc
        qt_app/inc
        src-gen
        ${COMMONAPI_INCLUDE_DIRS}
        ${COMMONAPI_SOMEIP_INCLUDE_DIRS}
        ${VSOMEIP3_INCLUDE_DIRS}
    )

    target_link_libraries(LoggerQtApp PRIVATE
        LoggerBase
        Qt6::Widgets
        magic_enum::magic_enum
        Threads::Threads
        CommonAPI
        CommonAPI-SomeIP
        vsomeip3
        nlohmann_json::nlohmann_json
    )
else()
    message(WARNING "Qt6 Widgets NOT found — skipping LoggerQtApp build")
endif()
