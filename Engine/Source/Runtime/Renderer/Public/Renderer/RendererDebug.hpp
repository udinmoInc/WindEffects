#pragma once

#include "Renderer/Export.hpp"

namespace we::runtime::renderer {

enum class RenderPassId : int;

/// Cumulative stages for re-enabling environment features after minimal debug rendering.
/// Stages 3–5 (individual LUTs) are baked together with SkyAtmosphere in this engine.
enum class MinimalRendererStage : int {
    DirectionalLight = 0,
    Tonemapping = 1,
    SkyAtmosphere = 2,
    TransmittanceLUT = 3,
    SkyViewLUT = 4,
    MultipleScattering = 5,
    VolumetricClouds = 6,
    Fog = 7,
    Bloom = 8,
    AutoExposure = 9,
    Count
};

struct RendererDebugSettings {
    /// When true (and enableStage < 0), only clear + unlit geometry + UI run.
    bool disableEnvironment = false;
    /// -1 = use disableEnvironment only; 0..9 = cumulative feature stages (implies debug isolation).
    int enableStage = -1;
    float clearColorR = 0.12f;
    float clearColorG = 0.12f;
    float clearColorB = 0.12f;
    float clearColorA = 1.0f;
};

class RendererDebug {
public:
    RENDERER_API static RendererDebug& Get();

    RENDERER_API void Configure(const RendererDebugSettings& settings);
    RENDERER_API const RendererDebugSettings& GetSettings() const { return m_Settings; }

    RENDERER_API void InitializeFromEnvironment(int argc, char* argv[]);

    /// True when minimal/staged debug rendering is active (not full production path).
    RENDERER_API bool IsMinimalRendererActive() const;

    RENDERER_API bool IsEnvironmentDisabled() const;

    RENDERER_API bool IsFeatureEnabled(MinimalRendererStage stage) const;

    RENDERER_API bool ShouldRunPass(RenderPassId pass) const;
    RENDERER_API bool ShouldRunPostProcess() const;
    RENDERER_API bool ShouldRenderGrid() const;
    RENDERER_API bool ShouldApplyBloom() const;
    RENDERER_API bool ShouldApplyAutoExposure() const;
    RENDERER_API bool ShouldApplyTonemapping() const;

    /// Lit meshes + environment uniform uploads.
    RENDERER_API bool ShouldUseDirectionalLighting() const;
    RENDERER_API bool ShouldUploadEnvironmentUniform() const;
    RENDERER_API bool ShouldCreateAtmosphereResources() const;
    RENDERER_API bool ShouldCreatePostProcessResources() const;

private:
    RendererDebug() = default;

    int EffectiveStage() const;

    RendererDebugSettings m_Settings{};
};

} // namespace we::runtime::renderer
