#pragma once

#include "Platform/Export.h"
#include "Platform/IPlatform.h"
#include "Platform/PlatformConfig.h"

namespace we::platform {

// Creates the host platform backend. Engine startup should not call this
// directly — Platform::Get() selects and initializes automatically.
class PLATFORM_API PlatformFactory {
public:
    [[nodiscard]] static IPlatform* Create();
    [[nodiscard]] static const char* GetHostPlatformName() noexcept;
};

} // namespace we::platform
