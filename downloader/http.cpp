#include "downloader/http.h"

namespace downloader {

namespace proto {

struct Http::ContentType {
    std::string type;
    std::string parameter;
};

Http::Http() : content_type_(new ContentType),
               content_length_(0)
{}

Http::~Http() {
    if (content_type_ != nullptr) delete content_type_;
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

void Http::SetContentLength(uint64_t len) {
    content_length_ = len;
}

uint64_t Http::GetContentLength() const {
    return content_length_;
}

}   // namespace proto

}   // namespace downloader
