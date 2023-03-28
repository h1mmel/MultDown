#ifndef _HTTP_DOWNLOAD_STRATEGY_FACTORY_H_   // NOLINT
#define _HTTP_DOWNLOAD_STRATEGY_FACTORY_H_   // NOLINT

#include <string>

#include "downloader/strategy_factory.h"
#include "downloader/http_download_strategy.h"

namespace downloader {

class HttpDownloadStrategyFactory : public StrategyFactory {
 public:
    HttpDownloadStrategyFactory() = default;

    HttpDownloadStrategyFactory(const HttpDownloadStrategyFactory&) = delete;
    HttpDownloadStrategyFactory&
                      operator=(const HttpDownloadStrategyFactory&) = delete;

    ~HttpDownloadStrategyFactory() override;

    DownloadStrategy* NewStrategy(int threads_number,
                                  const std::string& path) override;
};

}   // namespace downloader

#endif  // _HTTP_DOWNLOAD_STRATEGY_FACTORY_H_    NOLINT