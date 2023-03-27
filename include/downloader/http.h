#ifndef _HTTP_H_    // NOLINT
#define _HTTP_H_    // NOLINT

#include "downloader/proto.h"

namespace downloader {

namespace proto {

class Http : public ProtoBase {
 public:
    Http() = default;

    Http(const Http&) = delete;
    Http& operator=(const Http&) = delete;

    ~Http() override;
};

}   // namespace proto

}   // namespace downloader

#endif  // _HTTP_H_ NOLINT
