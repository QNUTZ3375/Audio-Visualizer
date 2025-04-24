#pragma once

#include <atomic>
#include <cstddef>
#include <vector>

template<typename T>
class RingBuffer {
    private:
        size_t m_Capacity;
        std::vector<T> m_Data;

        size_t m_Head; // Only modified by producer
        std::atomic<size_t> m_Tail; // Read/write by consumer
    public:
        explicit RingBuffer(size_t capacity)
            : m_Capacity(capacity), m_Data(capacity), m_Head(0), m_Tail(0) {
                static_assert(std::is_same<T, std::float_t>::value, "Template cannot be instantiated with type(s) other than: float");
            }

        ~RingBuffer() {}

        bool push(const T& item) {
            size_t nextHead = (m_Head + 1) % m_Capacity;
            if (nextHead == m_Tail.load(std::memory_order_acquire)) {
                return false; // Buffer full
            }
            m_Data.at(m_Head) = item;
            m_Head = nextHead;
            return true;
        }

        // Pop one item (main/render thread)
        bool pop(T& item) {
            if (m_Tail.load(std::memory_order_acquire) == m_Head) {
                return false; // Buffer empty
            }
            item = m_Data.at(m_Tail);
            m_Tail = (m_Tail + 1) % m_Capacity;
            return true;
        }

        bool isEmpty() const {
            return m_Head == m_Tail.load(std::memory_order_acquire);
        }

        bool isFull() const {
            return ((m_Head + 1) % m_Capacity) == m_Tail.load(std::memory_order_acquire);
        }

        size_t getSize(){
            return m_Head - m_Tail.load(std::memory_order_acquire);
        }
};
