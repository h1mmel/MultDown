#ifndef _HTTP_STRATEGY_FACTORY_H_   // NOLINT
#define _HTTP_STRATEGY_FACTORY_H_   // NOLINT

#include "downloader/strategy_factory.h"
#include "downloader/http_download.h"

namespace downloader {

class HttpStrategyFactory : public StrategyFactory {
 public:
    HttpStrategyFactory() {}
    DownloadStrategy* NewStrategy() override {
        return new HttpDownloader();
    }
    ~HttpStrategyFactory() {}
};

}   // namespace downloader

#endif  // _HTTP_STRATEGY_FACTORY_H_    NOLINT