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
    
    // The background thread that drains the message buffer and dispatches to sinks
    std::thread workerThread;
    std::atomic<bool> running{true};

    // Maps component names to the specific sinks they should write to.
    // If a component has no entry here, its messages are sent to every sink.
    std::map<std::string, std::vector<std::string>> m_routingTable;
    mutable std::mutex m_routingMutex;

    // Runs on the worker thread — continuously reads from the buffer and writes to sinks
    void processLogs();

public:
    explicit LogManager(std::shared_ptr<ThreadPool> pool, size_t bufferSize = 1024);
    ~LogManager();

    // LogManager owns the worker thread and sinks, so copying or moving it doesn't make sense
    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;
    LogManager(LogManager&&) = delete;
    LogManager& operator=(LogManager&&) = delete;

    void addSink(std::unique_ptr<ILogSink> sink);
    void configureRouting(const std::string& component, const std::vector<std::string>& sinkNames);
    void clearRouting();
    void addSinkToAllRoutes(const std::string& sinkName);
    void log(const LogMessage& msg);
    void log(LogMessage&& msg);
    
    // Flushes any remaining messages and stops the worker thread cleanly
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