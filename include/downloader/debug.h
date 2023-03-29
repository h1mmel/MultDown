#ifndef _DEBUG_H_   // NOLINT
#define _DEBUG_H_

#include <cstdio>

namespace downloader {

namespace debug {

class Debug {
 public:
    Debug(const Debug&) = delete;
    Debug& operator=(const Debug&) = delete;

    static Debug& GetDebugInstance();

    FILE* GetFilePointer();

 private:
    Debug();
    ~Debug();

    FILE* fp_;
};

}   // namespace debug

}   // namespace downloader

#endif  // _DEBUG_H_    NOLINT
