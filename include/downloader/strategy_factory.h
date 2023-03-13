#ifndef _STRATEGY_FACTORY_H_    // NOLINT
#define _STRATEGY_FACTORY_H_    // NOLINT

#include "downloader/proto.h"
#include "downloader/download_strategy.h"

namespace downloader {

class StrategyFactory {
 public:
    StrategyFactory() {}
    virtual ~StrategyFactory() = default;

    virtual DownloadStrategy* NewStrategy() = 0;
};

}   // namespace downloader

#endif  // _STRATEGY_FACTORY_H_ NOLINT
