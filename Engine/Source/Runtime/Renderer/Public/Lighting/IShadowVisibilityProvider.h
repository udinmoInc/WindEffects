// ==============================================================================
// WindEffects — Renderer — IShadowVisibilityProvider
// Modular sun/visibility — CSM today, ray-traced later without rewriting lighting.
// ==============================================================================
#pragma once

#include "Core/Math/Types.h"
#include "Renderer/Export.h"
#include "RHI/Types.h"

#include <cstdint>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace we::runtime::renderer {

/// Per-frame sun shadow data consumed by surface shaders / future RT.
struct ShadowFrameData {
    bool enabled = false;
    bool softShadows = false;
    bool contactShadows = false;
    uint32_t cascadeCount = 0;
    uint32_t cascadeResolution = 0;
    uint32_t atlasResolution = 0;
    float shadowDistance = 0.0f;
    float depthBias = 0.0f;
    float normalBias = 0.0f;
    float filterRadius = 0.0f;
    float cascadeBlend = 0.0f;
    float contactShadowLength = 0.0f;
    we::math::Vec3 sunTravelDirection{0.3f, -0.8f, 0.2f};
    we::rhi::RHITextureHandle atlas = we::rhi::RHITextureHandle::Invalid;
    we::rhi::RHITextureViewHandle atlasView = we::rhi::RHITextureViewHandle::Invalid;
    we::rhi::RHISamplerHandle comparisonSampler = we::rhi::RHISamplerHandle::Invalid;
    we::rhi::RHIBufferHandle cascadeBuffer = we::rhi::RHIBufferHandle::Invalid;
};

class RENDERER_API IShadowVisibilityProvider {
public:
    virtual ~IShadowVisibilityProvider() = default;

    [[nodiscard]] virtual bool IsEnabled() const = 0;
    [[nodiscard]] virtual const char* GetName() const = 0;
    [[nodiscard]] virtual const ShadowFrameData& GetFrameData() const = 0;
};

class RENDERER_API NullShadowVisibilityProvider final : public IShadowVisibilityProvider {
public:
    [[nodiscard]] bool IsEnabled() const override { return false; }
    [[nodiscard]] const char* GetName() const override { return "NullShadow"; }
    [[nodiscard]] const ShadowFrameData& GetFrameData() const override { return m_Data; }

private:
    ShadowFrameData m_Data{};
};

} // namespace we::runtime::renderer

#pragma warning(pop)
