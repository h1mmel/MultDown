#ifndef _THREAD_POOL_H_ // NOLINT
#define _THREAD_POOL_H_ // NOLINT

#include <atomic>
#include <thread>   // NOLINT
#include <vector>
#include <functional>
#include <future>   // NOLINT
#include <memory>
#include <utility>

#include "downloader/safe_queue.h"
#include "downloader/join_threads.h"

namespace downloader {

namespace thread {

class FunctionWrapper {
 public:
    template<typename F>
    explicit FunctionWrapper(F&& f) : m_impl(new ImplType<F>(std::move(f))) { }

    ~FunctionWrapper() { }

    void operator()() { m_impl->Call(); }

    FunctionWrapper() = default;
    FunctionWrapper(FunctionWrapper&& other) : m_impl(std::move(other.m_impl))
    { }

    FunctionWrapper& operator=(FunctionWrapper&& other) {
        m_impl = std::move(other.m_impl);
        return *this;
    }

    FunctionWrapper(const FunctionWrapper&) = delete;
    FunctionWrapper(FunctionWrapper&) = delete;
    FunctionWrapper& operator=(const FunctionWrapper&) = delete;

 private:
    struct ImplBase {
        virtual void Call() = 0;
        virtual ~ImplBase() { }
    };
    std::unique_ptr<ImplBase> m_impl;
    template<typename F>
    struct ImplType : ImplBase {
        F m_f;
        explicit ImplType(F&& f) : m_f(std::move(f)) { }
        void Call() { m_f(); }
    };
};


class ThreadPool {
 public:
    ThreadPool() : m_done(false), m_join(m_threads) {
        unsigned const thread_count = std::thread::hardware_concurrency();
        try {
            for (unsigned i = 0; i < thread_count; i++) {
                m_threads.push_back(
                        std::thread(&ThreadPool::WorkerThread, this));
            }
        } catch (...) {
            m_done = true;
            throw;
        }
    }

    ~ThreadPool() {
        m_done = true;
    }

    template<typename FunctionType>
    std::future<typename std::result_of<FunctionType()>::type>
    Submit(FunctionType f) {
        typedef typename std::result_of<FunctionType()>::type result_type;
        std::packaged_task<result_type()> task(std::move(f));
        std::future<result_type> res(task.get_future());
        m_work_queue.Push(std::move(task));
        return res;
    }

 private:
    std::atomic_bool m_done;
    SafeQueue<FunctionWrapper> m_work_queue;
    std::vector<std::thread> m_threads;
    JoinThreads m_join;
    void WorkerThread() {
        while (!m_done) {
            FunctionWrapper task;
            if (m_work_queue.TryPop(task)) {
                task();
            } else {
                std::this_thread::yield();
            }
        }
    }
};

}   // namespace thread

}   // namespace downloader

#endif  // _THREAD_POOL_H_ NOLINT
