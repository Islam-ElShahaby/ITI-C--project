#include "ThreadPool.hpp"

// Spawns the requested number of worker threads (defaults to 1 if 0 is passed in)
ThreadPool::ThreadPool(size_t numThreads)
{
    if (numThreads == 0) {
        numThreads = 1;
    }

    for (size_t i = 0; i < numThreads; ++i)
    {
        workers.emplace_back([this]() {
            workerLoop();
        });
    }
}

// Ensures a clean shutdown when the object goes out of scope
ThreadPool::~ThreadPool()
{
    shutdown();
}

// Submits a task without caring about the result
void ThreadPool::enqueueTask(std::function<void()> task)
{
    {
        std::lock_guard<std::mutex> lock(queueMutex);

        if (stop.load()) {
            throw std::runtime_error("Cannot enqueue on stopped ThreadPool");
        }

        tasks.emplace(std::move(task));
    }

    condition.notify_one();
}

// Sets the stop flag and waits for every worker thread to finish its current task
void ThreadPool::shutdown()
{
    bool expected = false;
    if (stop.compare_exchange_strong(expected, true))
    {
        condition.notify_all();

        for (std::thread& worker : workers)
        {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
}

void ThreadPool::workerLoop()
{
    while (true)
    {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queueMutex);

            condition.wait(lock, [this]() {
                return stop.load() || !tasks.empty();
            });

            if (stop.load() && tasks.empty()) {
                return;
            }

            task = std::move(tasks.front());
            tasks.pop();
        }
        task();
    }
}