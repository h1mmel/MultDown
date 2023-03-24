#ifndef _DOWNLOAD_H_    //  NOLINT
#define _DOWNLOAD_H_    //  NOLINT

#include <curl/curl.h>

#include <string>
#include <iostream>
#include <thread>   //  NOLINT
#include <fstream>
#include <chrono>   // NOLINT
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
        if (-1 == CreateFile(m_info.content_length)) {
            std::cerr << "CreateFile() failed" << std::endl;
            return -1;
        }
        std::chrono::time_point<std::chrono::high_resolution_clock> start
                                    = std::chrono::high_resolution_clock::now();
        std::time_t t_now =
                    std::chrono::high_resolution_clock::to_time_t(start);
        std::tm tm_now {};
        std::cout << "--" << std::put_time(localtime_r(&t_now, &tm_now), "%c")
                  << "--  " << url << std::endl;
        std::cout << "Length: " << m_info.content_length;
        std::cout << std::setprecision(1);
        std::cout << std::setiosflags(std::ios::fixed);
        if (m_info.content_length < 1024)
            std::cout << " (" << m_info.content_length << "B)";
        else if (m_info.content_length < 1024 * 1024)
            std::cout << " (" << 1.0 * m_info.content_length / 1024 << "K)";
        else if (m_info.content_length < 1024 * 1024 * 1024)
            std::cout << " ("
                      << 1.0 * m_info.content_length / 1024 / 1024 << "M)";
        else
            std::cout << " ("
                      << 1.0 * m_info.content_length / 1024 / 1024 / 1024
                      << "G)";
        std::cout << " [" << m_info.content_type << "]\n";
        std::cout << "Saving to: " << "'"
                  << m_save_path.substr(m_save_path.find_last_of('/') + 1,
                                    m_save_path.size())
                  << "'\n" << std::endl;
        uint64_t down = m_strategy->Download(url, 0, m_info.content_length - 1);
        std::chrono::time_point<std::chrono::high_resolution_clock> done
                                    = std::chrono::high_resolution_clock::now();
        t_now = std::chrono::high_resolution_clock::to_time_t(done);
        std::chrono::duration<double> elapsed = done - start;
        std::cout << "\n\n"
                  << std::put_time(localtime_r(&t_now, &tm_now), "%c");
        if (m_info.content_length < 1024) {
            double rate = 1.0 * m_info.content_length / elapsed.count();
            std::cout << " (" << rate << " B/s)";
        } else if (m_info.content_length < 1024 * 1024) {
            double rate = 1.0 * m_info.content_length / 1024 / elapsed.count();
            std::cout << " (" << rate << " KB/s)";
        } else {
            double rate =
                1.0 * m_info.content_length / 1024 / 1024 / elapsed.count();
            std::cout << " (" << rate << " MB/s)";
        }
        std::cout << " - '" << m_save_path << "' saved [" << down
                  << "/" << m_info.content_length << "]\n" << std::endl;
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
            list = curl_slist_append(list, "Connection: close");
            // curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, debug_callback);
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            // curl_easy_setopt(curl, CURLOPT_HEADER, 1);
            curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
            // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
            res = curl_easy_perform(curl);
            if (CURLE_OK == res) {
                double len = 0;
                char* type = nullptr;
                if (curl_easy_getinfo(curl,
                            CURLINFO_CONTENT_LENGTH_DOWNLOAD, &len))
                    return -1;
                if (curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &type))
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

    int CreateFile(uint64_t len) {
        if (m_save_path.back() == '/') {
            m_save_path += "index.html";
        }
        std::ofstream file(m_save_path, std::ios::out);
        try {
            file.seekp(len - 1);
            file << '\0';
        } catch (const std::ios_base::failure& e) {
            return -1;
        }
        return 0;
    }
};

}   // namespace downloader

#endif  // _DOWNLOAD_H_ NOLINT
