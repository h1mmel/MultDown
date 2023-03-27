#ifndef _HTTP_DOWNLOAD_H_   // NOLINT
#define _HTTP_DOWNLOAD_H_   // NOLINT

#include <math.h>
#include <curl/curl.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

#include <mutex>    // NOLINT
#include <thread>   // NOLINT
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include <iomanip>

#include "downloader/download_strategy.h"

namespace downloader {

static std::mutex g_mutex;
static std::mutex g_prog_mutex;

class HttpDownloader : public DownloadStrategy {
 public:
    HttpDownloader(int threads_number, const std::string& path);

    virtual ~HttpDownloader();

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

    Status GetDownloadStatistic();

    int m_threads_number;
    std::string m_path;
    MetaData* m_meta;
    std::vector<WriteData*> m_data_vec;
    std::once_flag m_once;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
};

}   // namespace downloader

#endif  // _HTTP_DOWNLOAD_H_    NOLINT