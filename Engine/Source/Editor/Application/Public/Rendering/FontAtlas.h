#pragma once

#include "Application/Export.h"

#include <volk.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Core/DeviceContext.h"
#include "Rendering/UiGpuUpload.h"

namespace we::UI::Text {
class IFontAtlasBackend;
}

namespace we::UI {

struct GlyphInfo {
    float x0, y0, x1, y1;
    float u0, v0, u1, v1;
    float xadvance;
};

class APPLICATION_API FontAtlas {
public:
    FontAtlas();
    ~FontAtlas();

    bool Init(we::runtime::renderer::DeviceContext* context,
              we::runtime::renderer::ResourceManager* resources,
              UiGpuUpload* gpuUpload,
              const std::string& fontName,
              float fontHeightPx = 18.0f,
              int firstChar = 32,
              int numChars = 96,
              int width = 512,
              int height = 512);
    void Shutdown();

    bool EnsureTextGlyphs(std::string_view utf8Text);
    bool GetGlyphQuad(uint32_t codepoint, float* xpos, float* ypos, GlyphInfo& quad);
    bool GetCharQuad(int c, float* xpos, float* ypos, GlyphInfo& quad);

    VkDescriptorSet GetDescriptorSet() const { return m_DescriptorSet; }
    VkDescriptorSet& GetDescriptorSetRef() { return m_DescriptorSet; }
    VkImageView GetImageView() const { return m_ImageView; }
    VkSampler GetSampler() const { return m_Sampler; }
    float GetFontHeight() const { return m_FontHeight; }
    float GetMsdfPixelRange() const;
    int GetAtlasWidth() const;
    int GetAtlasHeight() const;
    int GetGlyphCount() const;
    bool IsGpuAtlasValid() const;

    using GpuAtlasRecreatedFn = std::function<void(VkImageView imageView, VkSampler sampler)>;
    void SetGpuAtlasRecreatedCallback(GpuAtlasRecreatedFn callback);

private:
    void DestroyGpuImage();
    bool EnsureGpuImage(int atlasWidth, int atlasHeight);
    bool UploadAtlasIfDirty();

    we::runtime::renderer::DeviceContext* m_Context = nullptr;
    we::runtime::renderer::ResourceManager* m_Resources = nullptr;
    UiGpuUpload* m_GpuUpload = nullptr;

    std::unique_ptr<Text::IFontAtlasBackend> m_Backend;

    float m_FontHeight = 18.0f;

    VkImage m_Image = VK_NULL_HANDLE;
    VkDeviceMemory m_Memory = VK_NULL_HANDLE;
    VkImageView m_ImageView = VK_NULL_HANDLE;
    VkSampler m_Sampler = VK_NULL_HANDLE;
    VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;

    int m_GpuWidth = 0;
    int m_GpuHeight = 0;
    VkImageLayout m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    GpuAtlasRecreatedFn m_GpuAtlasRecreatedCallback;
};

} // namespace we::UI
