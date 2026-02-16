#pragma once
#include "RingBuffer.hpp"
#include "LogMessage.hpp"
#include <memory>
#include <thread>
#include <atomic>
#include "ThreadPool.hpp"
#include <map>
#include <vector>
#include <string>

class ILogSink; 

class LogManager {
private:
    RingBuffer<LogMessage> messageBuffer;
    std::vector<std::unique_ptr<ILogSink>> sinks;
    std::shared_ptr<ThreadPool> m_threadPool;
    
    // Threading members
    std::thread workerThread;
    std::atomic<bool> running{true};

    // Routing table: Component Name -> List of Sink Names
    // If a component is not in the map, it goes to all sinks (default behavior)
    std::map<std::string, std::vector<std::string>> m_routingTable;

    // Worker thread function
    void processLogs();

public:
    explicit LogManager(std::shared_ptr<ThreadPool> pool, size_t bufferSize = 1024);
    ~LogManager();

    // Disable copy & move
    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;
    LogManager(LogManager&&) = delete;
    LogManager& operator=(LogManager&&) = delete;

    void addSink(std::unique_ptr<ILogSink> sink);
    void configureRouting(const std::string& component, const std::vector<std::string>& sinkNames);
    void log(const LogMessage& msg);
    void log(LogMessage&& msg);
    
    // Graceful shutdown
    void shutdown();
};

class LogManagerBuilder
{
private:
    std::unique_ptr<LogManager> m_manager;
    std::shared_ptr<ThreadPool> m_pool;

public:
    LogManagerBuilder(std::shared_ptr<ThreadPool> pool);

    LogManagerBuilder& addSink(std::unique_ptr<ILogSink> sink);

    std::unique_ptr<LogManager> build();
};