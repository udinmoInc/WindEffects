#include "Rendering/UITextureAtlasManager.h"
#include "Core/Logger.h"
#include <algorithm>

namespace we::UI {

UITextureAtlasManager::UITextureAtlasManager()
    : m_Device(VK_NULL_HANDLE)
    , m_PhysicalDevice(VK_NULL_HANDLE)
    , m_AtlasWidth(0)
    , m_AtlasHeight(0)
    , m_MaxAtlasSize(4096)
    , m_NextTextureId(1)
{
}

UITextureAtlasManager::~UITextureAtlasManager() {
    Shutdown();
}

bool UITextureAtlasManager::Initialize(VkDevice device,
                                       VkPhysicalDevice physicalDevice,
                                       uint32_t maxAtlasSize) {
    m_Device = device;
    m_PhysicalDevice = physicalDevice;
    m_MaxAtlasSize = maxAtlasSize;
    
    // Initialize with default atlas size
    if (!AllocateAtlas(2048, 2048)) {
        HE_ERROR("UITextureAtlasManager: Failed to allocate initial atlas");
        return false;
    }
    
    return true;
}

void UITextureAtlasManager::Shutdown() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    if (m_AtlasSampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_Device, m_AtlasSampler, nullptr);
        m_AtlasSampler = VK_NULL_HANDLE;
    }
    
    if (m_AtlasImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_Device, m_AtlasImageView, nullptr);
        m_AtlasImageView = VK_NULL_HANDLE;
    }
    
    if (m_AtlasImage != VK_NULL_HANDLE) {
        vkDestroyImage(m_Device, m_AtlasImage, nullptr);
        m_AtlasImage = VK_NULL_HANDLE;
    }
    
    if (m_AtlasMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_Device, m_AtlasMemory, nullptr);
        m_AtlasMemory = VK_NULL_HANDLE;
    }
    
    m_TextureEntries.clear();
    m_Slots.clear();
    m_NextTextureId = 1;
}

uint32_t UITextureAtlasManager::RegisterTexture(VkImageView imageView,
                                                 VkSampler sampler,
                                                 uint32_t width,
                                                 uint32_t height) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    uint32_t x, y;
    if (!FindFreeSlot(width, height, x, y)) {
        // Try to expand atlas
        uint32_t newWidth = std::min(m_AtlasWidth * 2, m_MaxAtlasSize);
        uint32_t newHeight = std::min(m_AtlasHeight * 2, m_MaxAtlasSize);
        
        if (!AllocateAtlas(newWidth, newHeight)) {
            HE_ERROR("UITextureAtlasManager: Failed to expand atlas");
            return 0;
        }
        
        if (!FindFreeSlot(width, height, x, y)) {
            HE_ERROR("UITextureAtlasManager: No space in atlas for texture");
            return 0;
        }
    }
    
    // Copy texture to atlas (simplified - would need proper blit in production)
    // For now, just track the allocation
    
    UITextureAtlasEntry entry;
    entry.imageView = imageView;
    entry.sampler = sampler;
    entry.atlasX = x;
    entry.atlasY = y;
    entry.width = width;
    entry.height = height;
    entry.lastUsedFrame = 0;
    
    uint32_t textureId = m_NextTextureId++;
    m_TextureEntries[textureId] = entry;
    
    // Mark slot as occupied
    for (auto& slot : m_Slots) {
        if (slot.x == x && slot.y == y && slot.width == width && slot.height == height) {
            slot.occupied = true;
            break;
        }
    }
    
    return textureId;
}

void UITextureAtlasManager::UnregisterTexture(uint32_t textureId) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto it = m_TextureEntries.find(textureId);
    if (it != m_TextureEntries.end()) {
        // Mark slot as free
        for (auto& slot : m_Slots) {
            if (slot.x == it->second.atlasX && 
                slot.y == it->second.atlasY &&
                slot.width == it->second.width && 
                slot.height == it->second.height) {
                slot.occupied = false;
                break;
            }
        }
        
        m_TextureEntries.erase(it);
    }
}

const UITextureAtlasEntry* UITextureAtlasManager::GetTextureEntry(uint32_t textureId) const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    auto it = m_TextureEntries.find(textureId);
    if (it != m_TextureEntries.end()) {
        return &it->second;
    }
    
    return nullptr;
}

void UITextureAtlasManager::GetTextureUV(uint32_t textureId, 
                                         float& u0, float& v0, 
                                         float& u1, float& v1) const {
    const UITextureAtlasEntry* entry = GetTextureEntry(textureId);
    if (entry) {
        u0 = static_cast<float>(entry->atlasX) / m_AtlasWidth;
        v0 = static_cast<float>(entry->atlasY) / m_AtlasHeight;
        u1 = static_cast<float>(entry->atlasX + entry->width) / m_AtlasWidth;
        v1 = static_cast<float>(entry->atlasY + entry->height) / m_AtlasHeight;
    } else {
        u0 = v0 = u1 = v1 = 0.0f;
    }
}

float UITextureAtlasManager::GetAtlasUtilization() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    uint32_t occupiedPixels = 0;
    for (const auto& slot : m_Slots) {
        if (slot.occupied) {
            occupiedPixels += slot.width * slot.height;
        }
    }
    
    uint32_t totalPixels = m_AtlasWidth * m_AtlasHeight;
    return totalPixels > 0 ? static_cast<float>(occupiedPixels) / totalPixels : 0.0f;
}

bool UITextureAtlasManager::AllocateAtlas(uint32_t width, uint32_t height) {
    // Destroy old atlas if exists
    if (m_AtlasImage != VK_NULL_HANDLE) {
        vkDestroyImage(m_Device, m_AtlasImage, nullptr);
        m_AtlasImage = VK_NULL_HANDLE;
    }
    if (m_AtlasMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_Device, m_AtlasMemory, nullptr);
        m_AtlasMemory = VK_NULL_HANDLE;
    }
    if (m_AtlasImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_Device, m_AtlasImageView, nullptr);
        m_AtlasImageView = VK_NULL_HANDLE;
    }
    
    m_AtlasWidth = width;
    m_AtlasHeight = height;
    
    // Create atlas image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateImage(m_Device, &imageInfo, nullptr, &m_AtlasImage) != VK_SUCCESS) {
        return false;
    }
    
    // Allocate memory
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_Device, m_AtlasImage, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = 0; // Would need proper memory type finding
    
    if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &m_AtlasMemory) != VK_SUCCESS) {
        vkDestroyImage(m_Device, m_AtlasImage, nullptr);
        m_AtlasImage = VK_NULL_HANDLE;
        return false;
    }
    
    vkBindImageMemory(m_Device, m_AtlasImage, m_AtlasMemory, 0);
    
    // Create image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_AtlasImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    if (vkCreateImageView(m_Device, &viewInfo, nullptr, &m_AtlasImageView) != VK_SUCCESS) {
        return false;
    }
    
    // Create sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    
    if (vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_AtlasSampler) != VK_SUCCESS) {
        return false;
    }
    
    // Initialize slot tracking (simplified - would use proper packing algorithm)
    m_Slots.clear();
    // Add initial free slot (entire atlas)
    m_Slots.push_back({false, 0, 0, width, height});
    
    return true;
}

bool UITextureAtlasManager::FindFreeSlot(uint32_t width, uint32_t height,
                                         uint32_t& outX, uint32_t& outY) {
    // Simple first-fit algorithm (would use guillotine packing in production)
    for (auto& slot : m_Slots) {
        if (!slot.occupied && slot.width >= width && slot.height >= height) {
            outX = slot.x;
            outY = slot.y;
            return true;
        }
    }
    return false;
}

void UITextureAtlasManager::UpdateAtlasDescriptorSet() {
    // Would update descriptor set with atlas image view and sampler
    // Implementation depends on descriptor pool setup
}

} // namespace we::UI
