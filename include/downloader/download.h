#ifndef _DOWNLOAD_H_    //  NOLINT
#define _DOWNLOAD_H_    //  NOLINT

#include <curl/curl.h>
#include <string>
#include <iostream>
#include <thread>   //  NOLINT
#include <fstream>
#include <chrono>   // NOLINT
#include <iomanip>

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
    explicit Downloader(int threads_number, const std::string& save_path)
            : m_threads(threads_number), m_save_path(save_path) {
        StrategyFactory* factory =
                        util::GetFactory<value_type>::GetRealFactory();
        this->m_strategy = factory->NewStrategy();
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
        uint64_t len = GetFileLength(url);
        if (ULLONG_MAX == len) {
            std::cerr << "GetFileLength() failed" << std::endl;
            return -1;
        }
        if (-1 == CreateFile(len)) {
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
        std::cout << "Length: " << len;
        std::cout << std::setprecision(1);
        std::cout << std::setiosflags(std::ios::fixed);
        if (len < 1024)
            std::cout << " (" << len << "B)\n";
        else if (len < 1024 * 1024)
            std::cout << " (" << 1.0 * len / 1024 << "K)\n";
        else if (len < 1024 * 1024 * 1024)
            std::cout << " (" << 1.0 * len / 1024 / 1024 << "M)\n";
        else
            std::cout << " (" << 1.0 * len / 1024 / 1024 / 1024 << "G)\n";
        std::cout << "Saving to: " << "'" << m_save_path << "'\n" << std::endl;
        uint64_t down = 0;
        if (m_threads > 1) {
            m_strategy->MutiDown(url, m_save_path, 0, len - 1, m_threads);
            down = len - m_strategy->GetRest();
        } else {
            down = m_strategy->Download(url, m_save_path, 0, len - 1);
        }
        std::chrono::time_point<std::chrono::high_resolution_clock> done
                                    = std::chrono::high_resolution_clock::now();
        t_now = std::chrono::high_resolution_clock::to_time_t(done);
        std::chrono::duration<double> elapsed = done - start;
        std::cout << "\n\n"
                  << std::put_time(localtime_r(&t_now, &tm_now), "%c");
        if (len < 1024) {
            double rate = 1.0 * len / elapsed.count();
            std::cout << " (" << rate << " B/s)";
        } else if (len < 1024 * 1024) {
            double rate = 1.0 * len / 1024 / elapsed.count();
            std::cout << " (" << rate << " KB/s)";
        } else {
            double rate = 1.0 * len / 1024 / 1024 / elapsed.count();
            std::cout << " (" << rate << " MB/s)";
        }
        std::cout << " - '" << m_save_path << "' saved [" << down
                  << "/" << len << "]\n" << std::endl;
        return 0;
    }

 private:
    DownloadStrategy* m_strategy;
    int m_threads;
    std::string m_save_path;
    std::ofstream m_fstream;

    uint64_t GetFileLength(const std::string& url) {
        double len = -1;
        CURL* curl = curl_easy_init();
        CURLcode res;
        if (curl) {
            struct curl_slist* list = nullptr;
            list = curl_slist_append(list, "Connection: close");
            // curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            // curl_easy_setopt(curl, CURLOPT_HEADER, 1);
            curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
            // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
            res = curl_easy_perform(curl);
            if (CURLE_OK == res) {
                res = curl_easy_getinfo(curl,
                                CURLINFO_CONTENT_LENGTH_DOWNLOAD, &len);
                if (CURLE_OK != res) return ULLONG_MAX;
            }
            curl_slist_free_all(list);
            curl_easy_cleanup(curl);
        }
        return len;
    }

    int CreateFile(uint64_t len) {
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
