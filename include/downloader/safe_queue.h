#ifndef _SAFE_QUEUE_H_  // NOLINT
#define _SAFE_QUEUE_H_  // NOLINT

#include <queue>
#include <mutex>    // NOLINT
#include <condition_variable>   // NOLINT
#include <memory>
#include <utility>

namespace downloader {

namespace thread {

template<typename T>
class SafeQueue {
 public:
    SafeQueue() { }
    ~SafeQueue() { }

    void Push(T new_value) {
        std::shared_ptr<T> data(
            std::make_shared<T>(std::move(new_value)));
        std::lock_guard<std::mutex> lk(m_mutex);
        m_queue.push(data);
        m_cond.notify_one();
    }

    void WaitAndLoop(T& value) {    // NOLINT
        std::unique_lock<std::mutex> lk(m_mutex);
        m_cond.wait(lk, [this]{return !m_queue.empty();});
        value = std::move(*m_queue.front());
        m_queue.pop();
    }

    std::shared_ptr<T> WaitAndPop() {
        std::unique_lock<std::mutex> lk(m_mutex);
        m_cond.wait(lk, [this]{return !m_queue.empty();});
        std::shared_ptr<T> res = m_queue.front();
        m_queue.pop();
        return res;
    }

    bool TryPop(T& value) { // NOLINT
        std::lock_guard<std::mutex> lk(mut);
        if (m_queue.empty()) {
            return false;
        }
        value = std::move(*m_queue.front());
        m_queue.pop();
        return true;
    }

    std::shared_ptr<T> TryPop() {
        std::lock_guard<std::mutex> lk(m_mutex);
        if (m_queue.empty()) {
            return std::shared_ptr<T>();
        }
        std::shared_ptr<T> res = m_queue.front();
        m_queue.pop();
        return res;
    }

    bool Empty() {
        std::lock_guard<std::mutex> lk(m_mutex);
        return m_queue.empty();
    }

 private:
    std::queue<std::shared_ptr<T>> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_cond;
};

}   // namespace thread

}   // namespace downloader

#endif  // _SAFE_QUEUE_H_ NOLINT