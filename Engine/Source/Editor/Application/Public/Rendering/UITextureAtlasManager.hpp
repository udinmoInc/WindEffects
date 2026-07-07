#pragma once

#include "Application/Export.hpp"

#include <volk.h>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace we::UI {

// Texture atlas entry
struct APPLICATION_API UITextureAtlasEntry {
    VkImageView imageView;
    VkSampler sampler;
    uint32_t atlasX;
    uint32_t atlasY;
    uint32_t width;
    uint32_t height;
    uint32_t lastUsedFrame;
};

// Texture atlas manager (UE5 Slate-inspired)
class APPLICATION_API UITextureAtlasManager {
public:
    UITextureAtlasManager();
    ~UITextureAtlasManager();

    bool Initialize(VkDevice device, 
                   VkPhysicalDevice physicalDevice,
                   uint32_t maxAtlasSize = 4096);
    void Shutdown();

    // Texture registration
    uint32_t RegisterTexture(VkImageView imageView, VkSampler sampler, 
                            uint32_t width, uint32_t height);
    void UnregisterTexture(uint32_t textureId);
    
    // Atlas access
    VkDescriptorSet GetAtlasDescriptorSet() const { return m_AtlasDescriptorSet; }
    const UITextureAtlasEntry* GetTextureEntry(uint32_t textureId) const;

    // UV coordinate conversion
    void GetTextureUV(uint32_t textureId, float& u0, float& v0, float& u1, float& v1) const;

    // Statistics
    uint32_t GetTextureCount() const { return static_cast<uint32_t>(m_TextureEntries.size()); }
    float GetAtlasUtilization() const;

private:
    bool AllocateAtlas(uint32_t width, uint32_t height);
    bool FindFreeSlot(uint32_t width, uint32_t height, uint32_t& outX, uint32_t& outY);
    void UpdateAtlasDescriptorSet();

    VkDevice m_Device;
    VkPhysicalDevice m_PhysicalDevice;
    
    VkImage m_AtlasImage = VK_NULL_HANDLE;
    VkDeviceMemory m_AtlasMemory = VK_NULL_HANDLE;
    VkImageView m_AtlasImageView = VK_NULL_HANDLE;
    VkSampler m_AtlasSampler = VK_NULL_HANDLE;
    VkDescriptorSet m_AtlasDescriptorSet = VK_NULL_HANDLE;
    
    uint32_t m_AtlasWidth;
    uint32_t m_AtlasHeight;
    uint32_t m_MaxAtlasSize;
    
    // Slot allocation tracking
    struct AtlasSlot {
        bool occupied;
        uint32_t x;
        uint32_t y;
        uint32_t width;
        uint32_t height;
    };
    std::vector<AtlasSlot> m_Slots;
    
    std::unordered_map<uint32_t, UITextureAtlasEntry> m_TextureEntries;
    uint32_t m_NextTextureId;
    mutable std::mutex m_Mutex;
};

} // namespace we::UI
