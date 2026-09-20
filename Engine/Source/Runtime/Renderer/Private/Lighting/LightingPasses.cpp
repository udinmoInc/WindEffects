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
#include "Lighting/ShadowSystem.h"
#include "RHI/Desc.h"

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

ShadowMapPass::ShadowMapPass(ShadowSystem* shadows)
    : RenderPass("ShadowMapPass", we::rhi::QueueType::Graphics, GraphPassFlags::SideEffects)
    , m_Shadows(shadows)
{
}

void ShadowMapPass::Setup(std::vector<GraphTextureRef>& textures, std::vector<GraphBufferRef>&) {
    if (!m_Shadows || !m_Shadows->Enabled()) {
        return;
    }
    const auto& frame = m_Shadows->GetFrameData();
    if (frame.atlas == we::rhi::RHITextureHandle::Invalid) {
        return;
    }
    GraphTextureRef atlas{};
    atlas.handle = frame.atlas;
    atlas.desiredState = we::rhi::ResourceState::DepthWrite;
    atlas.access = GraphResourceAccess::Write;
    textures.push_back(atlas);
}

void ShadowMapPass::Execute(const GraphPassContext& ctx) {
    if (!m_Shadows || !m_Shadows->Enabled() || !ctx.commandList) {
        return;
    }
    const auto& frame = m_Shadows->GetFrameData();
    if (frame.atlas == we::rhi::RHITextureHandle::Invalid) {
        return;
    }

    // Clear atlas to far depth (no occluders). Future: depth-only caster draws per cascade.
    we::rhi::RenderingInfo info{};
    info.depth.enabled = true;
    info.depth.texture = frame.atlas;
    info.depth.loadOp = we::rhi::LoadOp::Clear;
    info.depth.storeOp = we::rhi::StoreOp::Store;
    info.depth.clearDepth = 1.0f;
    info.renderArea = {frame.atlasResolution, frame.atlasResolution};
    ctx.commandList->BeginRendering(info);
    // Cascade viewports reserved for caster injection (meshes / terrain).
    for (uint32_t i = 0; i < m_Shadows->CascadeCount(); ++i) {
        const auto& cascade = m_Shadows->Cascades()[i];
        ctx.commandList->SetViewport({
            static_cast<float>(cascade.viewportX),
            static_cast<float>(cascade.viewportY),
            static_cast<float>(cascade.viewportW),
            static_cast<float>(cascade.viewportH),
            0.0f,
            1.0f});
        ctx.commandList->SetScissor({
            cascade.viewportX,
            cascade.viewportY,
            cascade.viewportW,
            cascade.viewportH});
        // Caster draw hooks land here (depth-only PSO + lightViewProj[i]).
    }
    ctx.commandList->EndRendering();
}

} // namespace we::runtime::renderer
