#include <unistd.h>
#include <sys/types.h>
#include <string>

#include "downloader/debug.h"

namespace downloader {

namespace debug {

Debug::Debug() {
    pid_t pid = getpid();
    std::string name =
                "debug_" + std::to_string(static_cast<int>(pid)) + ".log";
    FILE* fp = fopen(name.c_str(), "w+");
    if (fp != nullptr) m_fp = fp;
    else
        m_fp = stderr;
}

Debug::~Debug() = default;

Debug& Debug::GetDebugInstance() {
    static Debug dbg;
    return dbg;
}

FILE* Debug::GetFilePointer() {
    return m_fp;
}

}   // namespace debug

}   // namespace downloader
