// ==============================================================================
// WindEffects — Renderer — ShadowSystem
// Internal implementation for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Lighting/ShadowSystem.h"
#include "Renderer/Graph/RenderGraph.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::renderer {

namespace {
constexpr float kDefaultNear = 0.1f;
constexpr float kDefaultFar = 10000.0f;
constexpr float kPracticalLambda = 0.85f;
} // namespace

void ShadowSystem::Configure(const ShadowQualitySettings& settings) {
    m_Settings = settings;
    m_CascadeCount = 0;
    if (m_Settings.enabled) {
        m_CascadeCount = std::min(4u, std::max(1u, m_Settings.cascadeCount));
    }
}

void ShadowSystem::BeginFrame(const CameraUniform& camera, const DirectionalLight* primaryLight) {
    (void)camera;
    m_PrimaryCastsShadows =
        primaryLight != nullptr && primaryLight->enabled && primaryLight->castsShadows;

    if (!m_Settings.enabled || !m_PrimaryCastsShadows || m_CascadeCount == 0) {
        m_CascadeSplits = {0.0f, 0.0f, 0.0f, 0.0f};
        return;
    }

    // Practical split (log/uniform blend). CameraUniform has no near/far yet — use scene defaults.
    const float nearZ = kDefaultNear;
    const float farZ = kDefaultFar;
    for (uint32_t i = 0; i < 4; ++i) {
        if (i >= m_CascadeCount) {
            m_CascadeSplits[i] = 0.0f;
            continue;
        }
        const float p = static_cast<float>(i + 1) / static_cast<float>(m_CascadeCount);
        const float logSplit = nearZ * std::pow(farZ / nearZ, p);
        const float uniSplit = nearZ + (farZ - nearZ) * p;
        m_CascadeSplits[i] = kPracticalLambda * logSplit + (1.0f - kPracticalLambda) * uniSplit;
    }
}

void ShadowSystem::BuildRenderGraph(RenderGraph& graph) {
    (void)graph;
    // Shadow-map passes land here once cascade atlases exist. No placeholder textures.
}

void ShadowSystem::Shutdown() {
    m_Settings = {};
    m_CascadeCount = 0;
    m_PrimaryCastsShadows = false;
    m_CascadeSplits = {0.0f, 0.0f, 0.0f, 0.0f};
}

} // namespace we::runtime::renderer
