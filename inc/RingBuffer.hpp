#pragma once
#include <vector>
#include <optional>
#include <mutex>
#include <atomic>

template <typename T>
class RingBuffer
{
private:
    std::vector<std::optional<T>> m_buffer;
    size_t m_head = 0;
    size_t m_tail = 0;
    size_t m_capacity;
    bool m_full = false;
    std::mutex m_mutex;

public:
    explicit RingBuffer(size_t capacity) : m_capacity(capacity)
    {
        m_buffer.resize(capacity);
    }

    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    RingBuffer(RingBuffer&& other) noexcept
    {
        std::lock_guard<std::mutex> lock(other.m_mutex);
        m_buffer = std::move(other.m_buffer);
        m_head = other.m_head;
        m_tail = other.m_tail;
        m_capacity = other.m_capacity;
        m_full = other.m_full;
    }

    RingBuffer& operator=(RingBuffer&& other) noexcept
    {
        if (this != &other)
        {
            std::scoped_lock lock(m_mutex, other.m_mutex);
            m_buffer = std::move(other.m_buffer);
            m_head = other.m_head;
            m_tail = other.m_tail;
            m_capacity = other.m_capacity;
            m_full = other.m_full;
        }
        return *this;
    }

    bool tryPush(T&& item)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (m_full) {
            return false;
        }

        m_buffer[m_head] = std::move(item);
        m_head = (m_head + 1) % m_capacity;
        
        if (m_head == m_tail) {
            m_full = true;
        }
        
        return true;
    }

    std::optional<T> tryPop()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (!m_full && m_head == m_tail) {
            return std::nullopt;
        }

        std::optional<T> item = std::move(m_buffer[m_tail]);
        m_buffer[m_tail] = std::nullopt;
        m_tail = (m_tail + 1) % m_capacity;
        m_full = false;

        return item;
    }
};
