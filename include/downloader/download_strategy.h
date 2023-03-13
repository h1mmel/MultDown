#ifndef _DOWNLOAD_STRATEGY_H_   // NOLINT
#define _DOWNLOAD_STRATEGY_H_

#include <string>
#include "downloader/proto.h"

namespace downloader {

class DownloadStrategy {
 public:
    DownloadStrategy() {}

    virtual int Download(const std::string& url,
                const std::string& file_path,
                uint64_t start,
                uint64_t end) = 0;

    virtual int MutiDown(const std::string& url,
                const std::string& file_path,
                uint64_t start,
                uint64_t end,
                int threads) = 0;

    virtual ~DownloadStrategy() = default;
};

}   // namespace downloader

#endif  // _DOWNLOAD_STRATEGY_H_    NOLINT
