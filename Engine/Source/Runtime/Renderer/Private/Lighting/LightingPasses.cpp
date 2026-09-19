// ==============================================================================
// WindEffects — Renderer — LightingPasses
// Internal implementation for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Lighting/LightingPasses.h"
#include "Lighting/LightingSystem.h"

namespace we::runtime::renderer {

LightingPreparePass::LightingPreparePass(LightingSystem* lighting)
    : RenderPass("LightingPreparePass", we::rhi::QueueType::Transfer, GraphPassFlags::SideEffects)
    , m_Lighting(lighting)
{
}

void LightingPreparePass::Setup(std::vector<GraphTextureRef>&, std::vector<GraphBufferRef>&) {}

void LightingPreparePass::Execute(const GraphPassContext&) {
    if (m_Lighting) {
        m_Lighting->UploadGpuData();
    }
}

} // namespace we::runtime::renderer
