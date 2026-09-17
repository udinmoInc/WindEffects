// ==============================================================================
// WindEffects — Renderer — CapabilityResolver
// Public API surface for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Renderer/Export.h"
#include "Renderer/Scalability/RenderingSettings.h"
#include "RHI/Capabilities.h"

namespace we::runtime::renderer {

/// Combines a requested profile with RHICapabilities into ResolvedRenderingSettings.
/// Never crashes on missing features — records explicit fallback notes instead.
class RENDERER_API CapabilityResolver {
public:
    [[nodiscard]] static ResolvedRenderingSettings Resolve(
        const RenderingProfileDesc& profile,
        const we::rhi::RHICapabilities& caps);
};

} // namespace we::runtime::renderer
