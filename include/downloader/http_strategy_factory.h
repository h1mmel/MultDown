#ifndef _HTTP_STRATEGY_FACTORY_H_   // NOLINT
#define _HTTP_STRATEGY_FACTORY_H_   // NOLINT

#include <string>

#include "downloader/strategy_factory.h"
#include "downloader/http_download.h"

namespace downloader {

class HttpStrategyFactory : public StrategyFactory {
 public:
    HttpStrategyFactory() {}
    DownloadStrategy* NewStrategy(int threads_number,
                                  const std::string& path) override {
        return new HttpDownloader(threads_number, path);
    }
    ~HttpStrategyFactory() {}
};

}   // namespace downloader

#endif  // _HTTP_STRATEGY_FACTORY_H_    NOLINT