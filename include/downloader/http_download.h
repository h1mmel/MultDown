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

#include "downloader/proto.h"
#include "downloader/download_strategy.h"
#include "downloader/join_threads.h"
#include "downloader/http.h"
#include "downloader/debug_functions.h"

namespace downloader {

static std::mutex g_mutex;
static std::mutex g_prog_mutex;
static uint64_t g_last;

class HttpDownloader : public DownloadStrategy {
 public:
    HttpDownloader(int threads_number, const std::string& path)
        : m_threads_number(threads_number),
          m_path(path),
          m_meta(new MetaData),
          m_once(),
          m_start() {
        for (int i = 0; i < threads_number; i++) {
            m_data_vec.push_back(new WriteData);
        }
        m_meta->file_name =
                path.substr(path.find_last_of('/') + 1, path.size());
    }

    Status Download(const std::string& url,
                    uint64_t start,
                    uint64_t end) override;

    virtual ~HttpDownloader() {
        for (uint64_t i = 0; i < m_data_vec.size(); i++) {
            if (m_data_vec[i]->stat != nullptr)
                delete m_data_vec[i]->stat;
            delete m_data_vec[i];
        }
        if (m_meta != nullptr && m_meta->fp != nullptr) {
            fclose(m_meta->fp);
        }
        if (m_meta != nullptr && m_meta->base != nullptr) {
            int ret = munmap(m_meta->base, m_meta->end - m_meta->start + 1);
            if (ret != 0) perror("munmap error");
        }
        if (m_meta != nullptr) delete m_meta;
    }

 private:
    struct MetaData {
        uint64_t start;
        uint64_t end;
        std::string file_name;
        std::string url;
        FILE* fp;
        uint8_t* base;
        void* m_this;

        MetaData() : start(0),
                     end(0),
                     file_name(),
                     url(),
                     fp(nullptr),
                     base(nullptr),
                     m_this(nullptr)
        {}
    };

    struct WriteData {
        MetaData* meta;
        MetaStatus* stat;
        uint64_t head;
        uint64_t tail;
        CURL* curl;

        WriteData() : meta(nullptr),
                      stat(nullptr),
                      head(0),
                      tail(0),
                      curl(nullptr)
        {}
    };

    int m_threads_number;
    std::string m_path;
    MetaData* m_meta;
    std::vector<WriteData*> m_data_vec;
    std::once_flag m_once;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start;

 private:
    void WorkerThread(WriteData* data) {
        // stream << syscall(SYS_gettid) << std::endl;
        CURL* curl = curl_easy_init();
        if (curl) {
            data->curl = curl;
            char range[64] = {0};
            snprintf(range, sizeof (range), "%lu-%lu",
                     data->head, data->tail);

            curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, debug_callback);
            curl_easy_setopt(curl, CURLOPT_URL, data->meta->url.c_str());
            // curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, LockWriteFunc);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, LockFreeWriteFunc);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                                            reinterpret_cast<void*>(data));
            curl_easy_setopt(curl, CURLOPT_RANGE, range);
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressFunc);
            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, data);
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
            // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);

            std::call_once(m_once, [this](){
                m_start = std::chrono::high_resolution_clock::now();
            });

            data->stat->curl_code = curl_easy_perform(data->curl);
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE,
                                            &data->stat->response_code);
            curl_easy_cleanup(data->curl);
        }
    }

    static size_t LockWriteFunc(void* ptr, size_t size, size_t nmemb,
                                void* user_data) {
        WriteData* data = reinterpret_cast<WriteData*>(user_data);
        size_t written = 0;
        std::lock_guard<std::mutex> lk(g_mutex);
        if (data->head + size * nmemb <= data->tail) {
            fseek(data->meta->fp, data->head, SEEK_SET);
            written = fwrite(ptr, size, nmemb, data->meta->fp);
            data->head += size * nmemb;
        } else {
            fseek(data->meta->fp, data->head, SEEK_SET);
            written = fwrite(ptr, 1, data->tail - data->head + 1,
                             data->meta->fp);
            data->head = data->tail;
        }
        return written;
    }

    static size_t LockFreeWriteFunc(void* ptr, size_t size, size_t nmemb,
                                    void* user_data) {
        WriteData* data = reinterpret_cast<WriteData*>(user_data);
        size_t written = 0;
        if (data->head + size * nmemb <= data->tail) {
            std::memcpy(data->head + data->meta->base, ptr, size * nmemb);
            written = nmemb * size;
            data->head += size * nmemb;
        } else {
            std::memcpy(data->head + data->meta->base, ptr,
                                    data->tail - data->head + 1);
            written = data->tail - data->head + 1;
            data->head = data->tail;
        }
        return written;
    }

    static int ProgressFunc(void *clientp,
                            curl_off_t dltotal, curl_off_t dlnow,
                            curl_off_t ultotal, curl_off_t ulnow) {
        WriteData* data = reinterpret_cast<WriteData*>(clientp);
        HttpDownloader* p_this =
                    reinterpret_cast<HttpDownloader*>(data->meta->m_this);
        int total_dot = 100;
        double percent = 0.0;
        uint64_t total = data->meta->end - data->meta->start + 1;
        uint64_t down = 0, rest = 0;
        for (uint64_t i = 0; i < p_this->m_data_vec.size(); i++) {
            rest += p_this->m_data_vec[i]->tail - p_this->m_data_vec[i]->head;
        }
        down = total - rest;
        std::chrono::time_point<std::chrono::high_resolution_clock> now
                                    = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = now - p_this->m_start;
        percent = 1.0 * down / total;
        int dot = round(percent * total_dot);
        std::stringstream stream;
        stream << "\r";
        stream << std::setw(30) << std::setiosflags(std::ios::left)
               << data->meta->file_name.c_str();
        stream << std::setw(3) << std::setiosflags(std::ios::right)
               << static_cast<uint64_t>(percent * 100) << "%[";
        for (int i = 0; i < total_dot; i++) {
            if (i < dot) stream << "=";
            else if (i == dot) stream << ">";
            else
                stream << " ";
        }
        stream << "]";
        stream << std::setiosflags(std::ios::fixed) << std::setprecision(2);
        if (down < 1024) stream << std::setw(10) << down << "B";
        else if (down < 1024 * 1024)
            stream << std::setw(10) << 1.0 * down / 1024 << "K";
        else if (down < 1024 * 1024 * 1024)
            stream << std::setw(10) << 1.0 * down / 1024 / 1024 << "M";
        else
            stream << std::setw(10) << 1.0 * down / 1024 / 1024 / 1024 << "G";
        stream << std::setiosflags(std::ios::right);
        if (down < 1024)
            stream << std::setw(10) << 1.0 * down / elapsed.count() << "B/s";
        else if (down < 1024 * 1024)
            stream << std::setw(10)
                   << 1.0 * down / 1024 / elapsed.count() << "KB/s";
        else
            stream << std::setw(10) << std::setprecision(1)
                   << 1.0 * down / 1024 / 1024 / elapsed.count() << "MB/s";
        {
            std::lock_guard<std::mutex> lk(g_prog_mutex);
            if (down > g_last) std::cout << stream.str() << std::flush;
        }
        return 0;
    }

    Status GetDownloadStatistic() {
        Status status {};
        for (size_t i = 0; i < m_data_vec.size(); i++) {
            WriteData* data = m_data_vec[i];
            if (data->stat->status < 0)
                status.status = data->stat->status;
            data->stat->total_down = data->head - data->stat->down.first + 1;
            status.total_down.push_back(data->stat->total_down);
            status.stats.push_back(data->stat->status);
            status.curl_codes.push_back(data->stat->curl_code);
            status.response_codes.push_back(data->stat->response_code);
            status.down.push_back(data->stat->down);
            status.total += data->stat->total_down;
        }
        return status;
    }
};

}   // namespace downloader

#endif  // _HTTP_DOWNLOAD_H_    NOLINT