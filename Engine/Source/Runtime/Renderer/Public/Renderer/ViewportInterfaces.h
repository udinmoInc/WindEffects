// ==============================================================================
// WindEffects — Renderer — ViewportInterfaces
// Public API surface for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Renderer/Export.h"
#include "RHI/Types.h"

#include <cstdint>

namespace we::runtime::renderer {

class RENDERER_API ISceneViewportController {
public:
    virtual ~ISceneViewportController() = default;

    virtual void SetViewportRenderTargetSize(uint32_t width, uint32_t height) = 0;
    virtual void SetViewportBlitRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
    virtual void SetViewportRenderTargetColor(we::rhi::RHITextureHandle colorTexture) = 0;
    virtual void SetViewportDepthTarget(we::rhi::RHITextureHandle depthTexture) = 0;
    [[nodiscard]] virtual we::rhi::RHITextureViewHandle GetViewportColorView() const = 0;
    [[nodiscard]] virtual we::rhi::RHISamplerHandle GetViewportColorSampler() const = 0;
};

} // namespace we::runtime::renderer
