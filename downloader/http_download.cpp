#include "downloader/http_download.h"

namespace downloader {

uint64_t HttpDownloader::Download(const std::string& url,
                                  uint64_t start,
                                  uint64_t end) {
    uint64_t len = end - start + 1;
    uint64_t size = len / m_threads_number;
    uint64_t head = start, tail = 0;
    std::vector<std::thread> threads_arr;
    FILE* fp = fopen(m_path.c_str(), "r+");
    if (fp == nullptr) return 0;
    m_fp = fp;

    uint8_t* base = reinterpret_cast<uint8_t*>(mmap(nullptr, len, PROT_WRITE,
                                                MAP_SHARED, fp->_fileno, 0));
    if (base == MAP_FAILED) return 0;

    for (int i = 0; i < m_threads_number; i++) {
        if (len - head < size || (len - head > size && len - head < 2 * size)) {
            tail = len - 1;
        } else {
            tail = head + size - 1;
        }
        m_data_vec[i]->file_name = m_file_name.c_str();
        m_data_vec[i]->head = head;
        m_data_vec[i]->tail = tail;
        m_data_vec[i]->url = url.c_str();
        m_data_vec[i]->fp = fp;
        m_data_vec[i]->m_this = this;
        m_data_vec[i]->base = base;
        head += size;
        threads_arr.push_back(std::thread(&HttpDownloader::WorkerThread,
                                this, m_data_vec[i]));
    }

    {
        downloader::thread::JoinThreads join(&threads_arr);
    }

    uint64_t rest = 0;
    for (size_t i = 0; i < m_data_vec.size(); i++) {
        rest += m_data_vec[i]->tail - m_data_vec[i]->head;
    }

    return len - rest;
}

}   // namespace downloader
