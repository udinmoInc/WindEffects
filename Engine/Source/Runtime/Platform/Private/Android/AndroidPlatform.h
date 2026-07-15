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
