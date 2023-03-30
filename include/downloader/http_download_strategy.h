#ifndef _HTTP_DOWNLOAD_STRATEGY_H_   // NOLINT
#define _HTTP_DOWNLOAD_STRATEGY_H_   // NOLINT

#include <mutex>    // NOLINT
#include <string>
#include <vector>

#include "downloader/download_strategy.h"

namespace downloader {

// TODO(xxx) : 尚未改成单例模式，所有对象共享锁
static std::mutex g_mutex;
static std::mutex g_prog_mutex;

class HttpDownloadStrategy : public DownloadStrategy {
 public:
    HttpDownloadStrategy(int threads_number, const std::string& path);

    virtual ~HttpDownloadStrategy();

    int SetThreadsNumber(int num) override;

    Status Download(const std::string& url,
                    uint64_t start,
                    uint64_t end) override;

 private:
    struct MetaData;
    struct WriteData;

    void WorkerThread(WriteData* data);

    static size_t WriteFunc(void* ptr, size_t size, size_t nmemb,
                            void* user_data);

    static size_t LockWriteFunc(void* ptr, size_t size, size_t nmemb,
                                void* user_data);

    static size_t LockFreeWriteFunc(void* ptr, size_t size, size_t nmemb,
                                    void* user_data);

    static int ProgressFunc(void *clientp,
                            curl_off_t dltotal, curl_off_t dlnow,
                            curl_off_t ultotal, curl_off_t ulnow);

    static int ProgressFunc2(void *clientp,
                             curl_off_t dltotal, curl_off_t dlnow,
                             curl_off_t ultotal, curl_off_t ulnow);

    Status GetDownloadStatistic() const;

    int threads_number_;
    std::string path_;
    MetaData* meta_;
    std::vector<WriteData*> data_vec_;
    std::once_flag once_;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

}   // namespace downloader

#endif  // _HTTP_DOWNLOAD_STRATEGY_H_    NOLINT