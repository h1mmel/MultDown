#ifndef _PROTO_H_   // NOLINT
#define _PROTO_H_   // NOLINT

namespace downloader {

namespace proto {

class ProtoBase {
 public:
    ProtoBase() = default;

    ProtoBase(const ProtoBase&) = delete;
    ProtoBase& operator=(const ProtoBase&) = delete;

    virtual ~ProtoBase();
};

}   // namespace proto

}   // namespace downloader

#endif  // _PROTO_H_ NOLINT
