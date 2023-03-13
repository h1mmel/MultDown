#ifndef _PROTO_FACTORY_H_   // NOLINT
#define _PROTO_FACTORY_H_   // NOLINT

#include "downloader/proto.h"

namespace proto {

class ProtoFactory {
 public:
    ProtoFactory() {}
    virtual ~ProtoFactory() = default;
    virtual ProtoBase* NewProto() = 0;
};

}   // namespace proto

#endif  // _PROTO_FACTORY_H_ NOLINT