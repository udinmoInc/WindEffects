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
