// ==============================================================================
// WindEffects — Platform — AndroidPlatform
// Internal implementation for the Platform module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Platform/PlatformConfig.h"

#if WE_PLATFORM_ANDROID

#include "Common/PlatformBackendBase.h"

namespace we::platform {

// Native Android backend (ANativeWindow / GameActivity / Choreographer).
class AndroidPlatform final : public PlatformBackendBase {
public:
    [[nodiscard]] const char* GetName() const override { return "Android"; }

    bool Initialize(const PlatformDesc& desc = {}) override {
        return PlatformBackendBase::Initialize(desc);
    }
};

} // namespace we::platform

#endif // WE_PLATFORM_ANDROID
