// ==============================================================================
// WindEffects — Renderer — ShadowSystem
// Cascaded Shadow Maps — same Sun travel direction as atmosphere / direct light.
// ==============================================================================
#include "Lighting/ShadowSystem.h"
#include "Lighting/LightingPasses.h"
#include "Renderer/Graph/RenderGraph.h"
#include "Core/Math/GlmInterop.h"
#include "RHI/Desc.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <span>

namespace we::runtime::renderer {
namespace {

we::math::Vec3 Normalize3(we::math::Vec3 v, we::math::Vec3 fallback) {
    const float lenSq = v.x * v.x + v.y * v.y + v.z * v.z;
    if (lenSq < 1.0e-12f) {
        return fallback;
    }
    const float inv = 1.0f / std::sqrt(lenSq);
    return {v.x * inv, v.y * inv, v.z * inv};
}

float Snap(float v, float texel) {
    if (texel <= 1.0e-8f) {
        return v;
    }
    return std::floor(v / texel + 0.5f) * texel;
}

void ExtractProjFov(
    const we::math::Mat4& proj,
    float& outTanHalfX,
    float& outTanHalfY)
{
    const glm::mat4 p = we::math::AsGlm(proj);
    // After EditorCamera Y-flip, abs(proj[1][1]) = 1/tan(fovY/2).
    outTanHalfY = 1.0f / std::max(std::abs(p[1][1]), 1.0e-5f);
    outTanHalfX = 1.0f / std::max(std::abs(p[0][0]), 1.0e-5f);
}

glm::mat4 InvertRigidOrFallback(const glm::mat4& m) {
    const glm::mat4 inv = glm::inverse(m);
    // If singular, identity — cascade build will still produce finite matrices.
    if (!std::isfinite(inv[0][0]) || !std::isfinite(inv[3][3])) {
        return glm::mat4(1.0f);
    }
    return inv;
}

} // namespace

bool ShadowSystem::Initialize(we::rhi::IRHIDevice* device) {
    Shutdown();
    m_Device = device;
    return m_Device != nullptr;
}

void ShadowSystem::Configure(
    const ShadowQualitySettings& settings,
    uint32_t maxShadowMapResolution)
{
    m_Settings = settings;
    m_MaxShadowMapResolution = std::max(256u, maxShadowMapResolution);
    m_CascadeCount = 0;
    if (m_Settings.enabled) {
        m_CascadeCount = std::min(kMaxShadowCascades, std::max(1u, m_Settings.cascadeCount));
    }

    // Per-cascade resolution; 2×2 atlas for up to 4 cascades.
    float scale = std::clamp(m_Settings.resolutionScale, 0.25f, 2.0f);
    uint32_t res = static_cast<uint32_t>(
        static_cast<float>(m_MaxShadowMapResolution) * scale + 0.5f);
    res = std::clamp(res, 256u, 4096u);
    // Round down to multiple of 64 for stable texel snapping.
    res = (res / 64u) * 64u;
    if (res < 256u) {
        res = 256u;
    }
    m_CascadeResolution = res;
    m_AtlasResolution = res * 2u;
}

void ShadowSystem::BeginFrame(
    const CameraUniform& camera,
    const DirectionalLight* primaryLight)
{
    m_PrimaryCastsShadows =
        primaryLight != nullptr && primaryLight->enabled && primaryLight->castsShadows;

    if (!m_Settings.enabled || !m_PrimaryCastsShadows || m_CascadeCount == 0) {
        m_CascadeSplits = {0.0f, 0.0f, 0.0f, 0.0f};
        m_Cascades = {};
        m_GpuUniform = {};
        UpdateFrameData();
        return;
    }

    m_SunTravel = Normalize3(
        primaryLight->direction,
        we::math::Vec3{0.3f, -0.8f, 0.2f});

    const float nearZ = std::max(m_Settings.shadowNear, 0.05f);
    const float farZ = std::max(m_Settings.shadowDistance, nearZ + 1.0f);
    ComputeCascadeSplits(nearZ, farZ);
    BuildCascades(camera, nearZ, farZ);
    FillGpuUniform();
    (void)EnsureResources();
    UpdateFrameData();
}

void ShadowSystem::ComputeCascadeSplits(float nearZ, float farZ) {
    const float lambda = std::clamp(m_Settings.cascadeSplitLambda, 0.0f, 1.0f);
    for (uint32_t i = 0; i < 4; ++i) {
        if (i >= m_CascadeCount) {
            m_CascadeSplits[i] = 0.0f;
            continue;
        }
        const float p = static_cast<float>(i + 1) / static_cast<float>(m_CascadeCount);
        const float logSplit = nearZ * std::pow(farZ / nearZ, p);
        const float uniSplit = nearZ + (farZ - nearZ) * p;
        m_CascadeSplits[i] = lambda * logSplit + (1.0f - lambda) * uniSplit;
    }
}

void ShadowSystem::BuildCascades(
    const CameraUniform& camera,
    float nearZ,
    float farZ)
{
    (void)farZ;
    float tanHalfX = 1.0f;
    float tanHalfY = 1.0f;
    ExtractProjFov(camera.proj, tanHalfX, tanHalfY);

    const glm::mat4 view = we::math::AsGlm(camera.view);
    const glm::mat4 invView = InvertRigidOrFallback(view);
    const glm::vec3 sunDir = we::math::AsGlm(m_SunTravel);

    // Stable light basis from sun travel (same vector as direct lighting / atmosphere).
    glm::vec3 upHint = (std::abs(sunDir.y) > 0.99f)
        ? glm::vec3(0.0f, 0.0f, 1.0f)
        : glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(upHint, sunDir));
    glm::vec3 up = glm::normalize(glm::cross(sunDir, right));

    float splitNear = nearZ;
    for (uint32_t i = 0; i < m_CascadeCount; ++i) {
        const float splitFar = m_CascadeSplits[i];
        ShadowCascadeCpu& cascade = m_Cascades[i];
        cascade.splitNear = splitNear;
        cascade.splitFar = splitFar;

        // 8 frustum-slice corners in view space (RH, camera looks down -Z).
        const float zn = splitNear;
        const float zf = splitFar;
        const float nx = tanHalfX * zn;
        const float ny = tanHalfY * zn;
        const float fx = tanHalfX * zf;
        const float fy = tanHalfY * zf;
        const glm::vec3 viewCorners[8] = {
            {-nx, -ny, -zn}, {nx, -ny, -zn}, {-nx, ny, -zn}, {nx, ny, -zn},
            {-fx, -fy, -zf}, {fx, -fy, -zf}, {-fx, fy, -zf}, {fx, fy, -zf},
        };

        glm::vec3 worldCorners[8];
        glm::vec3 center(0.0f);
        for (int c = 0; c < 8; ++c) {
            const glm::vec4 w = invView * glm::vec4(viewCorners[c], 1.0f);
            worldCorners[c] = glm::vec3(w) / std::max(w.w, 1.0e-6f);
            center += worldCorners[c];
        }
        center *= (1.0f / 8.0f);

        float radius = 0.0f;
        for (const glm::vec3& p : worldCorners) {
            radius = std::max(radius, glm::length(p - center));
        }
        // Padding for PCF / filter kernel + normal bias.
        radius *= 1.08f;
        radius = std::max(radius, 1.0f);

        // Light-space AABB of the sphere (stable under camera rotation within cascade).
        const float texelWorld = (2.0f * radius) / static_cast<float>(std::max(m_CascadeResolution, 1u));

        // Snap cascade center in light-space XY so texels stay put as the camera moves.
        float centerX = glm::dot(center, right);
        float centerY = glm::dot(center, up);
        float centerZ = glm::dot(center, sunDir);
        centerX = Snap(centerX, texelWorld);
        centerY = Snap(centerY, texelWorld);
        const glm::vec3 snappedCenter =
            right * centerX + up * centerY + sunDir * centerZ;

        const float lightDist = radius + 50.0f;
        const glm::vec3 eye = snappedCenter - sunDir * lightDist;
        const glm::mat4 lightView = glm::lookAtRH(eye, snappedCenter, up);
        const float zNear = 1.0f;
        const float zFar = lightDist + radius + 50.0f;
        const glm::mat4 lightProj = glm::orthoRH_ZO(
            -radius, radius, -radius, radius, zNear, zFar);

        cascade.lightViewProj = we::math::FromGlm(lightProj * lightView);

        // 2×2 atlas tile.
        const uint32_t tileX = i % 2u;
        const uint32_t tileY = i / 2u;
        cascade.viewportX = tileX * m_CascadeResolution;
        cascade.viewportY = tileY * m_CascadeResolution;
        cascade.viewportW = m_CascadeResolution;
        cascade.viewportH = m_CascadeResolution;
        cascade.atlasScale[0] = 0.5f;
        cascade.atlasScale[1] = 0.5f;
        cascade.atlasOffset[0] = static_cast<float>(tileX) * 0.5f;
        cascade.atlasOffset[1] = static_cast<float>(tileY) * 0.5f;

        splitNear = splitFar;
    }

    for (uint32_t i = m_CascadeCount; i < kMaxShadowCascades; ++i) {
        m_Cascades[i] = {};
    }
}

void ShadowSystem::FillGpuUniform() {
    m_GpuUniform = {};
    for (uint32_t i = 0; i < m_CascadeCount; ++i) {
        m_GpuUniform.lightViewProj[i] = m_Cascades[i].lightViewProj;
        m_GpuUniform.cascadeSplits[i] = m_CascadeSplits[i];
        m_GpuUniform.atlasScaleBias[i][0] = m_Cascades[i].atlasScale[0];
        m_GpuUniform.atlasScaleBias[i][1] = m_Cascades[i].atlasScale[1];
        m_GpuUniform.atlasScaleBias[i][2] = m_Cascades[i].atlasOffset[0];
        m_GpuUniform.atlasScaleBias[i][3] = m_Cascades[i].atlasOffset[1];
    }
    m_GpuUniform.sunTravel[0] = m_SunTravel.x;
    m_GpuUniform.sunTravel[1] = m_SunTravel.y;
    m_GpuUniform.sunTravel[2] = m_SunTravel.z;
    m_GpuUniform.sunTravel[3] = 1.0f;

    m_GpuUniform.params0[0] = m_Settings.depthBias;
    m_GpuUniform.params0[1] = m_Settings.normalBias;
    m_GpuUniform.params0[2] = m_Settings.filterRadius;
    m_GpuUniform.params0[3] = static_cast<float>(m_CascadeCount);

    m_GpuUniform.params1[0] = m_Settings.cascadeBlend;
    m_GpuUniform.params1[1] = m_Settings.softShadows ? 1.0f : 0.0f;
    m_GpuUniform.params1[2] = m_Settings.contactShadows ? 1.0f : 0.0f;
    m_GpuUniform.params1[3] = m_Settings.contactShadowLength;

    m_GpuUniform.params2[0] = static_cast<float>(m_AtlasResolution);
    m_GpuUniform.params2[1] = static_cast<float>(m_CascadeResolution);
    m_GpuUniform.params2[2] = 1.0f;
    m_GpuUniform.params2[3] = m_Settings.shadowDistance;
}

void ShadowSystem::UpdateFrameData() {
    m_FrameData = {};
    m_FrameData.enabled = Enabled() && m_Atlas != we::rhi::RHITextureHandle::Invalid;
    m_FrameData.softShadows = m_Settings.softShadows;
    m_FrameData.contactShadows = m_Settings.contactShadows;
    m_FrameData.cascadeCount = m_CascadeCount;
    m_FrameData.cascadeResolution = m_CascadeResolution;
    m_FrameData.atlasResolution = m_AtlasResolution;
    m_FrameData.shadowDistance = m_Settings.shadowDistance;
    m_FrameData.depthBias = m_Settings.depthBias;
    m_FrameData.normalBias = m_Settings.normalBias;
    m_FrameData.filterRadius = m_Settings.filterRadius;
    m_FrameData.cascadeBlend = m_Settings.cascadeBlend;
    m_FrameData.contactShadowLength = m_Settings.contactShadowLength;
    m_FrameData.sunTravelDirection = m_SunTravel;
    m_FrameData.atlas = m_Atlas;
    m_FrameData.atlasView = m_AtlasView;
    m_FrameData.comparisonSampler = m_CompareSampler;
    m_FrameData.cascadeBuffer = m_CascadeBuffer;
}

bool ShadowSystem::EnsureResources() {
    if (!m_Device || m_CascadeResolution == 0) {
        return false;
    }

    const bool needRebuild =
        m_Atlas == we::rhi::RHITextureHandle::Invalid
        || m_FrameData.atlasResolution != m_AtlasResolution;

    if (needRebuild) {
        DestroyResources();

        we::rhi::TextureDesc atlasDesc{};
        atlasDesc.extent = {m_AtlasResolution, m_AtlasResolution, 1};
        atlasDesc.format = we::rhi::Format::D32_SFLOAT;
        atlasDesc.usage = we::rhi::TextureUsage::DepthStencil | we::rhi::TextureUsage::Sampled;
        atlasDesc.debugName = "SunShadow.CascadeAtlas";
        auto atlas = m_Device->CreateTexture(atlasDesc);
        if (!atlas) {
            return false;
        }
        m_Atlas = *atlas;

        we::rhi::TextureViewDesc viewDesc{};
        viewDesc.texture = m_Atlas;
        viewDesc.format = we::rhi::Format::D32_SFLOAT;
        viewDesc.debugName = "SunShadow.CascadeAtlasView";
        auto view = m_Device->CreateTextureView(viewDesc);
        if (!view) {
            DestroyResources();
            return false;
        }
        m_AtlasView = *view;

        we::rhi::SamplerDesc samp{};
        samp.magFilter = we::rhi::Filter::Linear;
        samp.minFilter = we::rhi::Filter::Linear;
        samp.mipFilter = we::rhi::Filter::Nearest;
        samp.addressU = we::rhi::AddressMode::ClampToEdge;
        samp.addressV = we::rhi::AddressMode::ClampToEdge;
        samp.addressW = we::rhi::AddressMode::ClampToEdge;
        samp.anisotropy = false;
        samp.compareEnable = true;
        samp.compareOp = we::rhi::CompareOp::LessOrEqual;
        samp.debugName = "SunShadow.CompareSampler";
        auto sampler = m_Device->CreateSampler(samp);
        if (!sampler) {
            DestroyResources();
            return false;
        }
        m_CompareSampler = *sampler;

        we::rhi::BufferDesc buf{};
        buf.size = sizeof(ShadowCascadeUniform);
        buf.usage = we::rhi::BufferUsage::Uniform;
        buf.memory = we::rhi::MemoryUsage::HostVisible;
        buf.debugName = "SunShadow.CascadeUBO";
        auto cascadeBuf = m_Device->CreateBuffer(buf);
        if (!cascadeBuf) {
            DestroyResources();
            return false;
        }
        m_CascadeBuffer = *cascadeBuf;
    }
    return true;
}

void ShadowSystem::DestroyResources() {
    if (!m_Device) {
        m_Atlas = we::rhi::RHITextureHandle::Invalid;
        m_AtlasView = we::rhi::RHITextureViewHandle::Invalid;
        m_CompareSampler = we::rhi::RHISamplerHandle::Invalid;
        m_CascadeBuffer = we::rhi::RHIBufferHandle::Invalid;
        return;
    }
    if (m_CascadeBuffer != we::rhi::RHIBufferHandle::Invalid) {
        (void)m_Device->DestroyBuffer(m_CascadeBuffer);
        m_CascadeBuffer = we::rhi::RHIBufferHandle::Invalid;
    }
    if (m_CompareSampler != we::rhi::RHISamplerHandle::Invalid) {
        (void)m_Device->DestroySampler(m_CompareSampler);
        m_CompareSampler = we::rhi::RHISamplerHandle::Invalid;
    }
    if (m_AtlasView != we::rhi::RHITextureViewHandle::Invalid) {
        (void)m_Device->DestroyTextureView(m_AtlasView);
        m_AtlasView = we::rhi::RHITextureViewHandle::Invalid;
    }
    if (m_Atlas != we::rhi::RHITextureHandle::Invalid) {
        (void)m_Device->DestroyTexture(m_Atlas);
        m_Atlas = we::rhi::RHITextureHandle::Invalid;
    }
}

void ShadowSystem::UploadGpuData() {
    if (!m_Device || m_CascadeBuffer == we::rhi::RHIBufferHandle::Invalid) {
        return;
    }
    (void)m_Device->UpdateBuffer(
        m_CascadeBuffer,
        std::span(reinterpret_cast<const uint8_t*>(&m_GpuUniform), sizeof(m_GpuUniform)));
}

void ShadowSystem::BuildRenderGraph(RenderGraph& graph) {
    if (!Enabled() || m_Atlas == we::rhi::RHITextureHandle::Invalid) {
        return;
    }
    graph.AddPass(std::make_unique<ShadowMapPass>(this));
}

void ShadowSystem::Shutdown() {
    DestroyResources();
    m_Device = nullptr;
    m_Settings = {};
    m_CascadeCount = 0;
    m_CascadeResolution = 0;
    m_AtlasResolution = 0;
    m_PrimaryCastsShadows = false;
    m_CascadeSplits = {0.0f, 0.0f, 0.0f, 0.0f};
    m_Cascades = {};
    m_GpuUniform = {};
    m_FrameData = {};
}

} // namespace we::runtime::renderer
