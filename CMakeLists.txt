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
)

set(COMMONAPI_GEN_HEADERS
    src-gen/v1/omnimetron/gpu/GpuUsageData.hpp
    src-gen/v1/omnimetron/gpu/GpuUsageDataProxy.hpp
    src-gen/v1/omnimetron/gpu/GpuUsageDataProxyBase.hpp
    src-gen/v1/omnimetron/gpu/GpuUsageDataStub.hpp
    src-gen/v1/omnimetron/gpu/GpuUsageDataStubDefault.hpp
)

# 8. SomeIP Telemetry Sources
set(SOMEIP_CLIENT_SOURCES
    src/SomeIPTelemetrySource.cpp
)

set(SOMEIP_SERVICE_SOURCES
    src/GpuUsageService.cpp
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

# 11. Create GPU Service Application
add_executable(GpuService
    app/service_main.cpp
    ${SOMEIP_SERVICE_SOURCES}
    ${COMMONAPI_SOMEIP_GEN_SOURCES}
    ${COMMONAPI_GEN_HEADERS}
)

target_include_directories(GpuService PRIVATE
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

target_link_libraries(GpuService PRIVATE
    magic_enum::magic_enum
    Threads::Threads
    CommonAPI
    CommonAPI-SomeIP
    vsomeip3
)

