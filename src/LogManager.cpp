#include "LogManager.hpp"
#include "ILogSink.hpp"

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
    m_routingTable[component] = sinkNames;
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
            
            // Check routing
            bool hasRouting = m_routingTable.find(msg.appName) != m_routingTable.end();
            const auto& targetSinks = m_routingTable[msg.appName];
            
            if (m_threadPool) 
            {
                std::vector<std::future<void>> futures;
                futures.reserve(sinks.size());

                for (auto& sink : sinks) 
                {
                    ILogSink* sinkPtr = sink.get();
                    
                    // Routing check
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
                        futures.push_back(m_threadPool->enqueue([sinkPtr, &msg]() {
                            sinkPtr->write(msg);
                        }));
                    }
                }

                for(auto& f : futures) {
                   f.get();
                }
            } 
            else 
            {
                for (auto& sink : sinks) 
                {
                    // Routing check
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
                        sink->write(msg);
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