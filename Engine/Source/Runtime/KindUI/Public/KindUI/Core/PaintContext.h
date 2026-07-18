#pragma once

#include "KindUI/Export.h"
#include "RHI/Types.h"
#include "Text/Layout/TextStyle.h"

#include <vector>
#include <string>
#include "KindUI/Core/Geometry.h"

namespace we::runtime::kindui {

class TextUIService;

enum class DrawCommandType {
    Rect,
    Gradient,
    Shadow,
    RoundedOutline,
    Text,
    Icon,
    Line,
    Texture,
    ColorTexture
};

struct DrawCommand {
    DrawCommandType type;
    Rect rect;
    Color color;
    Color colorBottom;  // For gradients
    Rect clipRect;      // Scissor clipping
    we::rhi::RHIDescriptorSetHandle textureId = we::rhi::RHIDescriptorSetHandle::Invalid;
    std::string text;
    float fontSize = 14.0f;
    uint16_t textWeight = 400; // FontWeight: 400 Regular / 500 Medium / 600 SemiBold
    bool textBold = false;     // legacy; when true, treated as SemiBold (600)
    bool textItalic = false;
    float borderRadius = 0.0f;
    float thickness = 1.0f;
    float blur = 0.0f;  // For shadows
    Point lineStart;
    Point lineEnd;
};

class KINDUI_API PaintContext {
public:
    void SetTextUIService(TextUIService* service) { m_TextService = service; }

    void PushClipRect(const Rect& clip);
    void PopClipRect();

    void DrawRect(const Rect& rect, const Color& color, float borderRadius = 0.0f);
    void DrawRoundedRect(const Rect& rect, const Color& color, float radius);
    void DrawRoundedRectOutline(const Rect& rect, const Color& color, float thickness, float radius);

    void DrawGradient(const Rect& rect, const Color& topColor, const Color& bottomColor, float radius = 0.0f);
    void DrawShadow(const Rect& rect, const Color& color, float radius, float blur);

    void DrawText(const std::string& text, const Point& pos, const Color& color, float fontSize = 14.0f, bool bold = false, bool italic = false);
    void DrawText(
        const std::string& text,
        const Point& pos,
        const Color& color,
        float fontSize,
        we::runtime::text::layout::FontWeight weight,
        bool italic = false);
    void DrawIcon(const std::string& iconName, const Point& pos, const Color& color, float size = 16.0f);
    void DrawIcon(const std::string& iconName, const Rect& rect, const Color& color, float atlasTierPx);
    void DrawLine(const Point& start, const Point& end, const Color& color, float thickness = 1.0f);
    void DrawTexture(const Rect& rect, we::rhi::RHIDescriptorSetHandle textureId, const Color& tint = Color::White(), const Color& tintBottom = Color::Transparent());
    void DrawColorTexture(const Rect& rect, we::rhi::RHIDescriptorSetHandle textureId, const Color& tint = Color::White());

    float GetTextWidth(const std::string& text, float fontSize, bool bold = false, bool italic = false) const;

    const std::vector<DrawCommand>& GetCommands() const { return m_Commands; }
    void Clear() { m_Commands.clear(); m_ClipStack.clear(); }

private:
#pragma warning(push)
#pragma warning(disable: 4251)
    TextUIService* m_TextService = nullptr;
    std::vector<DrawCommand> m_Commands;
    std::vector<Rect> m_ClipStack;
#pragma warning(pop)
    Rect GetCurrentClipRect() const;
};

} // namespace we::runtime::kindui
