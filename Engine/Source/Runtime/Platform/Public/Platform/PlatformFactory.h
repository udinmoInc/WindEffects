// ==============================================================================
// WindEffects — Platform — PlatformFactory
// Public API surface for the Platform module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
