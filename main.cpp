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
    std::string save_path = p.get<std::string>("output");
    if (proto.empty() || proto.compare("http") == 0) {
        Downloader<Http> down(threads_number, save_path);
        if (down.DoDownload(url) != 0) {
            std::cout << "error!" << std::endl;
        }
    }

    return 0;
}

void helper(cmdline::parser* parser, int argc, char* argv[]) {
    parser->add<std::string>("url", 'u', "url", true, "");
    parser->add<std::string>("proto", 'p', "protocol type", false, "");
    parser->add<std::string>("output", 'o', "output file", true, "");
    parser->add<std::string>("threads", 't', "set threads, default is 8",
                            false, "");
    parser->add("help", 'h', "print this message");
    parser->set_program_name("downloader");
    parser->parse_check(argc, argv);
}
