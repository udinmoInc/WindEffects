// ==============================================================================
// WindEffects — Platform — LinuxPlatform
// Internal implementation for the Platform module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Platform/PlatformConfig.h"

#if WE_PLATFORM_LINUX

#include "Common/PlatformBackendBase.h"

namespace we::platform {

// Native Linux backend (X11 / Wayland). Windowing and input are filled in
// incrementally; shared filesystem/time/threading comes from PlatformBackendBase.
class LinuxPlatform final : public PlatformBackendBase {
public:
    [[nodiscard]] const char* GetName() const override { return "Linux"; }

    bool Initialize(const PlatformDesc& desc = {}) override {
        // Future: probe Wayland first, then fall back to X11.
        return PlatformBackendBase::Initialize(desc);
    }
};

} // namespace we::platform

#endif // WE_PLATFORM_LINUX
