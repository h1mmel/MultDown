#ifndef _DOWNLOAD_STRATEGY_H_   // NOLINT
#define _DOWNLOAD_STRATEGY_H_

#include <string>

#include "downloader/status.h"

namespace downloader {

class DownloadStrategy {
 public:
    DownloadStrategy() = default;

    virtual Status Download(const std::string& url,
                            uint64_t start,
                            uint64_t end) = 0;

    virtual ~DownloadStrategy();
};

}   // namespace downloader

#endif  // _DOWNLOAD_STRATEGY_H_    NOLINT
