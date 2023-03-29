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
        std::lock_guard<std::mutex> lk(mutex_);
        queue_.push(data);
        cond_.notify_one();
    }

    void WaitAndLoop(T& value) {    // NOLINT
        std::unique_lock<std::mutex> lk(mutex_);
        cond_.wait(lk, [this]{return !queue_.empty();});
        value = std::move(*queue_.front());
        queue_.pop();
    }

    std::shared_ptr<T> WaitAndPop() {
        std::unique_lock<std::mutex> lk(mutex_);
        cond_.wait(lk, [this]{return !queue_.empty();});
        std::shared_ptr<T> res = queue_.front();
        queue_.pop();
        return res;
    }

    bool TryPop(T& value) { // NOLINT
        std::lock_guard<std::mutex> lk(mut);
        if (queue_.empty()) {
            return false;
        }
        value = std::move(*queue_.front());
        queue_.pop();
        return true;
    }

    std::shared_ptr<T> TryPop() {
        std::lock_guard<std::mutex> lk(mutex_);
        if (queue_.empty()) {
            return std::shared_ptr<T>();
        }
        std::shared_ptr<T> res = queue_.front();
        queue_.pop();
        return res;
    }

    bool Empty() {
        std::lock_guard<std::mutex> lk(mutex_);
        return queue_.empty();
    }

 private:
    std::queue<std::shared_ptr<T>> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cond_;
};

}   // namespace thread

}   // namespace downloader

#endif  // _SAFE_QUEUE_H_ NOLINT