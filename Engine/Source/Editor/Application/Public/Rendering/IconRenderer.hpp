#pragma once

#include "Application/Export.hpp"

#include <volk.h>
#include <memory>
#include <unordered_map>
#include <string>
#include "Core/Geometry.hpp"
#include "Core/Icon.hpp"
#include "Core/Theme.hpp"

#include "Renderer/VulkanContext.hpp"

namespace we::UI {

// Cached icon texture
struct IconTexture {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    uint32_t width = 0;
    uint32_t height = 0;
};

// Icon renderer with caching support
class APPLICATION_API IconRenderer {
public:
    IconRenderer();
    ~IconRenderer();
    
    bool Init(const std::shared_ptr<we::runtime::renderer::VulkanContext>& context, VkDescriptorSetLayout textureLayout);
    void Shutdown();
    
    // Get or create icon texture at specified size (path or lucide name).
    VkDescriptorSet GetIcon(const std::string& iconName, uint32_t size);
    VkDescriptorSet GetLucideIcon(const std::string& iconName, uint32_t size, const Color& color, float strokeWidth = 0.0f);

    // Create a texture from raw RGBA bitmap data (for thumbnails)
    VkDescriptorSet CreateTextureFromBitmap(const std::vector<uint8_t>& bitmap, uint32_t width, uint32_t height);
    
    // Clear icon cache
    void ClearCache();
    
private:
    static std::string ResolveLucideSvgPath(const std::string& lucideName);

    // Parse SVG path and render to bitmap
    std::vector<uint8_t> RenderSVGToBitmap(const std::string& svgPath, uint32_t size, const Color& color, float strokeWidth = 0.0f);
    
    // Create Vulkan texture from bitmap
    bool CreateTexture(const std::vector<uint8_t>& bitmap, uint32_t width, uint32_t height, IconTexture& outTexture);
    
    // Destroy texture resources
    void DestroyTexture(IconTexture& texture);
    
    std::shared_ptr<we::runtime::renderer::VulkanContext> m_Context;
    VkDescriptorSetLayout m_TextureLayout = VK_NULL_HANDLE;
    
    // Icon cache: key = "iconName_size", value = texture
    std::unordered_map<std::string, IconTexture> m_Cache;
    
    // Default icon color
    Color m_DefaultColor = Theme::Get().TextPrimary;
};

} // namespace we::editor::application::UI
