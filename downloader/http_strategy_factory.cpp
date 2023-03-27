#include "downloader/http_strategy_factory.h"

namespace downloader {

HttpStrategyFactory::~HttpStrategyFactory() = default;

DownloadStrategy* HttpStrategyFactory::NewStrategy(
        int threads_number, const std::string& path) {
    return new HttpDownloader(threads_number, path);
}

}   // namespace downloader
