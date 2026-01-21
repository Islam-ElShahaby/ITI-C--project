# 1. Minimum CMake version required 
cmake_minimum_required(VERSION 3.10)

# 2. Project Name and Version
project(LoggerPhaseOne VERSION 1.0 LANGUAGES CXX)

# 3. Specify C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 4. featching magic enum lib
include(FetchContent)

FetchContent_Declare(
    magic_enum
    GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
    GIT_TAG        v0.9.7 
)

FetchContent_MakeAvailable(magic_enum)

# 4. Define Source and Header files
set(SOURCES
    app/main.cpp
    src/LogManager.cpp
    src/LogSinks.cpp
    src/LogMessage.cpp
    src/TelemetrySources.cpp
    src/SafeFile.cpp
    src/SafeSocket.cpp
)   

set(HEADERS
    inc/LogManager.hpp
    inc/LogSinks.hpp
    inc/LogMessage.hpp
    inc/ILogSink.hpp
    inc/LogFormatter.hpp
    inc/LogTypes.hpp
    inc/Policys.hpp
    inc/RingBuffer.hpp
)

# 5. Create the Executable
add_executable(LoggerApp ${SOURCES} ${HEADERS})

# 6. Include Directories
target_include_directories(LoggerApp PRIVATE inc)
target_link_libraries(LoggerApp PRIVATE magic_enum::magic_enum)