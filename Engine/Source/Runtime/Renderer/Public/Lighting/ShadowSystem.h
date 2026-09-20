// ==============================================================================
// WindEffects — Renderer — ShadowSystem
// Cascaded Shadow Maps for the authoritative Sun directional light.
// ==============================================================================
#pragma once

#pragma warning(push)
#pragma warning(disable : 4251)

#include "Camera/CameraUniform.h"
#include "Lighting/IShadowVisibilityProvider.h"
#include "Lighting/LightingTypes.h"
#include "Lighting/ShadowCascadeUniform.h"
#include "Renderer/Export.h"
#include "Renderer/Scalability/RenderingSettings.h"
#include "RHI/IRHI.h"
#include "RHI/Types.h"

#include <array>
#include <cstdint>

namespace we::runtime::renderer {

class RenderGraph;

struct ShadowCascadeCpu {
    we::math::Mat4 lightViewProj{};
    float splitNear = 0.0f;
    float splitFar = 0.0f;
    float atlasScale[2]{0.5f, 0.5f};
    float atlasOffset[2]{0.0f, 0.0f};
    uint32_t viewportX = 0;
    uint32_t viewportY = 0;
    uint32_t viewportW = 0;
    uint32_t viewportH = 0;
};

/// Outdoor CSM: practical splits, texel-snapped ortho, PCF-ready atlas.
/// Implements IShadowVisibilityProvider so RT shadows can replace this later.
class RENDERER_API ShadowSystem final : public IShadowVisibilityProvider {
public:
    bool Initialize(we::rhi::IRHIDevice* device);
    void Configure(const ShadowQualitySettings& settings, uint32_t maxShadowMapResolution);
    void BeginFrame(const CameraUniform& camera, const DirectionalLight* primaryLight);

    /// Registers shadow atlas clear + caster hook pass.
    void BuildRenderGraph(RenderGraph& graph);
    void UploadGpuData();
    void Shutdown();

    [[nodiscard]] bool Enabled() const override {
        return m_Settings.enabled && m_PrimaryCastsShadows && m_CascadeCount > 0;
    }
    [[nodiscard]] const char* GetName() const override { return "CascadedShadowMaps"; }
    [[nodiscard]] const ShadowFrameData& GetFrameData() const override { return m_FrameData; }

    [[nodiscard]] uint32_t CascadeCount() const { return m_CascadeCount; }
    [[nodiscard]] float ResolutionScale() const { return m_Settings.resolutionScale; }
    [[nodiscard]] bool SoftShadows() const { return m_Settings.softShadows; }
    [[nodiscard]] const ShadowQualitySettings& Settings() const { return m_Settings; }
    [[nodiscard]] const std::array<float, 4>& CascadeSplits() const { return m_CascadeSplits; }
    [[nodiscard]] const ShadowCascadeUniform& GpuUniform() const { return m_GpuUniform; }
    [[nodiscard]] const std::array<ShadowCascadeCpu, kMaxShadowCascades>& Cascades() const {
        return m_Cascades;
    }
    [[nodiscard]] we::math::Vec3 SunTravelDirection() const { return m_SunTravel; }

private:
    void DestroyResources();
    bool EnsureResources();
    void ComputeCascadeSplits(float nearZ, float farZ);
    void BuildCascades(const CameraUniform& camera, float nearZ, float farZ);
    void FillGpuUniform();
    void UpdateFrameData();

    we::rhi::IRHIDevice* m_Device = nullptr;
    ShadowQualitySettings m_Settings{};
    uint32_t m_MaxShadowMapResolution = 2048;
    uint32_t m_CascadeCount = 0;
    uint32_t m_CascadeResolution = 0;
    uint32_t m_AtlasResolution = 0;
    bool m_PrimaryCastsShadows = false;
    we::math::Vec3 m_SunTravel{0.3f, -0.8f, 0.2f};

    std::array<float, 4> m_CascadeSplits{0.0f, 0.0f, 0.0f, 0.0f};
    std::array<ShadowCascadeCpu, kMaxShadowCascades> m_Cascades{};
    ShadowCascadeUniform m_GpuUniform{};
    ShadowFrameData m_FrameData{};

    we::rhi::RHITextureHandle m_Atlas = we::rhi::RHITextureHandle::Invalid;
    we::rhi::RHITextureViewHandle m_AtlasView = we::rhi::RHITextureViewHandle::Invalid;
    we::rhi::RHISamplerHandle m_CompareSampler = we::rhi::RHISamplerHandle::Invalid;
    we::rhi::RHIBufferHandle m_CascadeBuffer = we::rhi::RHIBufferHandle::Invalid;
};

} // namespace we::runtime::renderer

#pragma warning(pop)
