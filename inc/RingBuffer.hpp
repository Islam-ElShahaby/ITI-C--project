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

    // Copying a ring buffer would create two owners of the same data with shared state, so we disallow it
    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    // Moving locks the source to safely transfer state, then resets the source to empty
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

    // Move assignment — same idea as the move constructor, but handles self-assignment too
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

    // Tries to push an item into the buffer. Returns false immediately if the buffer is full.
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

    // Convenience overload that accepts a const reference by making an internal copy before pushing
    bool tryPush(const T& item)
    {
        T copy = item;
        return tryPush(std::move(copy));
    }

    // Tries to pop an item from the buffer. Returns an empty optional immediately if the buffer is empty.
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

    // Blocks until an item is available or the provided stop condition returns true (used for clean shutdown)
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

    // Wakes up any threads blocked in waitAndPop so they can check the stop condition and exit
    void notifyAll()
    {
        m_notEmpty.notify_all();
        m_notFull.notify_all();
    }

    // Returns the number of items currently in the buffer (thread-safe)
    size_t size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_size;
    }

    // Returns true if the buffer currently holds no items (thread-safe)
    bool isEmpty() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_size == 0;
    }

    // Returns true if the buffer has reached its capacity and cannot accept more items (thread-safe)
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
