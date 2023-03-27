#include "downloader/http_factory.h"
#include "downloader/http.h"

namespace downloader {

namespace proto {

HttpFactory::~HttpFactory() = default;

ProtoBase* HttpFactory::NewProto() {
    return new Http();
}

}   // namespace proto

}   // namespace downloader
