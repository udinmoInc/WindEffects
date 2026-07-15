#pragma once

#include "Platform/Export.h"
#include "Platform/IPlatform.h"
#include "Platform/PlatformConfig.h"
#include "Platform/PlatformFactory.h"

namespace we::platform {

// Developer-facing facade. Auto-selects and initializes the host backend.
//
// Example:
//   auto& platform = Platform::Get();
//   auto window = platform.CreateWindow({
//       .title = "WindEffects Editor",
//       .width = 1920,
//       .height = 1080,
//       .resizable = true,
//       .maximized = true
//   });
//   if (!window) { /* inspect window.error */ }
//   while (platform.PollEvents()) {
//       engine.Tick();
//   }
class PLATFORM_API Platform {
public:
    // Initialize with an explicit descriptor (services / DPI / etc.). Prefer this
    // for tools and headless hosts. Safe to call once; subsequent calls return Get().
    [[nodiscard]] static IPlatform& Initialize(const PlatformDesc& desc = {});

    [[nodiscard]] static IPlatform& Get();
    [[nodiscard]] static bool IsAvailable() noexcept;
    static void Shutdown();

    Platform() = delete;
};

} // namespace we::platform
