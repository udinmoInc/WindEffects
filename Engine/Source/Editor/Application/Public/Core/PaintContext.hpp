#pragma once

#include <vector>
#include <string>
#include "Core/Geometry.hpp"
#include <volk.h>

namespace we::UI {

enum class DrawCommandType {
    Rect,
    Gradient,
    Shadow,
    Text,
    Icon,
    Line,
    Texture
};

struct DrawCommand {
    DrawCommandType type;
    Rect rect;
    Color color;
    Color colorBottom;  // For gradients
    Rect clipRect;      // Scissor clipping
    VkDescriptorSet textureId = VK_NULL_HANDLE; // Used for viewport or icons
    std::string text;
    int codepoint = 0;
    float fontSize = 14.0f;
    float borderRadius = 0.0f;
    float thickness = 1.0f;
    float blur = 0.0f;  // For shadows
    Point lineStart;
    Point lineEnd;
};

class PaintContext {
public:
    void PushClipRect(const Rect& clip);
    void PopClipRect();

    void DrawRect(const Rect& rect, const Color& color, float borderRadius = 0.0f);
    void DrawRoundedRect(const Rect& rect, const Color& color, float radius);
    void DrawRoundedRectOutline(const Rect& rect, const Color& color, float thickness, float radius);
    
    // New AAA styling
    void DrawGradient(const Rect& rect, const Color& topColor, const Color& bottomColor, float radius = 0.0f);
    void DrawShadow(const Rect& rect, const Color& color, float radius, float blur);
    
    void DrawText(const std::string& text, const Point& pos, const Color& color, float fontSize = 14.0f, bool bold = false, bool italic = false);
    void DrawIcon(int codepoint, const Point& pos, const Color& color, float fontSize = 16.0f);
    void DrawLine(const Point& start, const Point& end, const Color& color, float thickness = 1.0f);
    void DrawTexture(const Rect& rect, VkDescriptorSet textureId, const Color& tint = Color::White());
    
    float GetTextWidth(const std::string& text, float fontSize) const;

    const std::vector<DrawCommand>& GetCommands() const { return m_Commands; }
    void Clear() { m_Commands.clear(); m_ClipStack.clear(); }

private:
    std::vector<DrawCommand> m_Commands;
    std::vector<Rect> m_ClipStack;
    Rect GetCurrentClipRect() const;
};

} // namespace we::editor::application::UI
