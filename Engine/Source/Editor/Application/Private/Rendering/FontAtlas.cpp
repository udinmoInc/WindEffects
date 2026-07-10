#include "Rendering/FontAtlas.h"
#include "Rendering/Text/MsdfFontAtlasBackend.h"
#include "Rendering/Text/Utf8.h"
#include "Rendering/UiDebugImageWriter.h"
#include "Rendering/UiGpuUpload.h"
#include "Core/DeviceContext.h"
#include "Resource/ResourceManager.h"
#include "Core/ImageBarriers.h"
#include "Core/Logger.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace we::UI {

namespace {

constexpr const char* kAtlasDebugPaths[] = {
    "Build/Database/UI/font_atlas_debug.bmp",
    "../Build/Database/UI/font_atlas_debug.bmp",
    "../../Build/Database/UI/font_atlas_debug.bmp",
    "../../../Build/Database/UI/font_atlas_debug.bmp",
};

void SaveAtlasDebugImage(const Text::IFontAtlasBackend* backend) {
    if (!backend) {
        return;
    }
    const auto& pixels = backend->GetAtlasPixels();
    const int width = backend->GetAtlasWidth();
    const int height = backend->GetAtlasHeight();
    if (pixels.empty() || width <= 0 || height <= 0) {
        return;
    }
    for (const char* path : kAtlasDebugPaths) {
        if (SaveBmpRgba(path, pixels, static_cast<uint32_t>(width), static_cast<uint32_t>(height))) {
            return;
        }
    }
}

} // namespace

FontAtlas::FontAtlas() = default;

FontAtlas::~FontAtlas() {
    Shutdown();
}

void FontAtlas::SetGpuAtlasRecreatedCallback(GpuAtlasRecreatedFn callback) {
    m_GpuAtlasRecreatedCallback = std::move(callback);
}

bool FontAtlas::IsGpuAtlasValid() const {
    return m_Image != VK_NULL_HANDLE && m_ImageView != VK_NULL_HANDLE && m_Sampler != VK_NULL_HANDLE
        && m_GpuWidth > 0 && m_GpuHeight > 0;
}

int FontAtlas::GetGlyphCount() const {
    return m_Backend ? m_Backend->GetGlyphCount() : 0;
}

const std::vector<uint8_t>& FontAtlas::GetCpuAtlasPixels() const {
    static const std::vector<uint8_t> kEmpty{};
    return m_Backend ? m_Backend->GetAtlasPixels() : kEmpty;
}

bool FontAtlas::Init(we::runtime::renderer::DeviceContext* context,
                     we::runtime::renderer::ResourceManager* resources,
                     UiGpuUpload* gpuUpload,
                     const std::string& fontName,
                     float fontHeightPx,
                     int /*firstChar*/,
                     int /*numChars*/,
                     int width,
                     int height,
                     const std::string& fallbackFontPath) {
    m_Context = context;
    m_Resources = resources;
    m_GpuUpload = gpuUpload;
    if (!m_Context || !m_Resources || !m_GpuUpload) {
        return false;
    }

    m_FontHeight = std::clamp(fontHeightPx, 10.0f, 96.0f);
    m_Backend = Text::CreateMsdfFontAtlasBackend();

    Text::FontAtlasBakeRequest request{};
    request.primaryFontPath = fontName;
    request.fallbackFontPath = fallbackFontPath.empty() ? "Assets/Fonts/Inter-Regular.otf" : fallbackFontPath;
    request.emSizePx = m_FontHeight;
    request.msdfPixelRange = std::max(4.0f, m_FontHeight * 0.22f);
    request.atlasWidth = width;
    request.atlasHeight = height;

    if (!m_Backend->Initialize(request)) {
        HE_ERROR("FontAtlas: MSDF backend initialization failed");
        m_Backend.reset();
        return false;
    }

    Text::FontFaceHandles faces{};
    if (m_Backend->GetFontFaces(faces)) {
        m_TextLayout.Initialize(faces.primary, faces.fallback);
    }

    SaveAtlasDebugImage(m_Backend.get());

    if (!UploadAtlasIfDirty()) {
        HE_ERROR("FontAtlas: GPU atlas upload failed");
        return false;
    }

    return true;
}

void FontAtlas::Shutdown() {
    m_TextLayout.Shutdown();
    if (m_Backend) {
        m_Backend->Shutdown();
        m_Backend.reset();
    }

    DestroyGpuImage();

    m_Context = nullptr;
    m_Resources = nullptr;
    m_GpuUpload = nullptr;
    m_GpuAtlasRecreatedCallback = nullptr;
}

void FontAtlas::DestroyGpuImage() {
    if (!m_Context) {
        m_Image = VK_NULL_HANDLE;
        m_Memory = VK_NULL_HANDLE;
        m_ImageView = VK_NULL_HANDLE;
        m_Sampler = VK_NULL_HANDLE;
        m_GpuWidth = 0;
        m_GpuHeight = 0;
        m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        return;
    }

    VkDevice device = m_Context->GetDevice();
    if (m_Sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, m_Sampler, nullptr);
        m_Sampler = VK_NULL_HANDLE;
    }
    if (m_ImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, m_ImageView, nullptr);
        m_ImageView = VK_NULL_HANDLE;
    }
    if (m_Image != VK_NULL_HANDLE) {
        vkDestroyImage(device, m_Image, nullptr);
        m_Image = VK_NULL_HANDLE;
    }
    if (m_Memory != VK_NULL_HANDLE) {
        vkFreeMemory(device, m_Memory, nullptr);
        m_Memory = VK_NULL_HANDLE;
    }
    m_GpuWidth = 0;
    m_GpuHeight = 0;
    m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

bool FontAtlas::EnsureGpuImage(int atlasWidth, int atlasHeight) {
    if (atlasWidth <= 0 || atlasHeight <= 0) {
        return false;
    }
    if (m_Image != VK_NULL_HANDLE && m_GpuWidth == atlasWidth && m_GpuHeight == atlasHeight) {
        return true;
    }

    DestroyGpuImage();

    VkDevice device = m_Context->GetDevice();
    m_Resources->CreateImage(
        static_cast<uint32_t>(atlasWidth),
        static_cast<uint32_t>(atlasHeight),
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_Image,
        m_Memory);
    m_ImageView = m_Resources->CreateImageView(m_Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &m_Sampler) != VK_SUCCESS) {
        HE_ERROR("FontAtlas: failed to create sampler");
        DestroyGpuImage();
        return false;
    }

    m_GpuWidth = atlasWidth;
    m_GpuHeight = atlasHeight;
    m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (m_GpuAtlasRecreatedCallback) {
        m_GpuAtlasRecreatedCallback(m_ImageView, m_Sampler);
    }
    return true;
}

bool FontAtlas::EnsureTextGlyphs(std::string_view utf8Text) {
    if (!m_Backend) {
        return false;
    }

    std::vector<uint32_t> codepoints;
    Text::DecodeUtf8(utf8Text, codepoints);
    if (!m_Backend->EnsureGlyphs(codepoints)) {
        HE_ERROR("FontAtlas: EnsureGlyphs failed during text layout");
        return false;
    }
    if (!UploadAtlasIfDirty()) {
        HE_ERROR("FontAtlas: GPU atlas re-upload failed after glyph expansion");
        return false;
    }
    return true;
}

bool FontAtlas::ShapeText(std::string_view utf8Text, std::vector<Text::ShapedGlyph>& outGlyphs) {
    return m_TextLayout.Shape(utf8Text, outGlyphs);
}

bool FontAtlas::GetGlyphQuad(uint32_t codepoint, float* xpos, float* ypos, GlyphInfo& quad) {
    if (!m_Backend) {
        return false;
    }
    return m_Backend->GetGlyphQuad(codepoint, xpos, ypos, quad);
}

bool FontAtlas::GetGlyphQuadAt(uint32_t codepoint, float penX, float penY, GlyphInfo& quad) const {
    if (!m_Backend) {
        return false;
    }
    return m_Backend->GetGlyphQuadAt(codepoint, penX, penY, quad);
}

float FontAtlas::GetGlyphAdvance(uint32_t codepoint) const {
    return m_Backend ? m_Backend->GetGlyphAdvance(codepoint) : 0.0f;
}

bool FontAtlas::GetCharQuad(int c, float* xpos, float* ypos, GlyphInfo& quad) {
    if (c < 0) {
        return false;
    }
    return GetGlyphQuad(static_cast<uint32_t>(c), xpos, ypos, quad);
}

float FontAtlas::MeasureText(std::string_view utf8Text, float fontSize) const {
    if (!m_Backend) {
        return 0.0f;
    }
    std::vector<Text::ShapedGlyph> glyphs;
    if (!m_TextLayout.Shape(utf8Text, glyphs)) {
        return 0.0f;
    }
    float width = 0.0f;
    for (const Text::ShapedGlyph& glyph : glyphs) {
        width += glyph.hbAdvanceX;
    }
    const float scale = m_FontHeight > 0.0f ? (fontSize / m_FontHeight) : 1.0f;
    return width * scale;
}

float FontAtlas::GetAscender() const {
    return m_TextLayout.GetMetrics().ascender;
}

float FontAtlas::GetLineHeight() const {
    const float lineHeight = m_TextLayout.GetMetrics().lineHeight;
    return lineHeight > 0.0f ? lineHeight : m_FontHeight;
}

float FontAtlas::GetMsdfPixelRange() const {
    return m_Backend ? m_Backend->GetMsdfPixelRange() : 4.0f;
}

int FontAtlas::GetAtlasWidth() const {
    return m_Backend ? m_Backend->GetAtlasWidth() : 0;
}

int FontAtlas::GetAtlasHeight() const {
    return m_Backend ? m_Backend->GetAtlasHeight() : 0;
}

bool FontAtlas::UploadAtlasIfDirty() {
    if (!m_Backend || !m_Backend->IsDirty()) {
        return true;
    }

    const auto& pixels = m_Backend->GetAtlasPixels();
    const int atlasWidth = m_Backend->GetAtlasWidth();
    const int atlasHeight = m_Backend->GetAtlasHeight();
    if (pixels.empty() || atlasWidth <= 0 || atlasHeight <= 0) {
        HE_ERROR("FontAtlas: dirty atlas has no pixel data");
        return false;
    }

    if (!EnsureGpuImage(atlasWidth, atlasHeight)) {
        return false;
    }

    SaveAtlasDebugImage(m_Backend.get());

    VkDevice device = m_Context->GetDevice();
    const VkDeviceSize imageSize = static_cast<VkDeviceSize>(pixels.size());

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
    std::memcpy(mapped, pixels.data(), pixels.size());
    vkUnmapMemory(device, stagingMemory);

    m_GpuUpload->SubmitOneTime([&](VkCommandBuffer cmd) {
        const VkImageLayout oldLayout = m_ImageLayout;
        const VkPipelineStageFlags srcStage = (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED)
            ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
            : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        const VkAccessFlags srcAccess = (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED)
            ? 0
            : VK_ACCESS_SHADER_READ_BIT;

        we::runtime::renderer::TransitionImageLayout(
            cmd,
            m_Image,
            oldLayout,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            srcAccess,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            srcStage,
            VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = { static_cast<uint32_t>(atlasWidth), static_cast<uint32_t>(atlasHeight), 1 };
        vkCmdCopyBufferToImage(cmd, stagingBuffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        we::runtime::renderer::TransitionImageLayout(
            cmd,
            m_Image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        m_ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    });

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);

    m_Backend->ClearDirty();
    return true;
}

} // namespace we::UI
