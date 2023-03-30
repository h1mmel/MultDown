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

    void SetResponseLine(std::string version, uint32_t code,
                         std::string status);
    std::string GetResponseVersion() const;
    uint32_t GetResponseCode() const;
    std::string GetResponseStatus() const;

    void SetServer(std::string server);
    std::string GetServer() const;

    void SetAcceptRanges(std::string accept_ranges);
    std::string GetAcceptRanges() const;

    void SetContentEncoding(std::string encoding);
    std::string GetContentEncoding() const;

    void SetContentType(std::string type, std::string param);
    std::string GetContentType() const;
    std::string GetContentTypeParameter() const;

    void SetDate(std::string date);
    std::string GetDate() const;

    void SetETag(std::string e_tag);
    std::string GetETag() const;

    void SetExpires(std::string expires);
    std::string GetExpires() const;

    void SetLastModified(std::string last_modeifed);
    std::string GetLastModified() const;

    void SetContentLength(uint64_t len);
    uint64_t GetContentLength() const;

    void SetConnection(std::string connection);
    std::string GetConnection() const;

    void SetLocation(std::string location);
    std::string GetLocation() const;

 private:
    struct StatusLine;
    struct ContentType;

    StatusLine* response_line_;
    std::string server_;
    std::string accept_ranges_;
    std::string content_encoding_;
    ContentType* content_type_;
    std::string date_;
    std::string e_tag_;
    std::string expires_;
    std::string last_modified_;
    uint64_t content_length_;
    std::string connection_;
    std::string location_;
};

}   // namespace proto

}   // namespace downloader

#endif  // _HTTP_H_ NOLINT
