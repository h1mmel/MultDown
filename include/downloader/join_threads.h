#ifndef _JOIN_THREADS_H_    // NOLINT
#define _JOIN_THREADS_H_    // NOLINT

#include <vector>
#include <thread>   // NOLINT

namespace downloader {

namespace thread {

class JoinThreads {
 public:
    explicit JoinThreads(std::vector<std::thread>* threads)
        : threads_(threads) { }

    ~JoinThreads() {
        for (uint32_t i = 0; i < threads_->size(); i++) {
            if ((*threads_)[i].joinable()) {
                (*threads_)[i].join();
            }
        }
    }

 private:
    std::vector<std::thread>* threads_;
};

}   // namespace thread

}   // namespace downloader

#endif  // _JOIN_THREADS_H_ NOLINT
