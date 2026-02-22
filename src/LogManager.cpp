#include "LogManager.hpp"
#include "ILogSink.hpp"
#include <iostream>

LogManager::LogManager(std::shared_ptr<ThreadPool> pool, size_t bufferSize) 
    : messageBuffer(bufferSize), sinks(), m_threadPool(std::move(pool)), running(true)
{
    workerThread = std::thread(&LogManager::processLogs, this);
}

LogManager::~LogManager()
{
    shutdown();
}

void LogManager::shutdown()
{
    bool expected = true;
    if (running.compare_exchange_strong(expected, false)) {
        messageBuffer.notifyAll();
        
        if (workerThread.joinable()) {
            workerThread.join();
        }
    }
}

void LogManager::configureRouting(const std::string& component, const std::vector<std::string>& sinkNames) {
    std::lock_guard<std::mutex> lock(m_routingMutex);
    m_routingTable[component] = sinkNames;
}

void LogManager::clearRouting() {
    std::lock_guard<std::mutex> lock(m_routingMutex);
    m_routingTable.clear();
}

void LogManager::processLogs()
{
    while (running.load() || !messageBuffer.isEmpty()) 
    {
        auto msgOpt = messageBuffer.waitAndPop([this]() { 
            return !running.load(); 
        });

        if (msgOpt.has_value()) 
        {
            const LogMessage& msg = msgOpt.value();
            
            std::string routingKey = msg.context;
            for (char& c : routingKey) c = std::tolower(static_cast<unsigned char>(c));

            // Snapshot the routing decision under lock, then release before writing
            bool hasRouting = false;
            std::vector<std::string> targetSinks;
            {
                std::lock_guard<std::mutex> lock(m_routingMutex);
                auto it = m_routingTable.find(routingKey);
                hasRouting = (it != m_routingTable.end());
                if (hasRouting) {
                    targetSinks = it->second;
                }
            }

            if (m_threadPool) 
            {
                std::vector<std::pair<std::string, std::future<void>>> futures;
                futures.reserve(sinks.size());

                for (auto& sink : sinks) 
                {
                    ILogSink* sinkPtr = sink.get();
                    
                    // If a routing rule exists, only write to the sinks that are explicitly listed
                    bool shouldLog = true;
                    if (hasRouting) {
                        shouldLog = false;
                        for (const auto& name : targetSinks) {
                            if (name == sinkPtr->getName()) {
                                shouldLog = true;
                                break;
                            }
                        }
                    }

                    if (shouldLog) {
                        futures.push_back({sinkPtr->getName(), m_threadPool->enqueue([sinkPtr, &msg]() {
                            sinkPtr->write(msg);
                        })});
                    }
                }

                // Wait for each sink independently — a failure in one sink
                // must not prevent other sinks from receiving messages
                for (auto& [name, f] : futures) {
                    try {
                        f.get();
                    } catch (const std::exception& e) {
                        std::cerr << "[LogManager] Sink '" << name 
                                  << "' error: " << e.what() << std::endl;
                    }
                }
            } 
            else 
            {
                for (auto& sink : sinks) 
                {
                    bool shouldLog = true;
                    if (hasRouting) {
                        shouldLog = false;
                        for (const auto& name : targetSinks) {
                            if (name == sink->getName()) {
                                shouldLog = true;
                                break;
                            }
                        }
                    }
                    
                    if (shouldLog) {
                        try {
                            sink->write(msg);
                        } catch (const std::exception& e) {
                            std::cerr << "[LogManager] Sink '" << sink->getName()
                                      << "' error: " << e.what() << std::endl;
                        }
                    }
                }
            }
        }
    }
}

void LogManager::addSink(std::unique_ptr<ILogSink> sink) 
{
    sinks.push_back(std::move(sink));
}

void LogManager::log(const LogMessage& msg) 
{
    LogMessage copy = msg;
    messageBuffer.tryPush(std::move(copy));
}

void LogManager::log(LogMessage&& msg)
{
    messageBuffer.tryPush(std::move(msg));
}

LogManagerBuilder::LogManagerBuilder(std::shared_ptr<ThreadPool> pool) 
    : m_manager(std::make_unique<LogManager>(pool)), m_pool(pool) 
{}

LogManagerBuilder& LogManagerBuilder::addSink(std::unique_ptr<ILogSink> sink)
{
    m_manager->addSink(std::move(sink));
    return *this;
}

std::unique_ptr<LogManager> LogManagerBuilder::build()
{
    return std::move(m_manager);
}