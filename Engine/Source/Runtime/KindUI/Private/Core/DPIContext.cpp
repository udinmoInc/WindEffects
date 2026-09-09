// ==============================================================================
// WindEffects — KindUI — DPIContext
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "KindUI/Core/DPIContext.h"

#include <atomic>

namespace we::runtime::kindui {

namespace {

// UI-thread primary writer; atomic allows safe concurrent reads (e.g. layout helpers).
std::atomic<float> s_DPIScale{1.0f};

} // namespace

float DPIContext::GetScale() {
    return s_DPIScale.load(std::memory_order_relaxed);
}

void DPIContext::SetScale(float scale) {
    s_DPIScale.store(scale, std::memory_order_relaxed);
}

} // namespace we::runtime::kindui
 
