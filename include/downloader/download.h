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
#include <sstream>

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

    void PreDisplay(std::string url, uint64_t length);
    void Display(uint64_t length, Status* status);

    DownloadStrategy* strategy_;
    int threads_;
    std::string save_path_;
    proto::Http* http_header_;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
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

    n = line.find("Transfer-Encoding: ");
    if (n != std::string::npos) {
        n = std::strlen("Transfer-Encoding: ");
        data->SetTransferEncoding(line.substr(n));
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
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return 0;
}

// TODO(xxx) : 稀疏文件，未真正生效
template<typename ProtoType>
int Downloader<ProtoType>::CreateFile() {
    std::ofstream file(save_path_, std::ios::out);
    uint64_t len = http_header_->GetActualContentLength();
    try {
        file.seekp(len ? len - 1 : 0);
        file << '\0';
    } catch (const std::ios_base::failure& e) {
        return -1;
    }
    return 0;
}

template<typename ProtoType>
void Downloader<ProtoType>::PreDisplay(std::string url,
                                       uint64_t length) {
    start_ = std::chrono::high_resolution_clock::now();
    std::time_t t_now =
                std::chrono::high_resolution_clock::to_time_t(start_);
    std::tm tm_now {};
    std::string real_url = url;
    if (!http_header_->GetLocation().empty())
        real_url = http_header_->GetLocation();
    std::cout << std::put_time(localtime_r(&t_now, &tm_now), "--%F %X--")
                << "  " << real_url << std::endl;
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
}

template<typename ProtoType>
void Downloader<ProtoType>::Display(uint64_t length, Status* status) {
    std::stringstream stream;
    stream << "\n\n------Total Statistics-----\n\n";
    std::chrono::time_point<std::chrono::high_resolution_clock> done
                                = std::chrono::high_resolution_clock::now();
    std::time_t t_now = std::chrono::high_resolution_clock::to_time_t(done);
    std::chrono::duration<double> elapsed = done - start_;
    std::tm tm_now {};
    stream << std::put_time(localtime_r(&t_now, &tm_now), "%F %X");
    if (length == 0) {
        length = status->total;
        if (length < 1024)
            stream << " (" << 1.0 * length / elapsed.count() << " B/s)";
        else if (length < 1024 * 1024)
            stream << " (" << 1.0 * length / 1024 / elapsed.count() << " KB/s)";
        else
            stream << " ("
                   << 1.0 * length / 1024 / 1024 / elapsed.count() << " MB/s)";
        stream << " - '" << save_path_
               << "' saved [" << status->total << "]\n\n";
        if (status->status != 0) {
            stream << status->error_msg << "\n";
        } else {
            stream << "Thread  0: 0 - ";
            stream << std::setiosflags(std::ios::left);
            stream << std::setw(15) << status->total - 1;
            stream << ", 100%, curl: ";
            if (status->curl_codes[0] != CURLE_OK)
                stream << curl_easy_strerror(status->curl_codes[0]);
            else
                stream << "OK";
            stream << ", response: " << status->response_codes[0];
            stream << ", status: " << status->stats[0] << "\n";
        }
    } else {
        stream << std::setiosflags(std::ios::fixed) << std::setprecision(2);
        if (length < 1024)
            stream << " (" << 1.0 * length / elapsed.count() << " B/s)";
        else if (length < 1024 * 1024)
            stream << " (" << 1.0 * length / 1024 / elapsed.count() << " KB/s)";
        else
            stream << " ("
                   << 1.0 * length / 1024 / 1024 / elapsed.count() << " MB/s)";
        stream << " - '" << save_path_ << "' saved [" << status->total
               << "/" << length << "]\n\n";
        if (status->status != 0) {
            stream << status->error_msg << "\n";
        } else {
            for (int i = 0; i < threads_; i++) {
                stream << std::setiosflags(std::ios::right);
                stream << "Thread " << std::setw(3) << i << ": ";
                stream << std::setw(15) << status->down[i].first;
                stream << std::resetiosflags(std::ios::right);
                stream << std::setiosflags(std::ios::left);
                stream << "-";
                stream << std::setw(15) << status->down[i].second;
                stream << ", ";
                uint64_t total =
                            status->down[i].second - status->down[i].first + 1;
                double rate = 1.0 * status->total_down[i] / total;
                stream << std::setw(3) << std::setiosflags(std::ios::right)
                    << static_cast<uint64_t>(rate * 100) << "%";
                if (status->curl_codes[i] != CURLE_OK)
                    stream << ", curl: "
                        << curl_easy_strerror(status->curl_codes[i]);
                else
                    stream << ", curl: OK";
                stream << ", response: " << status->response_codes[i];
                stream << ", status: " << status->stats[i] << "\n";
            }
        }
    }
    stream << "\n---------------------------\n\n";
    std::cout << stream.str() << std::flush;
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
    uint64_t length = http_header_->GetActualContentLength();
    if (length == 0) {
        strategy_->SetThreadsNumber(1);
        threads_ = 1;
    }
    std::string real_url = url;
    if (!http_header_->GetLocation().empty())
        real_url = http_header_->GetLocation();
    PreDisplay(real_url, length);
    Status status {};
    status = strategy_->Download(real_url, 0, length - 1);
    Display(length, &status);
    return 0;
}

}   // namespace downloader

#endif  // _DOWNLOAD_H_ NOLINT
