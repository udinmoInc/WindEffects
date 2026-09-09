// ==============================================================================
// WindEffects — Platform — MacPlatform
// Internal implementation for the Platform module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Platform/PlatformConfig.h"

#if WE_PLATFORM_MAC

#include "Common/PlatformBackendBase.h"

namespace we::platform {

// Native macOS backend (Cocoa / AppKit / Quartz). No Metal rendering here —
// only windowing, input, and OS services for the RHI surface handle.
class MacPlatform final : public PlatformBackendBase {
public:
    [[nodiscard]] const char* GetName() const override { return "Mac"; }

    bool Initialize(const PlatformDesc& desc = {}) override {
        return PlatformBackendBase::Initialize(desc);
    }
};

} // namespace we::platform

#endif // WE_PLATFORM_MAC
