#pragma once

#include "Platform/Types.h"

#include <condition_variable>
#include <mutex>

namespace we::platform {

struct SyncMutex {
    std::mutex mutex;
};

struct SyncEvent {
    std::mutex mutex;
    std::condition_variable cv;
    bool signaled = false;
    bool manualReset = false;
};

} // namespace we::platform
