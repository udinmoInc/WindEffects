#include "Renderer/RendererDebug.hpp"
#include "Renderer/RenderPipelineInvestigator.hpp"

#include <cstdlib>
#include <cstring>

namespace we::runtime::renderer {

RendererDebug& RendererDebug::Get() {
    static RendererDebug instance;
    return instance;
}

void RendererDebug::Configure(const RendererDebugSettings& settings) {
    m_Settings = settings;
}

void RendererDebug::InitializeFromEnvironment(int argc, char* argv[]) {
    RendererDebugSettings settings{};

    if (const char* env = std::getenv("WE_DISABLE_ENVIRONMENT_RENDERING")) {
        if (env[0] == '1' || std::strcmp(env, "true") == 0) {
            settings.disableEnvironment = true;
        }
    }

    if (const char* stageEnv = std::getenv("WE_MINIMAL_RENDERER_STAGE")) {
        settings.enableStage = std::atoi(stageEnv);
        settings.disableEnvironment = true;
    }

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-DisableEnvironment") == 0
            || std::strcmp(argv[i], "-WE_DISABLE_ENVIRONMENT_RENDERING") == 0) {
            settings.disableEnvironment = true;
        } else if (std::strncmp(argv[i], "-MinimalRendererStage=", 22) == 0) {
            settings.enableStage = std::atoi(argv[i] + 22);
            settings.disableEnvironment = true;
        }
    }

    Configure(settings);
}

bool RendererDebug::IsMinimalRendererActive() const {
    return m_Settings.disableEnvironment || m_Settings.enableStage >= 0;
}

int RendererDebug::EffectiveStage() const {
    if (m_Settings.enableStage >= 0) {
        return m_Settings.enableStage;
    }
    if (m_Settings.disableEnvironment) {
        return -1;
    }
    return static_cast<int>(MinimalRendererStage::Count);
}

bool RendererDebug::IsEnvironmentDisabled() const {
    return IsMinimalRendererActive() && EffectiveStage() < 0;
}

bool RendererDebug::IsFeatureEnabled(MinimalRendererStage stage) const {
    if (!IsMinimalRendererActive()) {
        return true;
    }

    const int effective = EffectiveStage();
    if (effective < 0) {
        return false;
    }

    const int required = static_cast<int>(stage);
    if (required <= static_cast<int>(MinimalRendererStage::MultipleScattering)
        && effective >= static_cast<int>(MinimalRendererStage::SkyAtmosphere)) {
        return true;
    }

    return effective >= required;
}

bool RendererDebug::ShouldRunPass(RenderPassId pass) const {
    switch (pass) {
    case RenderPassId::Clear:
    case RenderPassId::Geometry:
    case RenderPassId::Lighting:
    case RenderPassId::Present:
        return true;
    case RenderPassId::SkyAtmosphere:
        return IsFeatureEnabled(MinimalRendererStage::SkyAtmosphere);
    case RenderPassId::VolumetricClouds:
        return IsFeatureEnabled(MinimalRendererStage::VolumetricClouds);
    case RenderPassId::Fog:
    case RenderPassId::AerialPerspective:
        return IsFeatureEnabled(MinimalRendererStage::Fog);
    case RenderPassId::PostProcessing:
    case RenderPassId::Exposure:
    case RenderPassId::ToneMapping:
        return ShouldRunPostProcess();
    default:
        return true;
    }
}

bool RendererDebug::ShouldRunPostProcess() const {
    if (!IsMinimalRendererActive()) {
        return true;
    }
    return IsFeatureEnabled(MinimalRendererStage::Tonemapping)
        || IsFeatureEnabled(MinimalRendererStage::Bloom)
        || IsFeatureEnabled(MinimalRendererStage::AutoExposure);
}

bool RendererDebug::ShouldRenderGrid() const {
    if (!IsMinimalRendererActive()) {
        return true;
    }
    return false;
}

bool RendererDebug::ShouldApplyBloom() const {
    if (!IsMinimalRendererActive()) {
        return true;
    }
    return IsFeatureEnabled(MinimalRendererStage::Bloom);
}

bool RendererDebug::ShouldApplyAutoExposure() const {
    if (!IsMinimalRendererActive()) {
        return true;
    }
    return IsFeatureEnabled(MinimalRendererStage::AutoExposure);
}

bool RendererDebug::ShouldApplyTonemapping() const {
    if (!IsMinimalRendererActive()) {
        return true;
    }
    return IsFeatureEnabled(MinimalRendererStage::Tonemapping);
}

bool RendererDebug::ShouldUseDirectionalLighting() const {
    if (!IsMinimalRendererActive()) {
        return true;
    }
    return IsFeatureEnabled(MinimalRendererStage::DirectionalLight);
}

bool RendererDebug::ShouldUploadEnvironmentUniform() const {
    if (!IsMinimalRendererActive()) {
        return true;
    }
    return ShouldUseDirectionalLighting()
        || IsFeatureEnabled(MinimalRendererStage::SkyAtmosphere)
        || IsFeatureEnabled(MinimalRendererStage::VolumetricClouds)
        || IsFeatureEnabled(MinimalRendererStage::Fog)
        || ShouldRunPostProcess();
}

bool RendererDebug::ShouldCreateAtmosphereResources() const {
    if (!IsMinimalRendererActive()) {
        return true;
    }
    return IsFeatureEnabled(MinimalRendererStage::SkyAtmosphere);
}

bool RendererDebug::ShouldCreatePostProcessResources() const {
    if (!IsMinimalRendererActive()) {
        return true;
    }
    return ShouldRunPostProcess();
}

} // namespace we::runtime::renderer
