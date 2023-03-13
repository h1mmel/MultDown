#ifndef _JOIN_THREADS_H_    // NOLINT
#define _JOIN_THREADS_H_    // NOLINT

#include <vector>
#include <thread>   // NOLINT

namespace downloader {

namespace thread {

class JoinThreads {
 public:
    explicit JoinThreads(std::vector<std::thread>& threads) // NOLINT
        : m_threads(threads) { }

    ~JoinThreads() {
        for (uint32_t i = 0; i < m_threads.size(); i++) {
            if (m_threads[i].joinable()) {
                m_threads[i].join();
            }
        }
    }

 private:
    std::vector<std::thread>& m_threads;
};

}   // namespace thread

}   // namespace downloader

#endif  // _JOIN_THREADS_H_ NOLINT
