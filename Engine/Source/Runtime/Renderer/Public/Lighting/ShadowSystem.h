// ==============================================================================
// WindEffects — Renderer — ShadowSystem
// Public API surface for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#pragma warning(push)
#pragma warning(disable : 4251)

#include "Camera/CameraUniform.h"
#include "Lighting/LightingTypes.h"
#include "Renderer/Export.h"
#include "Renderer/Scalability/RenderingSettings.h"

#include <array>
#include <cstdint>

namespace we::runtime::renderer {

class RenderGraph;

/// Cascade shadow foundation. Owns cascade parameters only until shadow maps land.
class RENDERER_API ShadowSystem {
public:
    void Configure(const ShadowQualitySettings& settings);
    void BeginFrame(const CameraUniform& camera, const DirectionalLight* primaryLight);

    /// Future: create real shadow-map passes. No placeholder resources today.
    void BuildRenderGraph(RenderGraph& graph);

    void Shutdown();

    [[nodiscard]] bool Enabled() const { return m_Settings.enabled && m_PrimaryCastsShadows; }
    [[nodiscard]] uint32_t CascadeCount() const { return m_CascadeCount; }
    [[nodiscard]] float ResolutionScale() const { return m_Settings.resolutionScale; }
    [[nodiscard]] bool SoftShadows() const { return m_Settings.softShadows; }
    [[nodiscard]] const ShadowQualitySettings& Settings() const { return m_Settings; }

    /// World-space cascade split distances (view-relative), filled in BeginFrame.
    [[nodiscard]] const std::array<float, 4>& CascadeSplits() const { return m_CascadeSplits; }

private:
    ShadowQualitySettings m_Settings{};
    uint32_t m_CascadeCount = 0;
    bool m_PrimaryCastsShadows = false;
    std::array<float, 4> m_CascadeSplits{0.0f, 0.0f, 0.0f, 0.0f};
};

} // namespace we::runtime::renderer

#pragma warning(pop)
