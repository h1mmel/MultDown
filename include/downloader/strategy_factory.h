#ifndef _STRATEGY_FACTORY_H_    // NOLINT
#define _STRATEGY_FACTORY_H_    // NOLINT

#include <string>

#include "downloader/proto.h"
#include "downloader/download_strategy.h"

namespace downloader {

class StrategyFactory {
 public:
    StrategyFactory() {}
    virtual ~StrategyFactory() = default;

    virtual DownloadStrategy* NewStrategy(int threads_number,
                                          const std::string& path) = 0;
};

}   // namespace downloader

#endif  // _STRATEGY_FACTORY_H_ NOLINT
