#pragma once

#include "Application/Export.hpp"

#include <volk.h>
#include "Core/Widget.hpp"

namespace we::UI {

class APPLICATION_API Image : public Widget {
public:
    Image(VkDescriptorSet textureId = VK_NULL_HANDLE);
    virtual ~Image() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void SetTexture(VkDescriptorSet textureId) { m_TextureId = textureId; }
    VkDescriptorSet GetTexture() const { return m_TextureId; }

    void SetSize(const Size& size) { m_CustomSize = size; m_UseCustomSize = true; }
    void SetWidth(float w) { m_CustomSize.width = w; m_UseCustomSize = true; }
    void SetHeight(float h) { m_CustomSize.height = h; m_UseCustomSize = true; }

    void SetTintColor(const Color& tint) { m_TintColor = tint; m_TintBottom = Color::Transparent(); }
    void SetGradientTintColor(const Color& top, const Color& bottom) { m_TintColor = top; m_TintBottom = bottom; }

private:
    VkDescriptorSet m_TextureId = VK_NULL_HANDLE;
    Size m_CustomSize = { 64.0f, 64.0f };
    bool m_UseCustomSize = false;
    Color m_TintColor = Color::White();
    Color m_TintBottom = Color::Transparent();
};

} // namespace we::editor::application::UI
