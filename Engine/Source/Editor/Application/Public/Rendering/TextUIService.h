#pragma once

#include "Application/Export.h"
#include "Text/TextEngine.h"

#include <volk.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace we::UI {

class OverlayRenderer;

class APPLICATION_API TextUIService {
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
        VkDescriptorSet& outTextureSet);

private:
    [[nodiscard]] we::runtime::text::layout::TextStyle BuildStyle(const DrawCommand& cmd) const;
    [[nodiscard]] VkDescriptorSet GetDescriptorForFont(we::runtime::text::FontHandle handle);

    OverlayRenderer* m_Renderer = nullptr;
    std::unique_ptr<we::runtime::text::ITextEngine> m_TextEngine;
    std::unique_ptr<we::runtime::text::rendering::ITextGpuBackend> m_TextBackend;
    std::unordered_map<we::runtime::text::FontHandle, VkDescriptorSet> m_FontDescriptors;
    we::runtime::text::FontHandle m_RegularFont = we::runtime::text::kInvalidFontHandle;
    we::runtime::text::FontHandle m_SemiBoldFont = we::runtime::text::kInvalidFontHandle;
};

} // namespace we::UI
