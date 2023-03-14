#ifndef _HTTP_DOWNLOAD_H_   // NOLINT
#define _HTTP_DOWNLOAD_H_   // NOLINT

#include <math.h>
#include <curl/curl.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <mutex>    // NOLINT
#include <thread>   // NOLINT
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "downloader/proto.h"
#include "downloader/download_strategy.h"
#include "downloader/join_threads.h"
#include "downloader/http.h"
#include "downloader/debug_functions.h"

namespace downloader {

static std::mutex g_mutex;
static std::mutex g_prog_mutex;

class HttpDownloader : public DownloadStrategy {
 public:
    HttpDownloader() : m_data_vec(), m_fp(nullptr) {}

    uint64_t Download(const std::string& url,
                      const std::string& file_path,
                      uint64_t start,
                      uint64_t end) override;

    int MutiDown(const std::string&,
                 const std::string& file_path,
                 uint64_t start,
                 uint64_t end,
                 int threads) override;

    // 获取还未下载的字节数
    // 与 MutiDown 结合使用，且必须在 MutiDown 函数完成后使用
    // TODO(xxx): 别扭
    uint64_t GetRest();

    virtual ~HttpDownloader() {
        if (m_fp != nullptr) fclose(m_fp);
        for (uint64_t i = 0; i < m_data_vec.size(); i++) {
            delete m_data_vec[i];
        }
    }

 private:
    struct WriteData {
        uint64_t head;
        uint64_t tail;
        CURL* curl;
        const char* file_path;
        const char* url;
        FILE* fp;
        void* m_this;
        WriteData() : head(0), tail(0), curl(nullptr),
                        file_path(nullptr), url(nullptr),
                        fp(nullptr), m_this(nullptr)
        {}
    };

    std::vector<WriteData*> m_data_vec;
    FILE* m_fp;

 private:
    void WorkerThread(WriteData* data) {
        std::stringstream stream;
        // stream << syscall(SYS_gettid) << std::endl;
        // std::cout << stream.str();
        CURL* curl = curl_easy_init();
        if (curl) {
            data->curl = curl;
            char range[64] = {0};
            snprintf (range, sizeof (range), "%lu-%lu",
                    data->head, data->tail);

            curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, debug_callback);
            curl_easy_setopt(curl, CURLOPT_URL, data->url);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFunc);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);
            curl_easy_setopt(curl, CURLOPT_RANGE, range);
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressFunc);
            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, data);
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
            // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

            CURLcode res = curl_easy_perform(data->curl);
            if (CURLE_OK != res) {
                stream.clear();
                stream.str("");
                stream << "curl_easy_perform() failed: "
                        << curl_easy_strerror(res) << std::endl;
                std::cerr << stream.str();
            }
            int32_t status = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
            if (status != 200 && status != 206) {
                stream.clear();
                stream.str("");
                stream << "\nerror status: " << status << std::endl;
                std::cerr << stream.str();
            }
            curl_easy_cleanup(data->curl);
        }
    }

    static size_t WriteFunc(void* ptr, size_t size, size_t nmemb,
                            void* user_data) {
        WriteData* data = static_cast<WriteData*>(user_data);
        size_t written = 0;
        std::lock_guard<std::mutex> lk(g_mutex);
        if (data->head + size * nmemb <= data->tail) {
            fseek(data->fp, data->head, SEEK_SET);
            written = fwrite(ptr, size, nmemb, data->fp);
            data->head += size * nmemb;
        } else {
            fseek(data->fp, data->head, SEEK_SET);
            written = fwrite(ptr, 1, data->tail - data->head + 1, data->fp);
            data->head = data->tail;
        }
        return written;
    }

    // TODO(xxx) : printf? lock 太长, 一次性输出
    static int ProgressFunc(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
                            curl_off_t ultotal, curl_off_t ulnow) {
        WriteData* data = static_cast<WriteData*>(clientp);
        HttpDownloader* p_this = static_cast<HttpDownloader*>(data->m_this);
        int total_dot = 80;
        double percent = 0.0;
        uint64_t total = 0;
        for (uint64_t i = 0; i < p_this->m_data_vec.size(); i++) {
            if (p_this->m_data_vec[i]->tail > total) {
                total = p_this->m_data_vec[i]->tail;
            }
        }
        total += 1;
        uint64_t down = 0, rest = 0;
        for (uint64_t i = 0; i < p_this->m_data_vec.size(); i++) {
            rest += p_this->m_data_vec[i]->tail - p_this->m_data_vec[i]->head;
        }
        down = total - rest;
        percent = 1.0 * down / total;
        int dot = round(percent * total_dot);
        std::lock_guard<std::mutex> lk(g_prog_mutex);
        printf("\r%-30s%3.0f%%", data->file_path, percent * 100);
        int i = 0;
        printf("[");
        for (; i < dot; i++)
            printf("=");
        if (i < total_dot) {
            printf(">");
            i++;
        }
        for (; i < total_dot; i++)
            printf(" ");
        printf("]       ");
        if (down < 1024) printf("%luB", down);
        else if (down < 1024 * 1024) printf("%3.2lfK", 1.0 * down / 1024);
        else if (down < 1024 * 1024 * 1024)
            printf("%3.2lfM", 1.0 * down / 1024 / 1024);
        else
            printf("%3.2lfG", 1.0 * down / 1024 / 1024 / 1024);
        printf("%30s", " ");
        fflush(stdout);
        return 0;
    }
};

}   // namespace downloader

#endif  // _HTTP_DOWNLOAD_H_    NOLINT