#include "Rendering/UIWidgetAdapter.h"
#include "Rendering/FontAtlas.h"
#include "Rendering/IconRenderer.h"
#include "Core/Logger.h"
#include <cmath>
#include <cstring>

namespace we::UI {

UIWidgetAdapter::UIWidgetAdapter()
    : m_Renderer(nullptr)
    , m_Width(0)
    , m_Height(0)
    , m_CurrentTextureSet(VK_NULL_HANDLE)
{
}

UIWidgetAdapter::~UIWidgetAdapter() {
    Shutdown();
}

void UIWidgetAdapter::Initialize(UIRenderer2* renderer) {
    m_Renderer = renderer;
    m_Vertices.clear();
    m_Indices.clear();
    m_Batches.clear();
}

void UIWidgetAdapter::Shutdown() {
    m_Renderer = nullptr;
    m_Vertices.clear();
    m_Indices.clear();
    m_Batches.clear();
}

void UIWidgetAdapter::ProcessWidget(const std::shared_ptr<Widget>& root,
                                     uint32_t width, uint32_t height) {
    if (!root || !m_Renderer) {
        return;
    }
    
    m_Width = width;
    m_Height = height;
    
    // Clear previous frame data
    m_Vertices.clear();
    m_Indices.clear();
    m_Batches.clear();
    
    // Layout and paint widget
    root->Measure(Size{static_cast<float>(width), static_cast<float>(height)});
    root->Arrange(Rect{0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)});
    
    PaintContext paintCtx;
    root->Paint(paintCtx);
    
    // Convert paint commands to geometry
    const auto& commands = paintCtx.GetCommands();
    for (const auto& cmd : commands) {
        ConvertDrawCommand(cmd);
    }
}

void UIWidgetAdapter::ConvertDrawCommand(const DrawCommand& cmd) {
    m_CurrentTextureSet = VK_NULL_HANDLE;
    m_CurrentScissor = {
        static_cast<int32_t>(cmd.clipRect.x),
        static_cast<int32_t>(cmd.clipRect.y),
        static_cast<uint32_t>(cmd.clipRect.width),
        static_cast<uint32_t>(cmd.clipRect.height)
    };
    
    // Clamp scissor to screen bounds
    if (m_CurrentScissor.x < 0) m_CurrentScissor.x = 0;
    if (m_CurrentScissor.y < 0) m_CurrentScissor.y = 0;
    if (m_CurrentScissor.width > m_Width) m_CurrentScissor.width = m_Width;
    if (m_CurrentScissor.height > m_Height) m_CurrentScissor.height = m_Height;
    
    switch (cmd.type) {
        case DrawCommandType::Rect:
            GenerateRectGeometry(cmd);
            break;
        case DrawCommandType::Text:
            GenerateTextGeometry(cmd);
            break;
        case DrawCommandType::Texture:
            GenerateTextureGeometry(cmd);
            break;
        case DrawCommandType::Icon:
            GenerateIconGeometry(cmd);
            break;
        case DrawCommandType::Line:
            GenerateLineGeometry(cmd);
            break;
        case DrawCommandType::Shadow:
            GenerateShadowGeometry(cmd);
            break;
        case DrawCommandType::Gradient:
            GenerateGradientGeometry(cmd);
            break;
        case DrawCommandType::RoundedOutline:
            GenerateRoundedOutlineGeometry(cmd);
            break;
    }
}

void UIWidgetAdapter::GenerateRectGeometry(const DrawCommand& cmd) {
    float x = cmd.rect.x;
    float y = cmd.rect.y;
    float w = cmd.rect.width;
    float h = cmd.rect.height;
    
    // Use SDF rendering for rounded rectangles
    float type = 1.0f; // SDF rounded rect
    
    Color colorTop = cmd.color;
    Color colorBottom = cmd.color;
    
    UIVertex2 v0{ {x,     y},     {0.5f, 0.5f}, {colorTop.r, colorTop.g, colorTop.b, colorTop.a},       {x, y, w, h}, {cmd.borderRadius, type, 0.0f, 0.0f} };
    UIVertex2 v1{ {x + w, y},     {0.5f, 0.5f}, {colorTop.r, colorTop.g, colorTop.b, colorTop.a},       {x, y, w, h}, {cmd.borderRadius, type, 0.0f, 0.0f} };
    UIVertex2 v2{ {x + w, y + h}, {0.5f, 0.5f}, {colorBottom.r, colorBottom.g, colorBottom.b, colorBottom.a}, {x, y, w, h}, {cmd.borderRadius, type, 0.0f, 0.0f} };
    UIVertex2 v3{ {x,     y + h}, {0.5f, 0.5f}, {colorBottom.r, colorBottom.g, colorBottom.b, colorBottom.a}, {x, y, w, h}, {cmd.borderRadius, type, 0.0f, 0.0f} };
    
    uint32_t startIndex = static_cast<uint32_t>(m_Vertices.size());
    m_Vertices.push_back(v0);
    m_Vertices.push_back(v1);
    m_Vertices.push_back(v2);
    m_Vertices.push_back(v3);
    
    m_Indices.push_back(startIndex + 0);
    m_Indices.push_back(startIndex + 1);
    m_Indices.push_back(startIndex + 2);
    m_Indices.push_back(startIndex + 2);
    m_Indices.push_back(startIndex + 3);
    m_Indices.push_back(startIndex + 0);
    
    // Create batch
    UIRenderBatch batch;
    batch.textureSet = m_CurrentTextureSet;
    batch.indexCount = 6;
    batch.firstIndex = static_cast<uint32_t>(m_Indices.size()) - 6;
    batch.vertexOffset = 0;
    batch.scissor[0] = static_cast<float>(m_CurrentScissor.x);
    batch.scissor[1] = static_cast<float>(m_CurrentScissor.y);
    batch.scissor[2] = static_cast<float>(m_CurrentScissor.width);
    batch.scissor[3] = static_cast<float>(m_CurrentScissor.height);
    batch.stencilRef = 0;
    
    m_Batches.push_back(batch);
}

void UIWidgetAdapter::GenerateTextGeometry(const DrawCommand& cmd) {
    if (!m_Renderer || !m_Renderer->GetFontAtlas()) {
        return;
    }
    
    FontAtlas* fontAtlas = m_Renderer->GetFontAtlas();
    m_CurrentTextureSet = fontAtlas->GetDescriptorSet();
    
    float xpos = cmd.rect.x;
    float ypos = cmd.rect.y;
    
    float scale = 1.0f;
    if (fontAtlas->GetFontHeight() > 0.0f) {
        scale = cmd.fontSize / fontAtlas->GetFontHeight();
    }
    float logicalX = 0.0f;
    float logicalY = 0.0f;
    float startX = xpos;
    float startY = ypos + cmd.fontSize * 0.85f;
    
    for (char c : cmd.text) {
        GlyphInfo q;
        if (fontAtlas->GetCharQuad(c, &logicalX, &logicalY, q)) {
            uint32_t charStart = static_cast<uint32_t>(m_Vertices.size());
            
            float px0 = startX + q.x0 * scale;
            float py0 = startY + q.y0 * scale;
            float px1 = startX + q.x1 * scale;
            float py1 = startY + q.y1 * scale;
            float w = px1 - px0;
            float h = py1 - py0;
            
            float type = 0.0f; // Textured
            
            UIVertex2 v0{ {px0, py0}, {q.u0, q.v0}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {px0, py0, w, h}, {0.0f, type, 0.0f, 0.0f} };
            UIVertex2 v1{ {px1, py0}, {q.u1, q.v0}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {px0, py0, w, h}, {0.0f, type, 0.0f, 0.0f} };
            UIVertex2 v2{ {px1, py1}, {q.u1, q.v1}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {px0, py0, w, h}, {0.0f, type, 0.0f, 0.0f} };
            UIVertex2 v3{ {px0, py1}, {q.u0, q.v1}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {px0, py0, w, h}, {0.0f, type, 0.0f, 0.0f} };
            
            m_Vertices.push_back(v0);
            m_Vertices.push_back(v1);
            m_Vertices.push_back(v2);
            m_Vertices.push_back(v3);
            
            m_Indices.push_back(charStart + 0);
            m_Indices.push_back(charStart + 1);
            m_Indices.push_back(charStart + 2);
            m_Indices.push_back(charStart + 2);
            m_Indices.push_back(charStart + 3);
            m_Indices.push_back(charStart + 0);
        }
    }
    
    // Create batch for all text
    UIRenderBatch batch;
    batch.textureSet = m_CurrentTextureSet;
    batch.indexCount = static_cast<uint32_t>(m_Indices.size()); // All indices added in this call
    batch.firstIndex = 0; // Would need proper tracking
    batch.vertexOffset = 0;
    batch.scissor[0] = static_cast<float>(m_CurrentScissor.x);
    batch.scissor[1] = static_cast<float>(m_CurrentScissor.y);
    batch.scissor[2] = static_cast<float>(m_CurrentScissor.width);
    batch.scissor[3] = static_cast<float>(m_CurrentScissor.height);
    batch.stencilRef = 0;
    
    m_Batches.push_back(batch);
}

void UIWidgetAdapter::GenerateTextureGeometry(const DrawCommand& cmd) {
    float x = cmd.rect.x;
    float y = cmd.rect.y;
    float w = cmd.rect.width;
    float h = cmd.rect.height;
    
    m_CurrentTextureSet = cmd.textureId;
    
    float type = 0.0f; // Textured
    
    Color colorTop = cmd.color;
    Color colorBottom = cmd.colorBottom;
    
    UIVertex2 v0{ {x,     y},     {0.0f, 0.0f}, {colorTop.r, colorTop.g, colorTop.b, colorTop.a},       {x, y, w, h}, {0.0f, type, 0.0f, 0.0f} };
    UIVertex2 v1{ {x + w, y},     {1.0f, 0.0f}, {colorTop.r, colorTop.g, colorTop.b, colorTop.a},       {x, y, w, h}, {0.0f, type, 0.0f, 0.0f} };
    UIVertex2 v2{ {x + w, y + h}, {1.0f, 1.0f}, {colorBottom.r, colorBottom.g, colorBottom.b, colorBottom.a}, {x, y, w, h}, {0.0f, type, 0.0f, 0.0f} };
    UIVertex2 v3{ {x,     y + h}, {0.0f, 1.0f}, {colorBottom.r, colorBottom.g, colorBottom.b, colorBottom.a}, {x, y, w, h}, {0.0f, type, 0.0f, 0.0f} };
    
    uint32_t startIndex = static_cast<uint32_t>(m_Vertices.size());
    m_Vertices.push_back(v0);
    m_Vertices.push_back(v1);
    m_Vertices.push_back(v2);
    m_Vertices.push_back(v3);
    
    m_Indices.push_back(startIndex + 0);
    m_Indices.push_back(startIndex + 1);
    m_Indices.push_back(startIndex + 2);
    m_Indices.push_back(startIndex + 2);
    m_Indices.push_back(startIndex + 3);
    m_Indices.push_back(startIndex + 0);
    
    UIRenderBatch batch;
    batch.textureSet = m_CurrentTextureSet;
    batch.indexCount = 6;
    batch.firstIndex = static_cast<uint32_t>(m_Indices.size()) - 6;
    batch.vertexOffset = 0;
    batch.scissor[0] = static_cast<float>(m_CurrentScissor.x);
    batch.scissor[1] = static_cast<float>(m_CurrentScissor.y);
    batch.scissor[2] = static_cast<float>(m_CurrentScissor.width);
    batch.scissor[3] = static_cast<float>(m_CurrentScissor.height);
    batch.stencilRef = 0;
    
    m_Batches.push_back(batch);
}

void UIWidgetAdapter::GenerateIconGeometry(const DrawCommand& cmd) {
    if (!m_Renderer || !m_Renderer->GetIconRenderer()) {
        return;
    }
    
    IconRenderer* iconRenderer = m_Renderer->GetIconRenderer();
    uint32_t iconSize = static_cast<uint32_t>(std::max(8.0f, cmd.fontSize));
    m_CurrentTextureSet = iconRenderer->GetLucideIcon(cmd.text, iconSize, cmd.color);
    
    if (m_CurrentTextureSet == VK_NULL_HANDLE) {
        return;
    }
    
    // Generate quad for icon
    float x = cmd.rect.x;
    float y = cmd.rect.y;
    float w = cmd.rect.width;
    float h = cmd.rect.height;
    
    float type = 0.0f; // Textured
    Color color = Color::White();
    
    UIVertex2 v0{ {x,     y},     {0.0f, 0.0f}, {color.r, color.g, color.b, color.a}, {x, y, w, h}, {0.0f, type, 0.0f, 0.0f} };
    UIVertex2 v1{ {x + w, y},     {1.0f, 0.0f}, {color.r, color.g, color.b, color.a}, {x, y, w, h}, {0.0f, type, 0.0f, 0.0f} };
    UIVertex2 v2{ {x + w, y + h}, {1.0f, 1.0f}, {color.r, color.g, color.b, color.a}, {x, y, w, h}, {0.0f, type, 0.0f, 0.0f} };
    UIVertex2 v3{ {x,     y + h}, {0.0f, 1.0f}, {color.r, color.g, color.b, color.a}, {x, y, w, h}, {0.0f, type, 0.0f, 0.0f} };
    
    uint32_t startIndex = static_cast<uint32_t>(m_Vertices.size());
    m_Vertices.push_back(v0);
    m_Vertices.push_back(v1);
    m_Vertices.push_back(v2);
    m_Vertices.push_back(v3);
    
    m_Indices.push_back(startIndex + 0);
    m_Indices.push_back(startIndex + 1);
    m_Indices.push_back(startIndex + 2);
    m_Indices.push_back(startIndex + 2);
    m_Indices.push_back(startIndex + 3);
    m_Indices.push_back(startIndex + 0);
    
    UIRenderBatch batch;
    batch.textureSet = m_CurrentTextureSet;
    batch.indexCount = 6;
    batch.firstIndex = static_cast<uint32_t>(m_Indices.size()) - 6;
    batch.vertexOffset = 0;
    batch.scissor[0] = static_cast<float>(m_CurrentScissor.x);
    batch.scissor[1] = static_cast<float>(m_CurrentScissor.y);
    batch.scissor[2] = static_cast<float>(m_CurrentScissor.width);
    batch.scissor[3] = static_cast<float>(m_CurrentScissor.height);
    batch.stencilRef = 0;
    
    m_Batches.push_back(batch);
}

void UIWidgetAdapter::GenerateLineGeometry(const DrawCommand& cmd) {
    float dx = cmd.lineEnd.x - cmd.lineStart.x;
    float dy = cmd.lineEnd.y - cmd.lineStart.y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len > 0.0f) {
        dx /= len;
        dy /= len;
        float px = -dy * (cmd.thickness * 0.5f);
        float py = dx * (cmd.thickness * 0.5f);
        
        float sx = cmd.lineStart.x;
        float sy = cmd.lineStart.y;
        float sw = dx * len;
        float sh = dy * len;
        
        float type = 0.0f; // Textured (using dummy)
        
        UIVertex2 v0{ {cmd.lineStart.x + px, cmd.lineStart.y + py}, {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {sx, sy, sw, sh}, {0.0f, type, 0.0f, 0.0f} };
        UIVertex2 v1{ {cmd.lineStart.x - px, cmd.lineStart.y - py}, {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {sx, sy, sw, sh}, {0.0f, type, 0.0f, 0.0f} };
        UIVertex2 v2{ {cmd.lineEnd.x - px,   cmd.lineEnd.y - py},   {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {sx, sy, sw, sh}, {0.0f, type, 0.0f, 0.0f} };
        UIVertex2 v3{ {cmd.lineEnd.x + px,   cmd.lineEnd.y + py},   {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {sx, sy, sw, sh}, {0.0f, type, 0.0f, 0.0f} };
        
        uint32_t startIndex = static_cast<uint32_t>(m_Vertices.size());
        m_Vertices.push_back(v0);
        m_Vertices.push_back(v1);
        m_Vertices.push_back(v2);
        m_Vertices.push_back(v3);
        
        m_Indices.push_back(startIndex + 0);
        m_Indices.push_back(startIndex + 1);
        m_Indices.push_back(startIndex + 2);
        m_Indices.push_back(startIndex + 2);
        m_Indices.push_back(startIndex + 3);
        m_Indices.push_back(startIndex + 0);
        
        UIRenderBatch batch;
        batch.textureSet = m_CurrentTextureSet;
        batch.indexCount = 6;
        batch.firstIndex = static_cast<uint32_t>(m_Indices.size()) - 6;
        batch.vertexOffset = 0;
        batch.scissor[0] = static_cast<float>(m_CurrentScissor.x);
        batch.scissor[1] = static_cast<float>(m_CurrentScissor.y);
        batch.scissor[2] = static_cast<float>(m_CurrentScissor.width);
        batch.scissor[3] = static_cast<float>(m_CurrentScissor.height);
        batch.stencilRef = 0;
        
        m_Batches.push_back(batch);
    }
}

void UIWidgetAdapter::GenerateShadowGeometry(const DrawCommand& cmd) {
    // Approximate soft shadow using multiple expanded semi-transparent rects
    const int numLayers = 4;
    float shadowSpread = cmd.blur / numLayers;
    float baseAlpha = cmd.color.a / (numLayers * 1.5f);
    
    float x = cmd.rect.x;
    float y = cmd.rect.y;
    float w = cmd.rect.width;
    float h = cmd.rect.height;
    
    for (int i = 0; i < numLayers; ++i) {
        float expand = (i + 1) * shadowSpread;
        float sx = x - expand;
        float sy = y - expand;
        float sw = w + expand * 2.0f;
        float sh = h + expand * 2.0f;
        float alpha = baseAlpha * (1.0f - (float)i / numLayers);
        
        float type = 1.0f; // SDF Rect
        float r = cmd.borderRadius + expand;
        
        UIVertex2 v0{ {sx,      sy},      {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, alpha}, {sx, sy, sw, sh}, {r, type, 0.0f, 0.0f} };
        UIVertex2 v1{ {sx + sw, sy},      {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, alpha}, {sx, sy, sw, sh}, {r, type, 0.0f, 0.0f} };
        UIVertex2 v2{ {sx + sw, sy + sh}, {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, alpha}, {sx, sy, sw, sh}, {r, type, 0.0f, 0.0f} };
        UIVertex2 v3{ {sx,      sy + sh}, {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, alpha}, {sx, sy, sw, sh}, {r, type, 0.0f, 0.0f} };
        
        uint32_t startIndex = static_cast<uint32_t>(m_Vertices.size());
        m_Vertices.push_back(v0);
        m_Vertices.push_back(v1);
        m_Vertices.push_back(v2);
        m_Vertices.push_back(v3);
        
        m_Indices.push_back(startIndex + 0);
        m_Indices.push_back(startIndex + 1);
        m_Indices.push_back(startIndex + 2);
        m_Indices.push_back(startIndex + 2);
        m_Indices.push_back(startIndex + 3);
        m_Indices.push_back(startIndex + 0);
    }
    
    UIRenderBatch batch;
    batch.textureSet = m_CurrentTextureSet;
    batch.indexCount = numLayers * 6;
    batch.firstIndex = static_cast<uint32_t>(m_Indices.size()) - (numLayers * 6);
    batch.vertexOffset = 0;
    batch.scissor[0] = static_cast<float>(m_CurrentScissor.x);
    batch.scissor[1] = static_cast<float>(m_CurrentScissor.y);
    batch.scissor[2] = static_cast<float>(m_CurrentScissor.width);
    batch.scissor[3] = static_cast<float>(m_CurrentScissor.height);
    batch.stencilRef = 0;
    
    m_Batches.push_back(batch);
}

void UIWidgetAdapter::GenerateGradientGeometry(const DrawCommand& cmd) {
    float x = cmd.rect.x;
    float y = cmd.rect.y;
    float w = cmd.rect.width;
    float h = cmd.rect.height;
    
    float type = 1.0f; // SDF rounded rect for gradient support
    
    Color colorTop = cmd.color;
    Color colorBottom = cmd.colorBottom;
    
    UIVertex2 v0{ {x,     y},     {0.5f, 0.5f}, {colorTop.r, colorTop.g, colorTop.b, colorTop.a},       {x, y, w, h}, {cmd.borderRadius, type, 0.0f, 0.0f} };
    UIVertex2 v1{ {x + w, y},     {0.5f, 0.5f}, {colorTop.r, colorTop.g, colorTop.b, colorTop.a},       {x, y, w, h}, {cmd.borderRadius, type, 0.0f, 0.0f} };
    UIVertex2 v2{ {x + w, y + h}, {0.5f, 0.5f}, {colorBottom.r, colorBottom.g, colorBottom.b, colorBottom.a}, {x, y, w, h}, {cmd.borderRadius, type, 0.0f, 0.0f} };
    UIVertex2 v3{ {x,     y + h}, {0.5f, 0.5f}, {colorBottom.r, colorBottom.g, colorBottom.b, colorBottom.a}, {x, y, w, h}, {cmd.borderRadius, type, 0.0f, 0.0f} };
    
    uint32_t startIndex = static_cast<uint32_t>(m_Vertices.size());
    m_Vertices.push_back(v0);
    m_Vertices.push_back(v1);
    m_Vertices.push_back(v2);
    m_Vertices.push_back(v3);
    
    m_Indices.push_back(startIndex + 0);
    m_Indices.push_back(startIndex + 1);
    m_Indices.push_back(startIndex + 2);
    m_Indices.push_back(startIndex + 2);
    m_Indices.push_back(startIndex + 3);
    m_Indices.push_back(startIndex + 0);
    
    UIRenderBatch batch;
    batch.textureSet = m_CurrentTextureSet;
    batch.indexCount = 6;
    batch.firstIndex = static_cast<uint32_t>(m_Indices.size()) - 6;
    batch.vertexOffset = 0;
    batch.scissor[0] = static_cast<float>(m_CurrentScissor.x);
    batch.scissor[1] = static_cast<float>(m_CurrentScissor.y);
    batch.scissor[2] = static_cast<float>(m_CurrentScissor.width);
    batch.scissor[3] = static_cast<float>(m_CurrentScissor.height);
    batch.stencilRef = 0;
    
    m_Batches.push_back(batch);
}

void UIWidgetAdapter::GenerateRoundedOutlineGeometry(const DrawCommand& cmd) {
    float x = cmd.rect.x;
    float y = cmd.rect.y;
    float w = cmd.rect.width;
    float h = cmd.rect.height;
    
    float type = 2.0f; // SDF rounded outline
    float thickness = cmd.thickness;
    
    UIVertex2 v0{ {x,     y},     {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {x, y, w, h}, {cmd.borderRadius, type, thickness, 0.0f} };
    UIVertex2 v1{ {x + w, y},     {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {x, y, w, h}, {cmd.borderRadius, type, thickness, 0.0f} };
    UIVertex2 v2{ {x + w, y + h}, {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {x, y, w, h}, {cmd.borderRadius, type, thickness, 0.0f} };
    UIVertex2 v3{ {x,     y + h}, {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {x, y, w, h}, {cmd.borderRadius, type, thickness, 0.0f} };
    
    uint32_t startIndex = static_cast<uint32_t>(m_Vertices.size());
    m_Vertices.push_back(v0);
    m_Vertices.push_back(v1);
    m_Vertices.push_back(v2);
    m_Vertices.push_back(v3);
    
    m_Indices.push_back(startIndex + 0);
    m_Indices.push_back(startIndex + 1);
    m_Indices.push_back(startIndex + 2);
    m_Indices.push_back(startIndex + 2);
    m_Indices.push_back(startIndex + 3);
    m_Indices.push_back(startIndex + 0);
    
    UIRenderBatch batch;
    batch.textureSet = m_CurrentTextureSet;
    batch.indexCount = 6;
    batch.firstIndex = static_cast<uint32_t>(m_Indices.size()) - 6;
    batch.vertexOffset = 0;
    batch.scissor[0] = static_cast<float>(m_CurrentScissor.x);
    batch.scissor[1] = static_cast<float>(m_CurrentScissor.y);
    batch.scissor[2] = static_cast<float>(m_CurrentScissor.width);
    batch.scissor[3] = static_cast<float>(m_CurrentScissor.height);
    batch.stencilRef = 0;
    
    m_Batches.push_back(batch);
}

} // namespace we::UI
