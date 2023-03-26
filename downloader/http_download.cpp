#include "downloader/http_download.h"

namespace downloader {

Status HttpDownloader::Download(const std::string& url,
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
        threads_arr.push_back(std::thread(&HttpDownloader::WorkerThread,
                                this, m_data_vec[i]));
    }

    {
        downloader::thread::JoinThreads join(&threads_arr);
    }

    return GetDownloadStatistic();
}

}   // namespace downloader
