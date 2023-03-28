#ifndef _STATUS_H_  // NOLINT
#define _STATUS_H_

#include <curl/curl.h>

#include <string>
#include <vector>
#include <utility>

namespace downloader {

struct MetaStatus {
    MetaStatus() : status(0),
                   total_down(0),
                   curl_code(CURL_LAST),
                   response_code(0)
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

    int32_t status;
    uint64_t total_down;
    CURLcode curl_code;
    int64_t response_code;
    std::pair<uint64_t, uint64_t> down;
    std::string error_msg;
};

struct Status {
    Status() : status(0),
               total(0)
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

    int32_t status;
    uint64_t total;
    std::vector<uint64_t> total_down;
    std::vector<int32_t> stats;
    std::vector<CURLcode> curl_codes;
    std::vector<int64_t> response_codes;
    std::vector<std::pair<uint64_t, uint64_t>> down;
    std::string error_msg;
};

}   // namespace downloader

#endif  // _STATUS_H_   NOLINT