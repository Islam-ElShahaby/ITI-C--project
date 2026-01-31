#pragma once
#include <vector>
#include <optional>
#include <mutex>
#include <condition_variable>

template <typename T>
class RingBuffer
{
private:
    std::vector<std::optional<T>> m_buffer;
    size_t m_head = 0;
    size_t m_tail = 0;
    size_t m_capacity;
    size_t m_size = 0;
    mutable std::mutex m_mutex;
    std::condition_variable m_notEmpty;
    std::condition_variable m_notFull;

public:
    explicit RingBuffer(size_t capacity) : m_capacity(capacity)
    {
        m_buffer.resize(capacity);
    }

    // Delete copy semantics
    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    // Move constructor
    RingBuffer(RingBuffer&& other) noexcept
    {
        std::lock_guard<std::mutex> lock(other.m_mutex);
        m_buffer = std::move(other.m_buffer);
        m_head = other.m_head;
        m_tail = other.m_tail;
        m_capacity = other.m_capacity;
        m_size = other.m_size;
        other.m_head = 0;
        other.m_tail = 0;
        other.m_size = 0;
    }

    // Move assignment
    RingBuffer& operator=(RingBuffer&& other) noexcept
    {
        if (this != &other)
        {
            std::scoped_lock lock(m_mutex, other.m_mutex);
            m_buffer = std::move(other.m_buffer);
            m_head = other.m_head;
            m_tail = other.m_tail;
            m_capacity = other.m_capacity;
            m_size = other.m_size;
            other.m_head = 0;
            other.m_tail = 0;
            other.m_size = 0;
        }
        return *this;
    }

    // Non-blocking push - returns false if buffer is full
    bool tryPush(T&& item)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            
            if (m_size >= m_capacity) {
                return false;
            }

            m_buffer[m_head] = std::move(item);
            m_head = (m_head + 1) % m_capacity;
            ++m_size;
        }
        
        m_notEmpty.notify_one();
        return true;
    }

    // Non-blocking push with lvalue
    bool tryPush(const T& item)
    {
        T copy = item;
        return tryPush(std::move(copy));
    }

    // Non-blocking pop - returns nullopt if buffer is empty
    std::optional<T> tryPop()
    {
        std::optional<T> item;
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            if (m_size == 0) {
                return std::nullopt;
            }

            item = std::move(m_buffer[m_tail]);
            m_buffer[m_tail] = std::nullopt;
            m_tail = (m_tail + 1) % m_capacity;
            --m_size;
        }

        m_notFull.notify_one();
        return item;
    }

    // Blocking pop with timeout support for shutdown
    template<typename Predicate>
    std::optional<T> waitAndPop(Predicate shouldStop)
    {
        std::optional<T> item;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            
            m_notEmpty.wait(lock, [this, &shouldStop]() {
                return m_size > 0 || shouldStop();
            });

            if (shouldStop() && m_size == 0) {
                return std::nullopt;
            }

            item = std::move(m_buffer[m_tail]);
            m_buffer[m_tail] = std::nullopt;
            m_tail = (m_tail + 1) % m_capacity;
            --m_size;
        }

        m_notFull.notify_one();
        return item;
    }

    // Wake up any waiting consumers (used during shutdown)
    void notifyAll()
    {
        m_notEmpty.notify_all();
        m_notFull.notify_all();
    }

    // Thread-safe size query
    size_t size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_size;
    }

    // Thread-safe empty check
    bool isEmpty() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_size == 0;
    }

    // Thread-safe full check
    bool isFull() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_size >= m_capacity;
    }

    size_t capacity() const
    {
        return m_capacity;
    }
};
