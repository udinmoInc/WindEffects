#include "Rendering/Icons/AtlasManager.h"

#include "Core/DeviceContext.h"
#include "Core/ImageBarriers.h"
#include "Core/Logger.h"
#include "Icons/Assets/IconAsset.h"
#include "Rendering/IconMetrics.h"
#include "Rendering/UiGpuUpload.h"
#include "Resource/ResourceManager.h"

#include <cstring>
#include <cstdlib>
#include <stdexcept>

namespace WindEffects::Editor::UI {

namespace {

uint32_t TierIndex(uint32_t tierPx)
{
    for (uint32_t i = 0; i < IconMetrics::kAtlasTierCount; ++i) {
        if (IconMetrics::kAtlasTiers[i] == tierPx) {
            return i;
        }
    }
    return UINT32_MAX;
}

bool IsAtlasAuditEnabled()
{
    const char* value = std::getenv("WE_ICONS_ATLAS_AUDIT");
    return value != nullptr && value[0] != '\0' && value[0] != '0';
}

} // namespace

AtlasManager::~AtlasManager()
{
    Shutdown();
}

bool AtlasManager::Init(
    we::runtime::renderer::DeviceContext* context,
    we::runtime::renderer::ResourceManager* resources,
    UiGpuUpload* gpuUpload,
    VkDescriptorPool descriptorPool,
    VkDescriptorSetLayout textureLayout)
{
    m_Context = context;
    m_Resources = resources;
    m_GpuUpload = gpuUpload;
    m_DescriptorPool = descriptorPool;
    m_TextureLayout = textureLayout;
    return context != nullptr && resources != nullptr && gpuUpload != nullptr
        && descriptorPool != VK_NULL_HANDLE && textureLayout != VK_NULL_HANDLE;
}

void AtlasManager::Shutdown()
{
    std::scoped_lock lock(m_Mutex);
    for (auto& tier : m_Tiers) {
        DestroyTier(tier);
    }
    for (auto& path : m_TierPaths) {
        path.clear();
    }
}

bool AtlasManager::LoadTierFromFile(const uint32_t tierPx, const std::filesystem::path& atlasPath)
{
    const uint32_t index = TierIndex(tierPx);
    if (index == UINT32_MAX) {
        return false;
    }

    const auto loaded = ::we::runtime::icons::assets::IconAtlasReader::LoadFromFile(atlasPath);
    if (!loaded.ok) {
        return false;
    }

    std::scoped_lock lock(m_Mutex);
    DestroyTier(m_Tiers[index]);
    m_TierPaths[index] = atlasPath;
    m_Tiers[index].tierPx = tierPx;
    const bool uploaded = UploadAtlasPage(
        m_Tiers[index],
        loaded.value.page.rgba,
        loaded.value.page.width,
        loaded.value.page.height);
    m_Tiers[index].ready = uploaded;

    if (uploaded && IsAtlasAuditEnabled()) {
        HE_INFO(
            "[Icons][Audit] uploaded tier=" + std::to_string(tierPx)
            + " path=" + atlasPath.string()
            + " size=" + std::to_string(loaded.value.page.width) + "x" + std::to_string(loaded.value.page.height)
            + " bytes=" + std::to_string(loaded.value.page.rgba.size())
            + " format=VK_FORMAT_R8G8B8A8_UNORM"
            + " descriptor=0x" + std::to_string(reinterpret_cast<uintptr_t>(m_Tiers[index].descriptorSet)));
    }

    return uploaded;
}

bool AtlasManager::EnsureTierLoaded(const uint32_t tierPx, const std::filesystem::path& atlasPath)
{
    const uint32_t index = TierIndex(tierPx);
    if (index == UINT32_MAX) {
        return false;
    }

    {
        std::scoped_lock lock(m_Mutex);
        if (m_Tiers[index].ready) {
            return true;
        }
    }
    return LoadTierFromFile(tierPx, atlasPath);
}

const AtlasGpuResource* AtlasManager::GetTier(const uint32_t tierPx) const
{
    const uint32_t index = TierIndex(tierPx);
    if (index == UINT32_MAX) {
        return nullptr;
    }
    std::scoped_lock lock(m_Mutex);
    return m_Tiers[index].ready ? &m_Tiers[index] : nullptr;
}

bool AtlasManager::IsTierReady(const uint32_t tierPx) const
{
    return GetTier(tierPx) != nullptr;
}

void AtlasManager::WaitDeviceIdle() const
{
    if (m_Context) {
        vkDeviceWaitIdle(m_Context->GetDevice());
    }
}

bool AtlasManager::UploadAtlasPage(
    AtlasGpuResource& tier,
    const std::vector<uint8_t>& rgba,
    const uint32_t width,
    const uint32_t height)
{
    if (!m_Context || !m_Resources || !m_GpuUpload || rgba.empty() || width == 0 || height == 0) {
        return false;
    }

    VkDevice device = m_Context->GetDevice();
    const VkDeviceSize imageSize = static_cast<VkDeviceSize>(rgba.size());

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    m_Resources->CreateBuffer(
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingMemory);

    void* mapped = nullptr;
    vkMapMemory(device, stagingMemory, 0, imageSize, 0, &mapped);
    std::memcpy(mapped, rgba.data(), rgba.size());
    vkUnmapMemory(device, stagingMemory);

    m_Resources->CreateImage(
        width,
        height,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        tier.image,
        tier.memory);

    if (tier.image == VK_NULL_HANDLE) {
        return false;
    }

    m_GpuUpload->SubmitOneTime([&](VkCommandBuffer cmd) {
        we::runtime::renderer::TransitionImageLayout(
            cmd,
            tier.image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            0,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = {width, height, 1};
        vkCmdCopyBufferToImage(cmd, stagingBuffer, tier.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        we::runtime::renderer::TransitionImageLayout(
            cmd,
            tier.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    });

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);

    tier.view = m_Resources->CreateImageView(tier.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
    tier.width = width;
    tier.height = height;

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &tier.sampler) != VK_SUCCESS) {
        return false;
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_TextureLayout;
    if (vkAllocateDescriptorSets(device, &allocInfo, &tier.descriptorSet) != VK_SUCCESS) {
        return false;
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = tier.view;
    imageInfo.sampler = tier.sampler;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = tier.descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;
    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

    return true;
}

void AtlasManager::DestroyTier(AtlasGpuResource& tier)
{
    if (!m_Context) {
        tier = {};
        return;
    }

    VkDevice device = m_Context->GetDevice();
    if (tier.descriptorSet != VK_NULL_HANDLE && m_DescriptorPool != VK_NULL_HANDLE) {
        vkFreeDescriptorSets(device, m_DescriptorPool, 1, &tier.descriptorSet);
    }
    if (tier.sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, tier.sampler, nullptr);
    }
    if (tier.view != VK_NULL_HANDLE) {
        vkDestroyImageView(device, tier.view, nullptr);
    }
    if (tier.image != VK_NULL_HANDLE) {
        vkDestroyImage(device, tier.image, nullptr);
    }
    if (tier.memory != VK_NULL_HANDLE) {
        vkFreeMemory(device, tier.memory, nullptr);
    }
    tier = {};
}

} // namespace WindEffects::Editor::UI
