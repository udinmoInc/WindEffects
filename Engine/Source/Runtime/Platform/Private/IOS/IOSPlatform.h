#pragma once

#include "Platform/PlatformConfig.h"

#if WE_PLATFORM_IOS

#include "Common/PlatformBackendBase.h"

namespace we::platform {

// Native iOS backend (UIKit). Graphics APIs stay out of this layer.
class IOSPlatform final : public PlatformBackendBase {
public:
    [[nodiscard]] const char* GetName() const override { return "iOS"; }

    bool Initialize(const PlatformDesc& desc = {}) override {
        return PlatformBackendBase::Initialize(desc);
    }
};

} // namespace we::platform

#endif // WE_PLATFORM_IOS
