#include <iomanip>

#include "downloader/display.h"

namespace downloader {

namespace util {

void ShowSize(std::stringstream& stream,
              int width, int precesion, uint64_t size) {
    stream << std::setiosflags(std::ios::fixed) << std::setprecision(precesion);
    if (size < 1024) stream << std::setw(width) << size << "B";
    else if (size < 1024 * 1024)
        stream << std::setw(width) << 1.0 * size / 1024 << "K";
    else if (size < 1024 * 1024 * 1024)
        stream << std::setw(width) << 1.0 * size / 1024 / 1024 << "M";
    else
        stream << std::setw(width) << 1.0 * size / 1024 / 1024 / 1024 << "G";
}

void ShowRate(std::stringstream& stream,
              int width, int precesion, uint64_t size,
              std::chrono::duration<double> elapsed) {
    stream << std::setiosflags(std::ios::fixed) << std::setprecision(precesion);
    if (size < 1024)
        stream << std::setw(width) << 1.0 * size / elapsed.count() << "B/s";
    else if (size < 1024 * 1024)
        stream << std::setw(width)
               << 1.0 * size / 1024 / elapsed.count() << "KB/s";
    else
        stream << std::setw(width)
               << 1.0 * size / 1024 / 1024 / elapsed.count() << "MB/s";
}

}   // namespace util

}   // namespace downloader
