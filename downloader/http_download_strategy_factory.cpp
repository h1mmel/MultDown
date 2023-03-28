#include "downloader/http_download_strategy_factory.h"

namespace downloader {

HttpDownloadStrategyFactory::~HttpDownloadStrategyFactory() = default;

DownloadStrategy* HttpDownloadStrategyFactory::NewStrategy(
        int threads_number, const std::string& path) {
    return new HttpDownloadStrategy(threads_number, path);
}

}   // namespace downloader
