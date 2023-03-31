#include <curl/curl.h>
#include <math.h>
#include <sys/mman.h>

#include <thread>   // NOLINT
#include <iostream>
#include <cstring>  // memcopy

#include "downloader/http_download_strategy.h"
#include "downloader/debug_functions.h"
#include "downloader/join_threads.h"
#include "downloader/download.h"
#include "downloader/display.h"
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

        if (downloader::is_debug) {
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
            curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, debug_callback);
            debug::Debug& dbg = debug::Debug::GetDebugInstance();
            void* data = reinterpret_cast<void*>(dbg.GetFilePointer());
            curl_easy_setopt(curl, CURLOPT_DEBUGDATA, data);
        }

        curl_easy_setopt(curl, CURLOPT_URL, data->meta->url.c_str());
        if (data->meta->base != nullptr)
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, LockFreeWriteFunc);
        else
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                                        reinterpret_cast<void*>(data));

        HttpDownloadStrategy* http
                = reinterpret_cast<HttpDownloadStrategy*>(data->meta->m_this);
        if (http->threads_number_ != 1) {
            char range[64] = {0};
            snprintf(range, sizeof (range), "%lu-%lu", data->head, data->tail);
            curl_easy_setopt(curl, CURLOPT_RANGE, range);
        }

        if (data->meta->end != UINT64_MAX)
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressFunc);
        else
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressFunc2);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA,
                                        reinterpret_cast<void*>(data));
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

        std::call_once(once_, [this](){
            start_ = std::chrono::high_resolution_clock::now();
        });

        data->stat->curl_code = curl_easy_perform(data->curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE,
                                        &data->stat->response_code);
        curl_easy_cleanup(data->curl);
    }
}

size_t HttpDownloadStrategy::WriteFunc(void* ptr, size_t size, size_t nmemb,
                                       void* user_data) {
    WriteData* data = reinterpret_cast<WriteData*>(user_data);
    size_t written = 0;
    fseek(data->meta->fp, data->head, SEEK_SET);
    written = fwrite(ptr, size, nmemb, data->meta->fp);
    data->head += size * nmemb - 1;
    data->tail = data->head;
    return written;
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
    for (uint64_t i = 0; i < p_this->data_vec_.size(); i++) {
        rest += p_this->data_vec_[i]->tail - p_this->data_vec_[i]->head;
    }
    down = total - rest;
    std::chrono::time_point<std::chrono::high_resolution_clock> now
                                = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = now - p_this->start_;
    percent = 1.0 * down / total;
    int dot = static_cast<int>(round(percent * total_dot));
    std::stringstream stream;
    stream << "\r";
    stream << std::setw(30) << std::setiosflags(std::ios::left)
           << data->meta->file_name.c_str();
    stream << std::setw(3) << std::setiosflags(std::ios::right)
           << static_cast<int64_t>(percent * 100) << "%[";
    for (int i = 0; i < total_dot; i++) {
        if (i < dot) stream << "=";
        else if (i == dot) stream << ">";
        else
            stream << " ";
    }
    stream << "]";
    util::ShowSize(stream, 10, 2, down);
    util::ShowRate(stream, 10, 1, down, elapsed);
    {
        std::lock_guard<std::mutex> lk(g_prog_mutex);
        if (down > g_last) {
            std::cout << stream.str() << std::flush;
            g_last = down;
        }
    }
    return 0;
}

int HttpDownloadStrategy::ProgressFunc2(void *clientp,
                                        curl_off_t dltotal, curl_off_t dlnow,
                                        curl_off_t ultotal, curl_off_t ulnow) {
    WriteData* data = reinterpret_cast<WriteData*>(clientp);
    HttpDownloadStrategy* p_this =
                reinterpret_cast<HttpDownloadStrategy*>(data->meta->m_this);
    uint64_t down = data->head;

    std::chrono::time_point<std::chrono::high_resolution_clock> now
                                = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = now - p_this->start_;

    std::stringstream stream;
    stream << "\r";
    stream << std::setw(30) << std::setiosflags(std::ios::left)
           << data->meta->file_name.c_str();

    util::ShowSize(stream, 0, 2, down);
    stream << "    ";
    util::ShowRate(stream, 0, 1, down, elapsed);

    if (down > g_last) {
        std::cout << stream.str() << std::flush;
        g_last = down;
    }
    return 0;
}

Status HttpDownloadStrategy::GetDownloadStatistic() const {
    Status status {};
    for (int i = 0; i < threads_number_; i++) {
        WriteData* data = data_vec_[i];
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
    : threads_number_(threads_number),
      path_(path),
      meta_(new MetaData) {
    for (int i = 0; i < threads_number; i++) {
        data_vec_.push_back(new WriteData);
    }
    meta_->file_name =
            path.substr(path.find_last_of('/') + 1, path.size());
}

HttpDownloadStrategy::~HttpDownloadStrategy() {
    for (uint64_t i = 0; i < data_vec_.size(); i++) {
        if (data_vec_[i]->stat != nullptr)
            delete data_vec_[i]->stat;
        delete data_vec_[i];
    }
    if (meta_ != nullptr && meta_->fp != nullptr) {
        fclose(meta_->fp);
    }
    if (meta_ != nullptr && meta_->base != nullptr) {
        int ret = munmap(meta_->base, meta_->end - meta_->start + 1);
        if (ret != 0) perror("munmap error");
    }
    if (meta_ != nullptr) delete meta_;
}

int HttpDownloadStrategy::SetThreadsNumber(int num) {
    if (num > 0 && num < 10000)
        threads_number_ = num;
    else
        return -1;
    return 0;
}

Status HttpDownloadStrategy::Download(const std::string& url,
                                      uint64_t start,
                                      uint64_t end) {
    meta_->fp = fopen(path_.c_str(), "r+");
    if (meta_->fp == nullptr)
        return {-1, 0, {}, {}, {CURL_LAST}, {}, {}, {strerror(errno)}};

    uint64_t len = 0, size = 0, head = 0, tail = 0;
    uint8_t* base = nullptr;
    if (end != UINT64_MAX) {
        len = end - start + 1;
        size = len / threads_number_;
        head = start;
        base = reinterpret_cast<uint8_t*>(mmap(nullptr, len, PROT_WRITE,
                                            MAP_SHARED, meta_->fp->_fileno, 0));
        if (base == MAP_FAILED)
            return {-1, 0, {}, {}, {CURL_LAST}, {}, {}, {strerror(errno)}};
    }

    meta_->base = base;
    meta_->start = start;
    meta_->end = end;
    meta_->url = url;
    meta_->m_this = reinterpret_cast<void*>(this);

    std::vector<std::thread> threads_arr;
    if (meta_->end == UINT64_MAX) {
        data_vec_[0]->meta = meta_;
        data_vec_[0]->stat = new MetaStatus;
        data_vec_[0]->stat->down = {head, tail};
        data_vec_[0]->head = head;
        data_vec_[0]->tail = tail;
        threads_arr.push_back(std::thread(&HttpDownloadStrategy::WorkerThread,
                                          this, data_vec_[0]));
    } else {
        for (int i = 0; i < threads_number_; i++) {
            if (len - head < size
                || (len - head > size && len - head < 2 * size)) {
                tail = len - 1;
            } else {
                tail = head + size - 1;
            }
            data_vec_[i]->meta = meta_;
            data_vec_[i]->stat = new MetaStatus;
            data_vec_[i]->stat->down = {head, tail};
            data_vec_[i]->head = head;
            data_vec_[i]->tail = tail;
            head += size;
            threads_arr.push_back(
                std::thread(&HttpDownloadStrategy::WorkerThread,
                this, data_vec_[i]));
        }
    }

    {
        downloader::thread::JoinThreads join(&threads_arr);
    }

    return GetDownloadStatistic();
}

}   // namespace downloader
