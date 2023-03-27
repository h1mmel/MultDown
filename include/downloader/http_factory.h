#ifndef _HTTP_FACTORY_H_    // NOLINT
#define _HTTP_FACTORY_H_    // NOLINT

#include "downloader/proto_factory.h"

namespace downloader {

namespace proto {

class HttpFactory : public ProtoFactory {
 public:
    HttpFactory() = default;

    HttpFactory(const HttpFactory&) = delete;
    HttpFactory& operator=(const HttpFactory&) = delete;

    virtual ~HttpFactory();

    ProtoBase* NewProto() override;
};

}   // namespace proto

}   // namespace downloader

#endif  // _HTTP_FACTORY_H_ NOLINT
