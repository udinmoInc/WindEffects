// ==============================================================================
// WindEffects — Platform — SyncPrimitives
// Internal implementation for the Platform module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
