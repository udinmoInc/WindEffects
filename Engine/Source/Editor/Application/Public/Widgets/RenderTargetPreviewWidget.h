#pragma once

#include "Application/Export.h"
#include "Core/Widget.h"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace we::UI {

class APPLICATION_API RenderTargetPreviewWidget : public Widget {
public:
    RenderTargetPreviewWidget(const std::string& title,
        const std::vector<uint8_t>& rgba,
        uint32_t width,
        uint32_t height);

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void OnMouseDown(const MouseEvent& event) override;

    void SetOnClose(std::function<void()> callback) { m_OnClose = std::move(callback); }

private:
    std::string m_Title;
    std::vector<uint8_t> m_Rgba;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    Rect m_PreviewRect{};
    Rect m_CloseRect{};
    std::function<void()> m_OnClose;
};

} // namespace we::UI
