#ifndef _DOWNLOAD_STRATEGY_H_   // NOLINT
#define _DOWNLOAD_STRATEGY_H_

#include <string>

#include "downloader/proto.h"

namespace downloader {

class DownloadStrategy {
 public:
    DownloadStrategy() {}

    virtual uint64_t Download(const std::string& url,
                              uint64_t start,
                              uint64_t end) = 0;

    virtual ~DownloadStrategy() = default;
};

}   // namespace downloader

#endif  // _DOWNLOAD_STRATEGY_H_    NOLINT
