#pragma once

#include "KindUI/Export.h"
#include "RHI/Types.h"
#include "KindUI/Core/Widget.h"

namespace we::runtime::kindui {

class KINDUI_API Image : public Widget {
public:
    Image(we::rhi::RHIDescriptorSetHandle textureId = we::rhi::RHIDescriptorSetHandle::Invalid);
    virtual ~Image() = default;

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;

    void SetTexture(we::rhi::RHIDescriptorSetHandle textureId) { m_TextureId = textureId; }
    we::rhi::RHIDescriptorSetHandle GetTexture() const { return m_TextureId; }

    void SetSize(const Size& size) { m_CustomSize = size; m_UseCustomSize = true; }
    void SetWidth(float w) { m_CustomSize.width = w; m_UseCustomSize = true; }
    void SetHeight(float h) { m_CustomSize.height = h; m_UseCustomSize = true; }

    void SetTintColor(const Color& tint) { m_TintColor = tint; m_TintBottom = Color::Transparent(); }
    void SetGradientTintColor(const Color& top, const Color& bottom) { m_TintColor = top; m_TintBottom = bottom; }

private:
    we::rhi::RHIDescriptorSetHandle m_TextureId = we::rhi::RHIDescriptorSetHandle::Invalid;
    Size m_CustomSize = { 64.0f, 64.0f };
    bool m_UseCustomSize = false;
    Color m_TintColor = Color::White();
    Color m_TintBottom = Color::Transparent();
};

} // namespace we::runtime::kindui
