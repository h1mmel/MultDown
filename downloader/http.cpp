#include "downloader/http.h"

namespace downloader {

namespace proto {

struct Http::StatusLine {
    StatusLine() : status_code(0) {}

    std::string version;
    uint32_t status_code;
    std::string status;
};

struct Http::ContentType {
    std::string type;
    std::string parameter;
};

Http::Http() : response_line_(new StatusLine),
               content_type_(new ContentType),
               content_length_(0) {}

Http::~Http() {
    if (response_line_ != nullptr) delete response_line_;
    if (content_type_ != nullptr) delete content_type_;
}

void Http::SetResponseLine(std::string version, uint32_t code,
                           std::string status) {
    response_line_->version = version;
    response_line_->status_code = code;
    response_line_->status = status;
    content_length_ = 0UL;
}

std::string Http::GetResponseVersion() const {
    return response_line_->version;
}

uint32_t Http::GetResponseCode() const {
    return response_line_->status_code;
}

std::string Http::GetResponseStatus() const {
    return response_line_->status;
}

void Http::SetServer(std::string server) {
    server_ = server;
}

std::string Http::GetServer() const {
    return server_;
}

void Http::SetAcceptRanges(std::string accept_ranges) {
    accept_ranges_ = accept_ranges;
}

std::string Http::GetAcceptRanges() const {
    return accept_ranges_;
}

void Http::SetContentEncoding(std::string encoding) {
    content_encoding_ = encoding;
}

std::string Http::GetContentEncoding() const {
    return content_encoding_;
}

void Http::SetContentType(std::string type, std::string param) {
    content_type_->type = type;
    content_type_->parameter = param;
}

std::string Http::GetContentType() const {
    return content_type_->type;
}

std::string Http::GetContentTypeParameter() const {
    return content_type_->parameter;
}

void Http::SetTransferEncoding(std::string transfer_encoding) {
    transfer_encoding_ = transfer_encoding;
}

std::string Http::GetTransferEncoding() const {
    return transfer_encoding_;
}

bool Http::IsChunked() const {
    return transfer_encoding_.find("chunked") != std::string::npos;
}

void Http::SetDate(std::string date) {
    date_ = date;
}

std::string Http::GetDate() const {
    return date_;
}

void Http::SetETag(std::string e_tag) {
    e_tag_ = e_tag;
}

std::string Http::GetETag() const {
    return e_tag_;
}

void Http::SetExpires(std::string expires) {
    expires_ = expires;
}

std::string Http::GetExpires() const {
    return expires_;
}

void Http::SetLastModified(std::string last_modified) {
    last_modified_ = last_modified;
}

std::string Http::GetLastModified() const {
    return last_modified_;
}

void Http::SetContentLength(uint64_t len) {
    content_length_ = len;
}

uint64_t Http::GetContentLength() const {
    return content_length_;
}

// 对于 Content-Encoding 响应, 返回 0
uint64_t Http::GetActualContentLength() const {
    if (IsChunked() || !GetContentEncoding().empty()) return 0UL;
    else
        return GetContentLength();
}

void Http::SetConnection(std::string connection) {
    connection_ = connection;
}

std::string Http::GetConnection() const {
    return connection_;
}

void Http::SetLocation(std::string location) {
    location_ = location;
}

std::string Http::GetLocation() const {
    return location_;
}

}   // namespace proto

}   // namespace downloader
