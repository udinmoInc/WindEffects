#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Core/TextMetrics.h"
#include "KindUI/Profiling/UiColorDebug.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Rendering/TextUIService.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/ChromeSeparation.h"
#include "KindUI/Tokens/DesignToken.h"

#include <cmath>

namespace we::runtime::kindui {
namespace {

bool ColorsMatch(const Color& a, const Color& b) {
    constexpr float kEps = 0.001f;
    return std::fabs(a.r - b.r) < kEps
        && std::fabs(a.g - b.g) < kEps
        && std::fabs(a.b - b.b) < kEps
        && std::fabs(a.a - b.a) < kEps;
}

bool AllowsChromeOutline(const Color& color) {
    if (!ChromeSeparation::kGapCutsEnabled) {
        return true;
    }
    return ColorsMatch(color, ResolveColor(ColorToken::BorderFocus))
        || ColorsMatch(color, ResolveColor(ColorToken::AccentPrimary))
        || ColorsMatch(color, ResolveColor(ColorToken::BorderError));
}

} // namespace

void PaintContext::PushSurfaceOwner(const char* widgetName, SurfaceRole role) {
    m_SurfaceOwnerStack.push_back(SurfaceOwnerScope{ widgetName, role });
}

void PaintContext::PopSurfaceOwner() {
    if (!m_SurfaceOwnerStack.empty()) {
        m_SurfaceOwnerStack.pop_back();
    }
}

const char* PaintContext::CurrentWidgetName() const {
    return m_SurfaceOwnerStack.empty() ? nullptr : m_SurfaceOwnerStack.back().widgetName;
}

const char* PaintContext::ParentWidgetName() const {
    if (m_SurfaceOwnerStack.size() < 2) {
        return nullptr;
    }
    return m_SurfaceOwnerStack[m_SurfaceOwnerStack.size() - 2].widgetName;
}

void PaintContext::RecordSemanticDraw(
    DrawCommand& cmd,
    SurfaceRole role,
    const char* widgetName,
    bool requiresRole)
{
    cmd.semantic.surfaceRole = role;
    cmd.semantic.widgetName = widgetName ? widgetName : CurrentWidgetName();
    cmd.semantic.parentWidget = ParentWidgetName();
    cmd.semantic.layer = ++m_LayerCounter;
    cmd.semantic.requiresRole = requiresRole;
    if (UiColorDebug::IsSemanticAuditEnabled()) {
        UiColorDebug::Get().TraceSemanticDraw(cmd);
    }
}

void PaintContext::DrawSurface(const Rect& rect, SurfaceRole role, float borderRadius, const char* widgetName) {
    const Color color = ResolveSurfaceColor(role);
    DrawCommand cmd{};
    cmd.type = DrawCommandType::Rect;
    cmd.rect = rect;
    cmd.color = color;
    cmd.clipRect = GetCurrentClipRect();
    cmd.borderRadius = borderRadius;
    RecordSemanticDraw(cmd, role, widgetName, true);
    m_Commands.push_back(cmd);
}

void PaintContext::DrawSurfaceOutline(
    const Rect& rect,
    SurfaceRole role,
    float thickness,
    float radius,
    const char* widgetName)
{
    const Color color = ResolveSurfaceColor(role);
    if (!AllowsChromeOutline(color)) {
        return;
    }
    DrawCommand cmd{};
    cmd.type = DrawCommandType::RoundedOutline;
    cmd.rect = rect;
    cmd.color = color;
    cmd.borderRadius = radius;
    cmd.thickness = thickness;
    cmd.clipRect = GetCurrentClipRect();
    RecordSemanticDraw(cmd, role, widgetName, true);
    m_Commands.push_back(cmd);
}

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
    const bool opaqueFill = color.a > 0.95f && rect.width >= 8.0f && rect.height >= 8.0f;
    if (opaqueFill && UiColorDebug::IsSemanticAuditEnabled()) {
        cmd.semantic.requiresRole = true;
        cmd.semantic.widgetName = CurrentWidgetName();
        cmd.semantic.parentWidget = ParentWidgetName();
        cmd.semantic.layer = ++m_LayerCounter;
        UiColorDebug::Get().TraceUnassignedBackground(cmd);
    }
    m_Commands.push_back(cmd);
}

void PaintContext::DrawRoundedRect(const Rect& rect, const Color& color, float radius) {
    DrawCommand cmd{};
    cmd.type = DrawCommandType::Rect;
    cmd.rect = rect;
    cmd.color = color;
    cmd.clipRect = GetCurrentClipRect();
    cmd.borderRadius = radius;
    const bool opaqueFill = color.a > 0.95f && rect.width >= 8.0f && rect.height >= 8.0f;
    if (opaqueFill && UiColorDebug::IsSemanticAuditEnabled()) {
        cmd.semantic.requiresRole = true;
        cmd.semantic.widgetName = CurrentWidgetName();
        cmd.semantic.parentWidget = ParentWidgetName();
        cmd.semantic.layer = ++m_LayerCounter;
        UiColorDebug::Get().TraceUnassignedBackground(cmd);
    }
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
    if (!AllowsChromeOutline(color)) {
        return;
    }
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
    DrawText(
        text,
        pos,
        color,
        fontSize,
        bold ? we::runtime::text::layout::FontWeight::SemiBold
             : we::runtime::text::layout::FontWeight::Regular,
        italic);
}

void PaintContext::DrawText(
    const std::string& text,
    const Point& pos,
    const Color& color,
    float fontSize,
    we::runtime::text::layout::FontWeight weight,
    bool italic) {
    DrawCommand cmd{};
    cmd.type = DrawCommandType::Text;
    // Store position in rect.x, rect.y
    cmd.rect = { pos.x, pos.y, 0.0f, 0.0f };
    cmd.color = color;
    cmd.clipRect = GetCurrentClipRect();
    cmd.text = text;
    cmd.fontSize = fontSize;
    cmd.textWeight = static_cast<uint16_t>(weight);
    cmd.textBold = weight >= we::runtime::text::layout::FontWeight::SemiBold;
    cmd.textItalic = italic;
    m_Commands.push_back(cmd);
}

void PaintContext::DrawWindIcon(WindIconRef icon, const Rect& rect) {
    if (!icon.IsValid()) {
        return;
    }
    DrawCommand cmd{};
    cmd.type = DrawCommandType::Icon;
    cmd.rect = rect;
    cmd.color = Color::White();
    cmd.clipRect = GetCurrentClipRect();
    cmd.iconStem = icon.stem;
    cmd.iconSizePx = icon.sizePx;
    m_Commands.push_back(cmd);
}

float PaintContext::GetTextWidth(const std::string& text, const float fontSize, const bool bold, const bool italic) const {
    (void)italic;
    if (m_TextService) {
        return m_TextService->MeasureText(text, fontSize, bold);
    }
    return TextMetrics::MeasureWidth(text, fontSize, bold);
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

void PaintContext::DrawTexture(const Rect& rect, we::rhi::RHIDescriptorSetHandle textureId, const Color& tint, const Color& tintBottom) {
    DrawCommand cmd{};
    cmd.type = DrawCommandType::Texture;
    cmd.rect = rect;
    cmd.color = tint;
    cmd.colorBottom = (tintBottom.a > 0.0f) ? tintBottom : tint;
    cmd.clipRect = GetCurrentClipRect();
    cmd.textureId = textureId;
    m_Commands.push_back(cmd);
}

void PaintContext::DrawColorTexture(const Rect& rect, we::rhi::RHIDescriptorSetHandle textureId, const Color& tint) {
    DrawCommand cmd{};
    cmd.type = DrawCommandType::ColorTexture;
    cmd.rect = rect;
    cmd.color = tint;
    cmd.colorBottom = tint;
    cmd.clipRect = GetCurrentClipRect();
    cmd.textureId = textureId;
    m_Commands.push_back(cmd);
}

} // namespace we::runtime::kindui
