// ==============================================================================
// WindEffects — Renderer — RendererSDK
// Public API surface for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Renderer/Renderer.h"
#include "Renderer/ViewportInterfaces.h"
#include "Camera/CameraUniform.h"
#include "Resource/DepthTarget.h"
#include "Platform/Types.h"
#include "RHI/RHISDK.h"

namespace we::runtime::renderer {

struct RendererInitOptions {
    we::platform::WindowId window = we::platform::WindowId::Invalid;
};

inline void InitializeRenderer(Renderer& renderer, const RendererInitOptions& options) {
    renderer.Init(options.window);
}

} // namespace we::runtime::renderer
