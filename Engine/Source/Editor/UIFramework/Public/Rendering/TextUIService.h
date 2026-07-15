#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "Rendering/OverlayRenderer.h"
#include "Core/PaintContext.h"
#include "Text/TextEngine.h"
#include "RHI/Types.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace WindEffects::Editor::UI {

class OverlayRenderer;

class UIFRAMEWORK_API TextUIService {
public:
    TextUIService();
    ~TextUIService();

    bool Initialize(OverlayRenderer* renderer);
    void Shutdown();

    [[nodiscard]] we::runtime::text::ITextEngine* GetEngine() const { return m_TextEngine.get(); }

    [[nodiscard]] float MeasureText(const std::string& text, float fontSize, bool bold) const;
    bool GenerateTextGeometry(
        const DrawCommand& cmd,
        std::vector<UIVertex2>& vertices,
        std::vector<uint32_t>& indices,
        we::rhi::RHIDescriptorSetHandle& outTextureSet,
        UIRenderBatch* outBatchInfo = nullptr);

private:
    struct FontGpuAtlas {
        // TODO(migr to VulkanRHI): opaque native handles
        uint64_t image = 0;
        uint64_t memory = 0;
        uint64_t imageView = 0;
        uint64_t sampler = 0;
        we::rhi::RHIDescriptorSetHandle descriptorSet = we::rhi::RHIDescriptorSetHandle::Invalid;
        uint32_t layout = 0; // VkImageLayout as uint32
        uint32_t width = 0;
        uint32_t height = 0;
    };

    [[nodiscard]] we::runtime::text::layout::TextStyle BuildStyle(const DrawCommand& cmd) const;
    [[nodiscard]] we::rhi::RHIDescriptorSetHandle GetDescriptorForFont(we::runtime::text::FontHandle handle);
    bool UploadFontAtlas(we::runtime::text::FontHandle handle, FontGpuAtlas& gpuAtlas);

    OverlayRenderer* m_Renderer = nullptr;
    std::unique_ptr<we::runtime::text::ITextEngine> m_TextEngine;
    std::unordered_map<we::runtime::text::FontHandle, FontGpuAtlas> m_FontAtlases;
    we::runtime::text::FontHandle m_RegularFont = we::runtime::text::kInvalidFontHandle;
    we::runtime::text::FontHandle m_SemiBoldFont = we::runtime::text::kInvalidFontHandle;
};

} // namespace WindEffects::Editor::UI
