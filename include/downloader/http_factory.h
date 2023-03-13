#ifndef _HTTP_FACTORY_H_    // NOLINT
#define _HTTP_FACTORY_H_    // NOLINT

#include "downloader/proto_factory.h"
#include "downloader/proto.h"
#include "downloader/http.h"

namespace proto {

class HttpFactory : public ProtoFactory {
 public:
    virtual ProtoBase* NewProto() {
        return new Http();
    }
};

}   // namespace proto

#endif  // _HTTP_FACTORY_H_ NOLINT
