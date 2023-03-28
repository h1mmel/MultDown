#ifndef _HTTP_STRATEGY_FACTORY_H_   // NOLINT
#define _HTTP_STRATEGY_FACTORY_H_   // NOLINT

#include <string>

#include "downloader/strategy_factory.h"
#include "downloader/http_download_strategy.h"

namespace downloader {

class HttpStrategyFactory : public StrategyFactory {
 public:
    HttpStrategyFactory() = default;

    HttpStrategyFactory(const HttpStrategyFactory&) = delete;
    HttpStrategyFactory& operator=(const HttpStrategyFactory&) = delete;

    ~HttpStrategyFactory() override;

    DownloadStrategy* NewStrategy(int threads_number,
                                  const std::string& path) override;
};

}   // namespace downloader

#endif  // _HTTP_STRATEGY_FACTORY_H_    NOLINT