// ==============================================================================
// WindEffects — Renderer — LightingPasses
// Public API surface for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Renderer/Export.h"
#include "Renderer/Graph/RenderGraph.h"

namespace we::runtime::renderer {

class LightingSystem;

/// Uploads LightingSystem GPU light buffers for the frame.
class RENDERER_API LightingPreparePass final : public RenderPass {
public:
    explicit LightingPreparePass(LightingSystem* lighting);
    void Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>& buffers) override;
    void Execute(const GraphPassContext& ctx) override;

private:
    LightingSystem* m_Lighting = nullptr;
};

} // namespace we::runtime::renderer
