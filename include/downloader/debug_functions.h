#ifndef _DEBUG_FUNCTIONS_H_ // NOLINT
#define _DEBUG_FUNCTIONS_H_ // NOLINT

#include <stdint.h>
#include <stdio.h>
#include <curl/curl.h>

namespace downloader {

static void dump(const char* text,
                 FILE* stream, uint8_t* ptr,
                 uint32_t size) {
    uint32_t i = 0, c = 0;
    uint32_t width = 0x10;

    fprintf(stream, "%s, %10.10d bytes (0x%8.8x)\n",
            text, static_cast<int32_t>(size),
            static_cast<int32_t>(size));

    for (i = 0; i < size; i += width) {
        fprintf(stream, "%4.4x: ", static_cast<int32_t>(i));
        for (c = 0; c < width; c++) {
            if (i + c < size)
                fprintf(stream, "%02x ", ptr[i+c]);
            else
                fputs("   ", stream);
        }
        for (c = 0; (c < width) && (i + c < size); c++) {
            char x = (ptr[i + c] >= 0x20 && ptr[i + c] < 0x80)
                    ? ptr[i + c] : '.';
            fputc(x, stream);
        }
        fputc('\n', stream);
    }
}

static int debug_callback(CURL* handle, curl_infotype type,
                          char* data, uint32_t size, void* clientp) {
    const char* text = nullptr;
    static_cast<void>(handle);
    static_cast<void>(clientp);
    FILE* stream = reinterpret_cast<FILE*>(clientp);
    if (stream == nullptr) stream = stderr;

    switch (type) {
        case CURLINFO_TEXT:
            fputs("== Info: ", stream);
            fwrite(data, size, 1, stream);
        default:
            return 0;
        case CURLINFO_HEADER_OUT:
            text = "=> Send header";
            break;
        case CURLINFO_DATA_OUT:
            text = "=> Send data";
            break;
        case CURLINFO_SSL_DATA_OUT:
            text = "=> Send SSL data";
            break;
        case CURLINFO_HEADER_IN:
            text = "<= Recv header";
            break;
        case CURLINFO_DATA_IN:
            text = "<= Recv data";
            break;
        case CURLINFO_SSL_DATA_IN:
            text = "<= Recv SSL data";
            break;
    }

    dump(text, stream, reinterpret_cast<uint8_t*>(data), size);
    return 0;
}

}   // namespace downloader

#endif  // _DEBUG_FUNCTIONS_H_  NOLINT
