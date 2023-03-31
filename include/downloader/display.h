#ifndef _DISPLAY_H_    // NOLINT
#define _DISPLAY_H_

#include <sstream>
#include <chrono>   // NOLINT

namespace downloader {

namespace util {

void ShowSize(std::stringstream& stream,
              int width, int precesion, uint64_t size);

void ShowRate(std::stringstream& stream,
              int width, int precesion, uint64_t size,
              std::chrono::duration<double> elapsed);

}   // namespace util

}   // namespace downloader

#endif    // NOLINT
