#include <unistd.h>
#include <sys/stat.h>

#include <cctype>
#include <algorithm>

#include "downloader/download.h"
#include "downloader/http_factory.h"
#include "third_party/cmdline/cmdline.h"

void helper(cmdline::parser* parser, int argc, char* argv[]);

using downloader::Downloader;
using proto::Http;

int main(int argc, char* argv[]) {
    cmdline::parser p;
    helper(&p, argc, argv);

    std::string url = p.get<std::string>("url");
    std::string proto = p.get<std::string>("proto");
    transform(proto.begin(), proto.end(), proto.begin(), ::tolower);
    int threads_number = 8;
    std::string threads = p.get<std::string>("threads");
    if (!threads.empty()) threads_number = std::stoul(threads);
    std::string path = p.get<std::string>("output");
    if (path.back() == '/') {
        std::cerr << path << ": Is a directory" << std::endl;
        exit(-1);
    } else if (path.empty()) {
        char buf[128] = {0}, *p_buf = buf;
        if ((p_buf = getcwd(buf, sizeof(buf))) == nullptr) {
            perror(nullptr);
            exit(-1);
        }
        path = buf;
        path += '/';
    } else {
        size_t pos = path.find_last_of('/');
        if (pos == std::string::npos) {
            char buf[256] = {0}, *p_buf = buf;
            if ((p_buf = getcwd(buf, sizeof(buf))) == nullptr) {
                perror(nullptr);
                exit(-1);
            }
            buf[strlen(buf)] = '/';
            path = buf + path;
        } else {
            std::string dir = path.substr(0, pos + 1);
            struct stat s = {0};
            if (stat(dir.c_str(), &s) != 0) {
                perror(nullptr);
                exit(-1);
            }
        }
    }
    if (proto.empty() || proto.compare("http") == 0) {
        Downloader<Http> down(threads_number, path);
        if (down.DoDownload(url) != 0) {
            std::cout << "error!" << std::endl;
        }
    }

    return 0;
}

void helper(cmdline::parser* parser, int argc, char* argv[]) {
    parser->add<std::string>("url", 'u', "url", true, "");
    parser->add<std::string>("proto", 'p', "protocol type", false, "");
    parser->add<std::string>("output", 'o', "output file", false, "");
    parser->add<std::string>("threads", 't', "set threads, default is 8",
                            false, "");
    parser->add("help", 'h', "print this message");
    parser->set_program_name("mdown");
    parser->parse_check(argc, argv);
}
