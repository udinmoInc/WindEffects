#include "Core/PaintContext.h"

namespace we::UI {

void PaintContext::PushClipRect(const Rect& clip) {
    if (m_ClipStack.empty()) {
        m_ClipStack.push_back(clip);
    } else {
        // Intersect with parent clip rect
        Rect parentClip = m_ClipStack.back();
        m_ClipStack.push_back(parentClip.Intersect(clip));
    }
}

void PaintContext::PopClipRect() {
    if (!m_ClipStack.empty()) {
        m_ClipStack.pop_back();
    }
}

Rect PaintContext::GetCurrentClipRect() const {
    if (m_ClipStack.empty()) {
        // Return a massive rect representing no clipping
        return { 0.0f, 0.0f, 100000.0f, 100000.0f };
    }
    return m_ClipStack.back();
}

void PaintContext::DrawRect(const Rect& rect, const Color& color, float borderRadius) {
    DrawCommand cmd{};
    cmd.type = DrawCommandType::Rect;
    cmd.rect = rect;
    cmd.color = color;
    cmd.clipRect = GetCurrentClipRect();
    cmd.borderRadius = borderRadius;
    m_Commands.push_back(cmd);
}

void PaintContext::DrawRoundedRect(const Rect& rect, const Color& color, float radius) {
    DrawCommand cmd{};
    cmd.type = DrawCommandType::Rect;
    cmd.rect = rect;
    cmd.color = color;
    cmd.clipRect = GetCurrentClipRect();
    cmd.borderRadius = radius;
    m_Commands.push_back(cmd);
}

void PaintContext::DrawGradient(const Rect& rect, const Color& topColor, const Color& bottomColor, float radius) {
    DrawCommand cmd{};
    cmd.type = DrawCommandType::Gradient;
    cmd.rect = rect;
    cmd.color = topColor;
    cmd.colorBottom = bottomColor;
    cmd.clipRect = GetCurrentClipRect();
    cmd.borderRadius = radius;
    m_Commands.push_back(cmd);
}

void PaintContext::DrawShadow(const Rect& rect, const Color& color, float radius, float blur) {
    DrawCommand cmd{};
    cmd.type = DrawCommandType::Shadow;
    cmd.rect = rect;
    cmd.color = color;
    cmd.clipRect = GetCurrentClipRect();
    cmd.borderRadius = radius;
    cmd.blur = blur;
    m_Commands.push_back(cmd);
}

void PaintContext::DrawRoundedRectOutline(const Rect& rect, const Color& color, float thickness, float radius) {
    DrawCommand cmd{};
    cmd.type = DrawCommandType::RoundedOutline;
    cmd.rect = rect;
    cmd.color = color;
    cmd.borderRadius = radius;
    cmd.thickness = thickness;
    cmd.clipRect = GetCurrentClipRect();
    m_Commands.push_back(cmd);
}

void PaintContext::DrawText(const std::string& text, const Point& pos, const Color& color, float fontSize, bool bold, bool italic) {
    DrawCommand cmd{};
    cmd.type = DrawCommandType::Text;
    // Store position in rect.x, rect.y
    cmd.rect = { pos.x, pos.y, 0.0f, 0.0f };
    cmd.color = color;
    cmd.clipRect = GetCurrentClipRect();
    cmd.text = text;
    cmd.fontSize = fontSize;
    cmd.textBold = bold;
    cmd.textItalic = italic;
    m_Commands.push_back(cmd);
}

void PaintContext::DrawIcon(const std::string& iconName, const Point& pos, const Color& color, float size) {
    DrawCommand cmd{};
    cmd.type = DrawCommandType::Icon;
    cmd.rect = { pos.x, pos.y, size, size };
    cmd.color = color;
    cmd.clipRect = GetCurrentClipRect();
    cmd.text = iconName;
    cmd.fontSize = size;
    m_Commands.push_back(cmd);
}

float PaintContext::GetTextWidth(const std::string& text, float fontSize) const {
    // Approximate text width (should use font atlas for accurate measurement)
    return static_cast<float>(text.length()) * fontSize * 0.6f;
}

void PaintContext::DrawLine(const Point& start, const Point& end, const Color& color, float thickness) {
    DrawCommand cmd{};
    cmd.type = DrawCommandType::Line;
    cmd.color = color;
    cmd.clipRect = GetCurrentClipRect();
    cmd.lineStart = start;
    cmd.lineEnd = end;
    cmd.thickness = thickness;
    m_Commands.push_back(cmd);
}

void PaintContext::DrawTexture(const Rect& rect, VkDescriptorSet textureId, const Color& tint, const Color& tintBottom) {
    DrawCommand cmd{};
    cmd.type = DrawCommandType::Texture;
    cmd.rect = rect;
    cmd.color = tint;
    cmd.colorBottom = (tintBottom.a > 0.0f) ? tintBottom : tint;
    cmd.clipRect = GetCurrentClipRect();
    cmd.textureId = textureId;
    m_Commands.push_back(cmd);
}

void PaintContext::DrawColorTexture(const Rect& rect, VkDescriptorSet textureId, const Color& tint) {
    DrawCommand cmd{};
    cmd.type = DrawCommandType::ColorTexture;
    cmd.rect = rect;
    cmd.color = tint;
    cmd.colorBottom = tint;
    cmd.clipRect = GetCurrentClipRect();
    cmd.textureId = textureId;
    m_Commands.push_back(cmd);
}

} // namespace we::editor::application::UI
