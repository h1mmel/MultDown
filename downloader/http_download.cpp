#include "downloader/http_download.h"

namespace downloader {

uint64_t HttpDownloader::Download(const std::string& url,
                            const std::string& file_path,
                            uint64_t start,
                            uint64_t end) {
    std::vector<std::thread> threads_arr;
    FILE* fp = fopen(file_path.c_str(), "r+");
    CURL* curl = curl_easy_init();
    WriteData* data = new WriteData;
    data->file_path = file_path.c_str();
    data->head = start;
    data->tail = end;
    data->url = url.c_str();
    data->fp = fp;
    m_fp = fp;
    data->m_this = this;
    m_data_vec.push_back(data);
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, debug_callback);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressFunc);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, data);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
        // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        CURLcode res = curl_easy_perform(curl);
        if (CURLE_OK != res) {
            std::cerr << "\ncurl_easy_perform() failed: "
                        << curl_easy_strerror(res) << std::endl;
        }
        int32_t status = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
        if (200 != status) {
            std::cerr << "\nerror status: " << status << std::endl;
        }
        curl_easy_cleanup(curl);
    }
    uint64_t rest = data->tail - data->head;
    return rest == 0 ? data->tail + 1 : data->head - start + 1;
}

int HttpDownloader::MutiDown(const std::string& url,
                                  const std::string& file_path,
                                  uint64_t start,
                                  uint64_t end,
                                  int threads) {
    uint64_t len = end - start + 1;
    uint64_t size = len / threads;
    uint64_t head = start, tail = 0;
    std::vector<std::thread> threads_arr;
    FILE* fp = fopen(file_path.c_str(), "r+");
    for (int i = 0; i < threads; i++) {
        if (len - head < size || (len - head > size && len - head < 2 * size)) {
            tail = len - 1;
        } else {
            tail = head + size - 1;
        }
        WriteData* data = new WriteData;
        data->file_path = file_path.c_str();
        data->head = head;
        data->tail = tail;
        data->url = url.c_str();
        data->fp = fp;
        m_fp = fp;
        data->m_this = this;
        head += size;
        m_data_vec.push_back(data);
        threads_arr.push_back(std::thread(&HttpDownloader::WorkerThread,
                                this, data));
    }

    downloader::thread::JoinThreads join(&threads_arr);

    return 0;
}

uint64_t HttpDownloader::GetRest() {
    uint64_t rest = 0;
    for (size_t i = 0; i < m_data_vec.size(); i++) {
        rest += m_data_vec[i]->tail - m_data_vec[i]->head;
    }
    return rest;
}

}   // namespace downloader
