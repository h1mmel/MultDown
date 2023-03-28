#ifndef _HTTP_DOWNLOAD_STRATEGY_H_   // NOLINT
#define _HTTP_DOWNLOAD_STRATEGY_H_   // NOLINT

#include <mutex>    // NOLINT
#include <string>
#include <vector>

#include "downloader/download_strategy.h"

namespace downloader {

static std::mutex g_mutex;
static std::mutex g_prog_mutex;

class HttpDownloadStrategy : public DownloadStrategy {
 public:
    HttpDownloadStrategy(int threads_number, const std::string& path);

    virtual ~HttpDownloadStrategy();

    Status Download(const std::string& url,
                    uint64_t start,
                    uint64_t end) override;

 private:
    struct MetaData;
    struct WriteData;

    void WorkerThread(WriteData* data);

    static size_t LockWriteFunc(void* ptr, size_t size, size_t nmemb,
                                void* user_data);

    static size_t LockFreeWriteFunc(void* ptr, size_t size, size_t nmemb,
                                    void* user_data);

    static int ProgressFunc(void *clientp,
                            curl_off_t dltotal, curl_off_t dlnow,
                            curl_off_t ultotal, curl_off_t ulnow);

    Status GetDownloadStatistic() const;

    int m_threads_number;
    std::string m_path;
    MetaData* m_meta;
    std::vector<WriteData*> m_data_vec;
    std::once_flag m_once;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
};

}   // namespace downloader

#endif  // _HTTP_DOWNLOAD_STRATEGY_H_    NOLINT