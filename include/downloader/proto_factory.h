#ifndef _PROTO_FACTORY_H_   // NOLINT
#define _PROTO_FACTORY_H_   // NOLINT

#include "downloader/proto.h"

namespace downloader {

namespace proto {

class ProtoFactory {
 public:
    ProtoFactory() = default;

    ProtoFactory(const ProtoFactory&) = delete;
    ProtoFactory& operator=(const ProtoFactory&) = delete;

    virtual ~ProtoFactory();

    virtual ProtoBase* NewProto() = 0;
};

}   // namespace proto

}   // namespace downloader

#endif  // _PROTO_FACTORY_H_ NOLINT