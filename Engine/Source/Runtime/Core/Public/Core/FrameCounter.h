// ==============================================================================
// WindEffects — Core — FrameCounter
// Public API surface for the Core module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Core/Export.h"
#include <atomic>
#include <cstdint>

namespace we::runtime::core {

class FrameCounter {
public:
    CORE_API static void Advance();
    CORE_API static uint64_t GetFrameNumber();
    CORE_API static void Reset();

private:
    static std::atomic<uint64_t> s_FrameNumber;
};

} // namespace we::runtime::core
