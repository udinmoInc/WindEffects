#pragma once

#include <volk.h>
#include <memory>
#include <string>
#include <vector>
#include <stb_truetype.h>

namespace HouseEngine {
class VulkanContext;
}

namespace HouseEngine::UI {

struct GlyphInfo {
    float x0, y0, x1, y1;
    float u0, v0, u1, v1;
    float xadvance;
};

class FontAtlas {
public:
    FontAtlas() = default;
    ~FontAtlas();

    // Load font from path, baking a specific range of codepoints
    bool Init(const std::shared_ptr<VulkanContext>& context, const std::string& fontName, int firstChar = 32, int numChars = 96, int width = 512, int height = 512);
    void Shutdown();

    // Get UV coordinates and vertex offsets for a single character
    bool GetCharQuad(int c, float* xpos, float* ypos, GlyphInfo& quad);

    VkDescriptorSet GetDescriptorSet() const { return m_DescriptorSet; }
    VkDescriptorSet& GetDescriptorSetRef() { return m_DescriptorSet; }
    VkImageView GetImageView() const { return m_ImageView; }
    VkSampler GetSampler() const { return m_Sampler; }
    float GetFontHeight() const { return m_FontHeight; }

private:
    std::shared_ptr<VulkanContext> m_Context;

    // Font parameters
    float m_FontHeight = 18.0f;
    int m_AtlasWidth = 512;
    int m_AtlasHeight = 512;
    int m_FirstChar = 32;
    int m_NumChars = 96;

    // stb_truetype baked char data (dynamically allocated)
    std::vector<stbtt_bakedchar> m_BakedChars;

    // Vulkan texture resources
    VkImage m_Image = VK_NULL_HANDLE;
    VkDeviceMemory m_Memory = VK_NULL_HANDLE;
    VkImageView m_ImageView = VK_NULL_HANDLE;
    VkSampler m_Sampler = VK_NULL_HANDLE;
    VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
};

} // namespace HouseEngine::UI
