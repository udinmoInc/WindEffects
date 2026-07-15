#include "Lighting/CloudRenderTargets.h"
#include "Core/DeviceContext.h"
#include "Core/Validation.h"
#include "Resource/ResourceManager.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::renderer {

CloudRenderTargets::~CloudRenderTargets() {
    Shutdown();
}

void CloudRenderTargets::Init(const CloudRenderTargetsConfig& config) {
    WE_VALIDATE_INIT(!m_Initialized, "CloudRenderTargets", "Already initialized.");
    WE_VALIDATE_INIT(config.deviceContext != nullptr, "CloudRenderTargets", "DeviceContext null.");
    WE_VALIDATE_INIT(config.resourceManager != nullptr, "CloudRenderTargets", "ResourceManager null.");

    m_DeviceContext = config.deviceContext;
    m_ResourceManager = config.resourceManager;
    m_FullWidth = std::max(1u, config.fullWidth);
    m_FullHeight = std::max(1u, config.fullHeight);
    m_ResolutionScale = std::clamp(config.resolutionScale, 0.25f, 1.0f);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.maxLod = 0.0f;
    WE_VALIDATE_INIT(
        vkCreateSampler(m_DeviceContext->GetDevice(), &samplerInfo, nullptr, &m_Sampler) == VK_SUCCESS,
        "CloudRenderTargets",
        "Failed to create sampler.");

    RecreateAll();
    m_Initialized = true;
}

void CloudRenderTargets::Shutdown() {
    if (!m_Initialized && m_Scratch.image == VK_NULL_HANDLE && m_Sampler == VK_NULL_HANDLE) {
        return;
    }

    DestroyTarget(m_Scratch);
    DestroyTarget(m_History[0]);
    DestroyTarget(m_History[1]);
    DestroyTarget(m_FallbackDepth);

    if (m_Sampler != VK_NULL_HANDLE && m_DeviceContext) {
        vkDestroySampler(m_DeviceContext->GetDevice(), m_Sampler, nullptr);
        m_Sampler = VK_NULL_HANDLE;
    }

    m_Initialized = false;
}

void CloudRenderTargets::Resize(uint32_t fullWidth, uint32_t fullHeight, float resolutionScale) {
    fullWidth = std::max(1u, fullWidth);
    fullHeight = std::max(1u, fullHeight);
    resolutionScale = std::clamp(resolutionScale, 0.25f, 1.0f);
    if (m_Initialized
        && fullWidth == m_FullWidth
        && fullHeight == m_FullHeight
        && resolutionScale == m_ResolutionScale) {
        return;
    }

    m_FullWidth = fullWidth;
    m_FullHeight = fullHeight;
    m_ResolutionScale = resolutionScale;

    if (!m_DeviceContext || !m_ResourceManager) {
        return;
    }

    DestroyTarget(m_Scratch);
    DestroyTarget(m_History[0]);
    DestroyTarget(m_History[1]);
    RecreateAll();
    m_Initialized = true;
}

void CloudRenderTargets::DestroyTarget(Target& target) {
    if (!m_DeviceContext) {
        return;
    }
    VkDevice device = m_DeviceContext->GetDevice();
    if (target.view != VK_NULL_HANDLE) {
        vkDestroyImageView(device, target.view, nullptr);
        target.view = VK_NULL_HANDLE;
    }
    if (target.image != VK_NULL_HANDLE) {
        m_ResourceManager->DestroyImage(target.image, target.memory);
        target.image = VK_NULL_HANDLE;
        target.memory = VK_NULL_HANDLE;
    }
    target.layout = VK_IMAGE_LAYOUT_UNDEFINED;
}

void CloudRenderTargets::CreateTarget(Target& target, uint32_t width, uint32_t height) {
    m_ResourceManager->CreateImage(
        width,
        height,
        m_Format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        target.image,
        target.memory);
    target.view = m_ResourceManager->CreateImageView(target.image, m_Format, VK_IMAGE_ASPECT_COLOR_BIT);
    target.layout = VK_IMAGE_LAYOUT_UNDEFINED;
}

void CloudRenderTargets::RecreateAll() {
    m_CloudWidth = std::max(1u, static_cast<uint32_t>(std::lround(m_FullWidth * m_ResolutionScale)));
    m_CloudHeight = std::max(1u, static_cast<uint32_t>(std::lround(m_FullHeight * m_ResolutionScale)));
    CreateTarget(m_Scratch, m_CloudWidth, m_CloudHeight);
    CreateTarget(m_History[0], m_CloudWidth, m_CloudHeight);
    CreateTarget(m_History[1], m_CloudWidth, m_CloudHeight);

    // 1x1 far-depth stub (cleared conceptually to 1.0 via UNDEFINED→unused when real depth exists).
    if (m_FallbackDepth.image == VK_NULL_HANDLE) {
        m_ResourceManager->CreateImage(
            1,
            1,
            VK_FORMAT_D32_SFLOAT,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_FallbackDepth.image,
            m_FallbackDepth.memory);
        m_FallbackDepth.view = m_ResourceManager->CreateImageView(
            m_FallbackDepth.image, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT);
        m_FallbackDepth.layout = VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

} // namespace we::runtime::renderer
