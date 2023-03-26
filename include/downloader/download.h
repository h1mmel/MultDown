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

namespace downloader {

class DownloadStrategy;
class StrategyFactory;

template<typename ProtoType>
class Downloader {
 public:
    typedef ProtoType value_type;

 public:
    explicit Downloader(int threads_number, const std::string& path)
            : m_strategy(nullptr),
              m_threads(threads_number),
              m_save_path(path),
              m_info() {
        StrategyFactory* factory =
                        util::GetFactory<value_type>::GetRealFactory();
        if (m_save_path.back() == '/') {
            m_save_path += "index.html";
        }
        this->m_strategy = factory->NewStrategy(threads_number, m_save_path);
        delete factory;
        curl_global_init(CURL_GLOBAL_ALL);
        std::cout << "libcurl version: " << curl_version() << std::endl;
    }

    Downloader(const Downloader&) = delete;
    Downloader& operator=(const Downloader&) = delete;

    virtual ~Downloader() {
        delete this->m_strategy;
        curl_global_cleanup();
    }

    int DoDownload(const std::string& url) {
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
                  << url << std::endl;
        uint64_t length = m_info.content_length;
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
        if (m_info.content_type == nullptr) std::cout << " [NULL]\n";
        else
            std::cout << " [" << m_info.content_type << "]\n";
        std::cout << "Saving to: " << "'"
                  << m_save_path.substr(m_save_path.find_last_of('/') + 1,
                                    m_save_path.size())
                  << "'\n" << std::endl;
        Status status = m_strategy->Download(url, 0, length - 1);
        std::cout << "\n\n\n------Total Statistics-----\n" << std::endl;
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
        std::cout << " - '" << m_save_path << "' saved [" << status.total
                  << "/" << length << "]\n" << std::endl;
        for (int i = 0; i < m_threads; i++) {
            std::cout << "Thread "
                      << std::setw(3) << std::setiosflags(std::ios::right)
                      << i << ": ";
            std::cout << std::setw(15) << std::setiosflags(std::ios::right)
                      << status.down[i].first;
            std::cout << "-" << std::resetiosflags(std::ios::right);
            std::cout << std::setw(15) << std::setiosflags(std::ios::left)
                      << status.down[i].second;
            std::cout << ", ";
            uint64_t total = status.down[i].second - status.down[i].first + 1;
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
        std::cout << "\n--------------------------\n" << std::endl;
        return 0;
    }

 private:
    DownloadStrategy* m_strategy;
    int m_threads;
    std::string m_save_path;
    struct FileInfo {
        char* content_type;
        uint64_t content_length;
        FileInfo() : content_type(nullptr), content_length(0) {}
        ~FileInfo() {
            if (content_type != nullptr) delete []content_type;
        }
    } m_info;

    int GetFileInfo(const std::string& url) {
        CURL* curl = curl_easy_init();
        CURLcode res;
        if (curl) {
            struct curl_slist* list = nullptr;
            // list = curl_slist_append(list, "Connection: close");
            // curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, debug_callback);
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            // curl_easy_setopt(curl, CURLOPT_HEADER, 1);
            curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
            // curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
            // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
            res = curl_easy_perform(curl);
            if (CURLE_OK == res) {
                double len = 0;
                if (CURLE_OK != curl_easy_getinfo(curl,
                            CURLINFO_CONTENT_LENGTH_DOWNLOAD, &len))
                    return -1;
                char* type = nullptr;
                if (CURLE_OK != curl_easy_getinfo(curl,
                            CURLINFO_CONTENT_TYPE, &type))
                    return -1;
                m_info.content_length = len;
                m_info.content_type = new char[std::strlen(type) + 1];
                std::memcpy(m_info.content_type, type, std::strlen(type) + 1);
            }
            curl_slist_free_all(list);
            curl_easy_cleanup(curl);
        }
        return 0;
    }

    int CreateFile() {
        std::ofstream file(m_save_path, std::ios::out);
        try {
            file.seekp(m_info.content_length - 1);
            file << '\0';
        } catch (const std::ios_base::failure& e) {
            return -1;
        }
        return 0;
    }
};

}   // namespace downloader

#endif  // _DOWNLOAD_H_ NOLINT
