#ifndef _HTTP_H_    // NOLINT
#define _HTTP_H_    // NOLINT

#include <cstdint>
#include <string>

#include "downloader/proto.h"

namespace downloader {

namespace proto {

// TODO(xxx) : 完善 http 字段
class Http : public ProtoBase {
 public:
    Http();

    Http(const Http&) = delete;
    Http& operator=(const Http&) = delete;

    ~Http() override;

    void SetContentEncoding(std::string encoding);
    std::string GetContentEncoding() const;

    void SetContentType(std::string type, std::string param);
    std::string GetContentType() const;
    std::string GetContentTypeParameter() const;

    void SetContentLength(uint64_t len);
    uint64_t GetContentLength() const;

 private:
    struct ContentType;

    std::string content_encoding_;
    ContentType* content_type_;
    uint64_t content_length_;
};

}   // namespace proto

}   // namespace downloader

#endif  // _HTTP_H_ NOLINT
