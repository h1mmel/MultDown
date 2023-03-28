#ifndef _UTIL_H_    // NOLINT
#define _UTIL_H_    // NOLINT

#include "downloader/http.h"
#include "downloader/http_download_strategy_factory.h"

namespace downloader {

namespace util {

template<typename ProtoType> struct GetFactory;

template<> struct GetFactory<proto::Http> {
    static downloader::StrategyFactory* GetRealFactory() {
        return new downloader::HttpDownloadStrategyFactory();
    }
};

}   // namespace util

}   // namespace downloader

#endif  // _UTIL_H_ NOLINT
