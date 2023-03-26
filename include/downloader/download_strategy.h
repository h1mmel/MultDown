#ifndef _DOWNLOAD_STRATEGY_H_   // NOLINT
#define _DOWNLOAD_STRATEGY_H_

#include <curl/curl.h>

#include <string>
#include <vector>
#include <utility>

#include "downloader/proto.h"

namespace downloader {

struct MetaStatus {
    int32_t status;
    uint64_t total_down;
    CURLcode curl_code;
    int64_t response_code;
    std::pair<uint64_t, uint64_t> down;
    std::string error_msg;

    MetaStatus() : status(0),
                   total_down(0),
                   curl_code(CURL_LAST),
                   response_code(0),
                   down(),
                   error_msg()
    {}

    MetaStatus(int32_t status,
               uint64_t total_down,
               CURLcode curl_code,
               int64_t response_code,
               std::pair<uint64_t, uint64_t> down,
               std::string error_msg)
            : status(status),
              total_down(total_down),
              curl_code(curl_code),
              response_code(response_code),
              down(down),
              error_msg(error_msg)
    {}
};

struct Status {
    int32_t status;
    uint64_t total;
    std::vector<uint64_t> total_down;
    std::vector<int32_t> stats;
    std::vector<CURLcode> curl_codes;
    std::vector<int64_t> response_codes;
    std::vector<std::pair<uint64_t, uint64_t>> down;
    std::string error_msg;

    Status() : status(0),
               total(0),
               total_down(),
               stats(),
               curl_codes(),
               response_codes(),
               down(),
               error_msg()
    {}

    Status(int32_t status,
           uint64_t total,
           std::vector<uint64_t> total_down,
           std::vector<int32_t> stats,
           std::vector<CURLcode> curl_codes,
           std::vector<int64_t> response_codes,
           std::vector<std::pair<uint64_t, uint64_t>> down,
           std::string error_msg)
           : status(status),
             total(total),
             total_down(total_down),
             stats(stats),
             curl_codes(curl_codes),
             response_codes(response_codes),
             down(down),
             error_msg(error_msg)
    {}
};

class DownloadStrategy {
 public:
    DownloadStrategy() {}

    virtual Status Download(const std::string& url,
                              uint64_t start,
                              uint64_t end) = 0;

    virtual ~DownloadStrategy() = default;
};

}   // namespace downloader

#endif  // _DOWNLOAD_STRATEGY_H_    NOLINT
