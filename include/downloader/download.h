#ifndef _DOWNLOAD_H_    //  NOLINT
#define _DOWNLOAD_H_    //  NOLINT

#include <curl/curl.h>

#include <string>
#include <iostream>
#include <thread>   //  NOLINT
#include <fstream>
#include <chrono>   //  NOLINT
#include <iomanip>
#include <cstring>

#include "downloader/proto.h"
#include "downloader/download_strategy.h"
#include "downloader/strategy_factory.h"
#include "downloader/util.h"
#include "downloader/debug_functions.h"
#include "downloader/debug.h"
#include "downloader/http.h"

namespace downloader {

class DownloadStrategy;
class StrategyFactory;

extern bool is_debug;

template<typename ProtoType>
class Downloader {
 public:
    typedef ProtoType value_type;

 public:
    Downloader(int threads_number, const std::string& path);

    Downloader(const Downloader&) = delete;
    Downloader& operator=(const Downloader&) = delete;

    virtual ~Downloader();

    int DoDownload(const std::string& url);

 private:
    struct FileInfo;

    static size_t HeaderCallback(char* buffer, size_t size,
                                 size_t nitems, void* userdata);

    int GetFileInfo(const std::string& url);

    int CreateFile();

    DownloadStrategy* strategy_;
    int threads_;
    std::string save_path_;
    proto::Http* http_header_;
};

template<typename ProtoType>
struct Downloader<ProtoType>::FileInfo {
    FileInfo() : content_type(nullptr), content_length(0) {}
    ~FileInfo() {
        if (content_type != nullptr) delete []content_type;
    }

    char* content_type;
    int64_t content_length;
};

template<typename ProtoType>
size_t Downloader<ProtoType>::HeaderCallback(char* buffer, size_t size,
                                             size_t nitems, void* userdata) {
    proto::Http* data = reinterpret_cast<proto::Http*>(userdata);
    std::string line(buffer, size * nitems - 2);
    std::string::size_type n = 0;

    n = line.find("HTTP/");
    if (n != std::string::npos) {
        if (downloader::is_debug)
            std::cout << "\n---Response Header---" << std::endl;
        n = line.find(" ");
        std::string version;
        if (n != std::string::npos)
            version = line.substr(0, n);
        std::string::size_type m = line.substr(n + 1).find(" ");
        uint32_t code = 0;
        std::string status;
        if (m != std::string::npos) {
            code = std::stoul(line.substr(n + 1, m));
            status = line.substr(n + m + 2);
        }
        data->SetResponseLine(version, code, status);
    }

    n = line.find("Server: ");
    if (n != std::string::npos) {
        n = std::strlen("Server: ");
        data->SetServer(line.substr(n));
    }

    n = line.find("Accept-Ranges: ");
    if (n != std::string::npos) {
        n = std::strlen("Accept-Ranges: ");
        data->SetAcceptRanges(line.substr(n));
    }

    n = line.find("Content-Encoding: ");
    if (n != std::string::npos) {
        n = std::strlen("Content-Encoding: ");
        data->SetContentEncoding(line.substr(n));
    }

    n = line.find("Content-Type: ");
    if (n != std::string::npos) {
        n = std::strlen("Content-Type: ");
        std::string::size_type m = line.find("; ");
        if (m != std::string::npos)
            data->SetContentType(line.substr(n, m - n), line.substr(m + 2));
        else
            data->SetContentType(line.substr(n), std::string());
    }

    n = line.find("Date: ");
    if (n != std::string::npos) {
        n = std::strlen("Date: ");
        data->SetDate(line.substr(n));
    }

    n = line.find("Etag: ");
    if (n != std::string::npos) {
        n = std::strlen("Etag: ");
        data->SetETag(line.substr(n));
    }

    n = line.find("Expires: ");
    if (n != std::string::npos) {
        n = std::strlen("Expires: ");
        data->SetExpires(line.substr(n));
    }

    n = line.find("Last-Modified: ");
    if (n != std::string::npos) {
        n = std::strlen("Last-Modified: ");
        data->SetExpires(line.substr(n));
    }

    n = line.find("Content-Length: ");
    if (n != std::string::npos) {
        n = std::strlen("Content-Length: ");
        data->SetContentLength(
                    static_cast<uint64_t>(std::stoull(line.substr(n))));
    }

    n = line.find("Connection: ");
    if (n != std::string::npos) {
        n = std::strlen("Connection: ");
        data->SetConnection(line.substr(n));
    }

    n = line.find("Location: ");
    if (n != std::string::npos) {
        n = std::strlen("Location: ");
        data->SetLocation(line.substr(n));
    }

    if (downloader::is_debug) {
        if (!line.empty())
            fwrite(buffer, size, nitems, stdout);
        else
            std::cout << "----Response  End----\n" << std::endl;
    }

    return nitems * size;
}

template<typename ProtoType>
int Downloader<ProtoType>::GetFileInfo(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (curl) {
        if (downloader::is_debug) {
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
            curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, debug_callback);
            debug::Debug& dbg = debug::Debug::GetDebugInstance();
            void* data = reinterpret_cast<void*>(dbg.GetFilePointer());
            curl_easy_setopt(curl, CURLOPT_DEBUGDATA, data);
        }
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        // TODO(xxx) : Content-Encoding 字段导致 Content-Length
        // 数据长度是压缩后的长度
        // TODO(xxx) : 解决 chunked 模式
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA,
                                reinterpret_cast<void*>(http_header_));
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return 0;
}

// TODO(xxx) : 稀疏文件，未真正生效
template<typename ProtoType>
int Downloader<ProtoType>::CreateFile() {
    std::ofstream file(save_path_, std::ios::out);
    try {
        file.seekp(http_header_->GetContentLength() - 1);
        file << '\0';
    } catch (const std::ios_base::failure& e) {
        return -1;
    }
    return 0;
}

template<typename ProtoType>
Downloader<ProtoType>::Downloader(int threads_number, const std::string& path)
        : strategy_(nullptr),
          threads_(threads_number),
          save_path_(path),
          http_header_(new proto::Http) {
    StrategyFactory* factory =
                    util::GetFactory<value_type>::GetRealFactory();
    if (save_path_.back() == '/') {
        save_path_ += "index.html";
    }
    this->strategy_ = factory->NewStrategy(threads_number, save_path_);
    delete factory;
    curl_global_init(CURL_GLOBAL_ALL);
    std::cout << "libcurl version: " << curl_version() << std::endl;
}

template<typename ProtoType>
Downloader<ProtoType>::~Downloader() {
    if (strategy_ != nullptr) delete strategy_;
    if (http_header_ != nullptr) delete http_header_;
    curl_global_cleanup();
}

template<typename ProtoType>
int Downloader<ProtoType>::DoDownload(const std::string& url) {
    int ret = GetFileInfo(url);
    if (-1 == ret) {
        std::cerr << "GetFileInfo() failed" << std::endl;
        return -1;
    }
    if (-1 == CreateFile()) {
        std::cerr << "CreateFile() failed" << std::endl;
        return -1;
    }
    std::chrono::time_point<std::chrono::high_resolution_clock> start
                                = std::chrono::high_resolution_clock::now();
    std::time_t t_now =
                std::chrono::high_resolution_clock::to_time_t(start);
    std::tm tm_now {};
    std::cout << std::put_time(localtime_r(&t_now, &tm_now), "--%F %X--")
                << "  " << url << std::endl;
    uint64_t length = http_header_->GetContentLength();
    std::cout << "Length: " << length;
    std::cout << std::setprecision(1);
    std::cout << std::setiosflags(std::ios::fixed);
    if (length < 1024)
        std::cout << " (" << length << "B)";
    else if (length < 1024 * 1024)
        std::cout << " (" << 1.0 * length / 1024 << "K)";
    else if (length < 1024 * 1024 * 1024)
        std::cout << " ("
                    << 1.0 * length / 1024 / 1024 << "M)";
    else
        std::cout << " ("
                    << 1.0 * length / 1024 / 1024 / 1024
                    << "G)";
    std::cout << " [" << http_header_->GetContentType() << "]\n";
    std::cout << "Saving to: " << "'"
                << save_path_.substr(save_path_.find_last_of('/') + 1,
                                save_path_.size())
                << "'\n" << std::endl;
    Status status {};
    if (length >= 1)
        status = strategy_->Download(url, 0, length - 1);
    else
        status = {-1, 0, {}, {}, {CURL_LAST}, {}, {}, "warn: 0 bytes"};
    std::cout << "\n\n------Total Statistics-----\n" << std::endl;
    std::chrono::time_point<std::chrono::high_resolution_clock> done
                                = std::chrono::high_resolution_clock::now();
    t_now = std::chrono::high_resolution_clock::to_time_t(done);
    std::chrono::duration<double> elapsed = done - start;
    std::cout << std::put_time(localtime_r(&t_now, &tm_now), "%F %X");
    if (length < 1024) {
        double rate = 1.0 * length / elapsed.count();
        std::cout << " (" << rate << " B/s)";
    } else if (length < 1024 * 1024) {
        double rate = 1.0 * length / 1024 / elapsed.count();
        std::cout << " (" << rate << " KB/s)";
    } else {
        double rate =
            1.0 * length / 1024 / 1024 / elapsed.count();
        std::cout << " (" << rate << " MB/s)";
    }
    std::cout << " - '" << save_path_ << "' saved [" << status.total
                << "/" << length << "]\n" << std::endl;
    if (status.status != 0) {
        std::cout << status.error_msg << std::endl;
    } else {
        for (int i = 0; i < threads_; i++) {
            std::cout << "Thread "
                        << std::setw(3) << std::setiosflags(std::ios::right)
                        << i << ": ";
            std::cout << std::setw(15) << std::setiosflags(std::ios::right)
                        << status.down[i].first;
            std::cout << "-" << std::resetiosflags(std::ios::right);
            std::cout << std::setw(15) << std::setiosflags(std::ios::left)
                        << status.down[i].second;
            std::cout << ", ";
            uint64_t total =
                        status.down[i].second - status.down[i].first + 1;
            double rate = 1.0 * status.total_down[i] / total;
            std::cout << std::setw(3) << std::setiosflags(std::ios::right)
                        << static_cast<uint64_t>(rate * 100) << "%";
            if (status.curl_codes[i] != CURLE_OK)
                std::cout << ", curl: "
                            << curl_easy_strerror(status.curl_codes[i]);
            else
                std::cout << ", curl: OK";
            std::cout << ", response: " << status.response_codes[i];
            std::cout << ", status: " << status.stats[i];
            std::cout << std::endl;
        }
    }
    std::cout << "\n---------------------------\n" << std::endl;
    return 0;
}

}   // namespace downloader

#endif  // _DOWNLOAD_H_ NOLINT
