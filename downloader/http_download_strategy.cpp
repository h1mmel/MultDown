#include <curl/curl.h>
#include <math.h>
#include <sys/mman.h>

#include <thread>   // NOLINT
#include <sstream>
#include <iostream>
#include <cstring>  // memcopy
#include <iomanip>  // std::setw std::setiosflags std::setprecision

#include "downloader/http_download_strategy.h"
#include "downloader/debug_functions.h"
#include "downloader/join_threads.h"
#include "downloader/download.h"
#include "downloader/debug.h"

namespace downloader {

static uint64_t g_last = 0;

struct HttpDownloadStrategy::MetaData {
    MetaData() : start(0),
                 end(0),
                 fp(nullptr),
                 base(nullptr),
                 m_this(nullptr)
    {}

    uint64_t start;
    uint64_t end;
    std::string file_name;
    std::string url;
    FILE* fp;
    uint8_t* base;
    void* m_this;
};

struct HttpDownloadStrategy::WriteData {
    WriteData() : meta(nullptr),
                  stat(nullptr),
                  head(0),
                  tail(0),
                  curl(nullptr)
    {}

    MetaData* meta;
    MetaStatus* stat;
    uint64_t head;
    uint64_t tail;
    CURL* curl;
};

void HttpDownloadStrategy::WorkerThread(WriteData* data) {
    // stream << syscall(SYS_gettid) << std::endl;
    CURL* curl = curl_easy_init();
    if (curl) {
        data->curl = curl;
        char range[64] = {0};
        snprintf(range, sizeof (range), "%lu-%lu",
                    data->head, data->tail);

        // TODO(xxx) : 解决 302 跳转问题
        if (downloader::is_debug) {
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
            curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, debug_callback);
            debug::Debug& dbg = debug::Debug::GetDebugInstance();
            void* data = reinterpret_cast<void*>(dbg.GetFilePointer());
            curl_easy_setopt(curl, CURLOPT_DEBUGDATA, data);
        }
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, debug_callback);
        curl_easy_setopt(curl, CURLOPT_URL, data->meta->url.c_str());
        // curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, LockWriteFunc);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, LockFreeWriteFunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                                        reinterpret_cast<void*>(data));
        HttpDownloadStrategy* http
                = reinterpret_cast<HttpDownloadStrategy*>(data->meta->m_this);
        if (http->m_threads_number != 1)
            curl_easy_setopt(curl, CURLOPT_RANGE, range);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressFunc);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, data);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

        std::call_once(m_once, [this](){
            m_start = std::chrono::high_resolution_clock::now();
        });

        data->stat->curl_code = curl_easy_perform(data->curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE,
                                        &data->stat->response_code);
        curl_easy_cleanup(data->curl);
    }
}

size_t HttpDownloadStrategy::LockWriteFunc(void* ptr, size_t size, size_t nmemb,
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

size_t HttpDownloadStrategy::LockFreeWriteFunc(void* ptr, size_t size,
                                               size_t nmemb, void* user_data) {
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

int HttpDownloadStrategy::ProgressFunc(void *clientp,
                                       curl_off_t dltotal, curl_off_t dlnow,
                                       curl_off_t ultotal, curl_off_t ulnow) {
    WriteData* data = reinterpret_cast<WriteData*>(clientp);
    HttpDownloadStrategy* p_this =
                reinterpret_cast<HttpDownloadStrategy*>(data->meta->m_this);
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
        if (down > g_last) {
            std::cout << stream.str() << std::flush;
            g_last = down;
        }
    }
    return 0;
}

Status HttpDownloadStrategy::GetDownloadStatistic() const {
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

HttpDownloadStrategy::HttpDownloadStrategy(int threads_number,
                                           const std::string& path)
    : m_threads_number(threads_number),
      m_path(path),
      m_meta(new MetaData) {
    for (int i = 0; i < threads_number; i++) {
        m_data_vec.push_back(new WriteData);
    }
    m_meta->file_name =
            path.substr(path.find_last_of('/') + 1, path.size());
}

HttpDownloadStrategy::~HttpDownloadStrategy() {
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

Status HttpDownloadStrategy::Download(const std::string& url,
                                      uint64_t start,
                                      uint64_t end) {
    m_meta->fp = fopen(m_path.c_str(), "r+");
    if (m_meta->fp == nullptr)
        return {-1, 0, {}, {}, {CURL_LAST}, {}, {}, {strerror(errno)}};

    uint64_t len = end - start + 1;
    uint64_t size = len / m_threads_number;
    uint64_t head = start, tail = 0;

    uint8_t* base = reinterpret_cast<uint8_t*>(mmap(nullptr, len, PROT_WRITE,
                                    MAP_SHARED, m_meta->fp->_fileno, 0));
    if (base == MAP_FAILED)
        return {-1, 0, {}, {}, {CURL_LAST}, {}, {}, {strerror(errno)}};

    m_meta->base = base;
    m_meta->start = start;
    m_meta->end = end;
    m_meta->url = url;
    m_meta->m_this = reinterpret_cast<void*>(this);

    std::vector<std::thread> threads_arr;
    for (int i = 0; i < m_threads_number; i++) {
        if (len - head < size || (len - head > size && len - head < 2 * size)) {
            tail = len - 1;
        } else {
            tail = head + size - 1;
        }
        m_data_vec[i]->meta = m_meta;
        m_data_vec[i]->stat = new MetaStatus;
        m_data_vec[i]->stat->down = {head, tail};
        m_data_vec[i]->head = head;
        m_data_vec[i]->tail = tail;
        head += size;
        threads_arr.push_back(std::thread(&HttpDownloadStrategy::WorkerThread,
                                this, m_data_vec[i]));
    }

    {
        downloader::thread::JoinThreads join(&threads_arr);
    }

    return GetDownloadStatistic();
}

}   // namespace downloader
