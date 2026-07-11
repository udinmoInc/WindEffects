#include "Rendering/UIWidgetAdapter.h"
#include "Rendering/IconRenderer.h"
#include "Rendering/TextUIService.h"
#include "Core/Logger.h"
#include "Core/FrameCounter.h"
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <functional>

namespace WindEffects::Editor::UI {

namespace {
inline float SnapPx(float v) {
    // Snap to nearest pixel in UI space (pixel-center mapping happens in the vertex shader).
    return std::floor(v + 0.5f);
}

bool IsEnvEnabled(const char* name) {
    const char* v = std::getenv(name);
    return v != nullptr && v[0] != '\0' && v[0] != '0';
}
} // namespace

UIWidgetAdapter::UIWidgetAdapter()
    : m_Renderer(nullptr)
    , m_Width(0)
    , m_Height(0)
    , m_CurrentTextureSet(VK_NULL_HANDLE)
    , m_DefaultTextureSet(VK_NULL_HANDLE)
{
}

UIWidgetAdapter::~UIWidgetAdapter() {
    Shutdown();
}

void UIWidgetAdapter::Initialize(OverlayRenderer* renderer) {
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
    m_DefaultTextureSet = m_Renderer->GetDummyDescriptorSet();
    
    // Clear previous frame data
    m_Vertices.clear();
    m_Indices.clear();
    m_Batches.clear();
    
    // Helper lambda to count widgets
    std::function<void(const std::shared_ptr<Widget>&)> CountWidgets = [&](const std::shared_ptr<Widget>& w) {
        if (!w) return;
        Widget::s_TotalWidgetCount++;
        if (w->IsVisible()) Widget::s_VisibleWidgetCount++;
        else Widget::s_HiddenWidgetCount++;
        for (const auto& child : w->GetChildren()) {
            CountWidgets(child);
        }
    };
    CountWidgets(root);

    // Layout and paint widget
    Widget::s_LayoutPassCount++;
    root->Measure(Size{static_cast<float>(width), static_cast<float>(height)});
    root->Arrange(Rect{0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)});
    
    PaintContext paintCtx;
    if (TextUIService* textService = m_Renderer->GetTextUIService()) {
        paintCtx.SetTextUIService(textService);
    }
    Widget::s_PaintCalls++;
    root->Paint(paintCtx);
    m_Diagnostics.paintCommandsRecorded = static_cast<uint32_t>(paintCtx.GetCommands().size());
    
    // Convert paint commands to geometry
    const auto& commands = paintCtx.GetCommands();
    for (const auto& cmd : commands) {
        ConvertDrawCommand(cmd);
    }
}

void UIWidgetAdapter::ConvertDrawCommand(const DrawCommand& cmd) {
    const bool textOnly = IsEnvEnabled("WE_UI_AB_TEXT_ONLY");
    const bool noText = IsEnvEnabled("WE_UI_AB_NO_TEXT");
    if (textOnly && cmd.type != DrawCommandType::Text) {
        return;
    }
    if (noText && cmd.type == DrawCommandType::Text) {
        return;
    }

    m_CurrentTextureSet = m_DefaultTextureSet;
    m_CurrentScissor = {
        static_cast<int32_t>(cmd.clipRect.x),
        static_cast<int32_t>(cmd.clipRect.y),
        static_cast<uint32_t>(cmd.clipRect.width),
        static_cast<uint32_t>(cmd.clipRect.height)
    };
    
    // Clamp scissor to screen bounds correctly for Vulkan
    if (m_CurrentScissor.x < 0) {
        m_CurrentScissor.width = (m_CurrentScissor.x + m_CurrentScissor.width > 0) ? (m_CurrentScissor.width + m_CurrentScissor.x) : 0;
        m_CurrentScissor.x = 0;
    }
    if (m_CurrentScissor.y < 0) {
        m_CurrentScissor.height = (m_CurrentScissor.y + m_CurrentScissor.height > 0) ? (m_CurrentScissor.height + m_CurrentScissor.y) : 0;
        m_CurrentScissor.y = 0;
    }
    
    // Ensure extent doesn't exceed screen dimensions when added to offset
    if (static_cast<uint32_t>(m_CurrentScissor.x) + m_CurrentScissor.width > m_Width) {
        m_CurrentScissor.width = (m_Width > static_cast<uint32_t>(m_CurrentScissor.x)) ? (m_Width - static_cast<uint32_t>(m_CurrentScissor.x)) : 0;
    }
    if (static_cast<uint32_t>(m_CurrentScissor.y) + m_CurrentScissor.height > m_Height) {
        m_CurrentScissor.height = (m_Height > static_cast<uint32_t>(m_CurrentScissor.y)) ? (m_Height - static_cast<uint32_t>(m_CurrentScissor.y)) : 0;
    }
    
    m_Diagnostics.totalDrawCommandsGenerated++;

    if (cmd.clipRect.width == 0 || cmd.clipRect.height == 0) {
        HE_INFO(std::string("Skipped: Zero Size | Type: ") + std::to_string(static_cast<int>(cmd.type)));
        return;
    }
    if (cmd.color.a == 0.0f) {
        HE_INFO(std::string("Skipped: Zero Alpha | Type: ") + std::to_string(static_cast<int>(cmd.type)));
        // Might still want to process if it's a clipping rect or something, but usually skipped
    }

    switch (cmd.type) {
        case DrawCommandType::Rect:
            m_Diagnostics.rectangleCommands++;
            GenerateRectGeometry(cmd);
            break;
        case DrawCommandType::Text:
            m_Diagnostics.textCommands++;
            GenerateTextGeometry(cmd);
            break;
        case DrawCommandType::Texture:
            m_Diagnostics.imageCommands++;
            GenerateTextureGeometry(cmd);
            break;
        case DrawCommandType::ColorTexture:
            m_Diagnostics.imageCommands++;
            GenerateColorTextureGeometry(cmd);
            break;
        case DrawCommandType::Icon:
            m_Diagnostics.iconCommands++;
            GenerateIconGeometry(cmd);
            break;
        case DrawCommandType::Line:
            m_Diagnostics.borderCommands++;
            GenerateLineGeometry(cmd);
            break;
        case DrawCommandType::Shadow:
            m_Diagnostics.shadowCommands++;
            GenerateShadowGeometry(cmd);
            break;
        case DrawCommandType::Gradient:
            m_Diagnostics.gradientCommands++;
            GenerateGradientGeometry(cmd);
            break;
        case DrawCommandType::RoundedOutline:
            m_Diagnostics.borderCommands++;
            GenerateRoundedOutlineGeometry(cmd);
            break;

    }
}

void UIWidgetAdapter::GenerateRectGeometry(const DrawCommand& cmd) {
    // Pixel-snap UI chrome so 1px borders and edges stay crisp.
    float x0 = SnapPx(cmd.rect.x);
    float y0 = SnapPx(cmd.rect.y);
    float x1 = SnapPx(cmd.rect.x + cmd.rect.width);
    float y1 = SnapPx(cmd.rect.y + cmd.rect.height);
    float x = x0;
    float y = y0;
    float w = x1 - x0;
    float h = y1 - y0;
    
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
    if (!m_Renderer) {
        ++m_Diagnostics.textDrawsSkipped;
        return;
    }

    TextUIService* textService = m_Renderer->GetTextUIService();
    if (!textService) {
        ++m_Diagnostics.textDrawsSkipped;
        return;
    }

    ++m_Diagnostics.textStringsProcessed;
    VkDescriptorSet fontDescriptor = VK_NULL_HANDLE;
    const uint32_t startTotalIndex = static_cast<uint32_t>(m_Indices.size());
    UIRenderBatch batchInfo;
    if (!textService->GenerateTextGeometry(cmd, m_Vertices, m_Indices, fontDescriptor, &batchInfo)) {
        ++m_Diagnostics.textDrawsSkipped;
        return;
    }

    m_CurrentTextureSet = fontDescriptor;
    m_Diagnostics.textGlyphsResolved += static_cast<uint32_t>((m_Indices.size() - startTotalIndex) / 6);
    m_Diagnostics.textVerticesGenerated += static_cast<uint32_t>(m_Vertices.size());
    m_Diagnostics.textIndicesGenerated += static_cast<uint32_t>(m_Indices.size());
    
    UIRenderBatch batch;
    batch.textureSet = m_CurrentTextureSet;
    batch.indexCount = static_cast<uint32_t>(m_Indices.size()) - startTotalIndex;
    batch.firstIndex = startTotalIndex;
    batch.vertexOffset = 0;
    batch.scissor[0] = static_cast<float>(m_CurrentScissor.x);
    batch.scissor[1] = static_cast<float>(m_CurrentScissor.y);
    batch.scissor[2] = static_cast<float>(m_CurrentScissor.width);
    batch.scissor[3] = static_cast<float>(m_CurrentScissor.height);
    batch.stencilRef = 0;
    batch.isText = batchInfo.isText;
    batch.atlasWidth = batchInfo.atlasWidth;
    batch.atlasHeight = batchInfo.atlasHeight;
    batch.msdfPixelRange = batchInfo.msdfPixelRange;
    
    m_Batches.push_back(batch);
    ++m_Diagnostics.textBatchesCreated;
}

void UIWidgetAdapter::GenerateTextureGeometry(const DrawCommand& cmd) {
    // Pixel-snap textured UI elements (thumbnails, bitmaps) to avoid subpixel sampling blur.
    float x0 = SnapPx(cmd.rect.x);
    float y0 = SnapPx(cmd.rect.y);
    float x1 = SnapPx(cmd.rect.x + cmd.rect.width);
    float y1 = SnapPx(cmd.rect.y + cmd.rect.height);
    float x = x0;
    float y = y0;
    float w = x1 - x0;
    float h = y1 - y0;
    
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

void UIWidgetAdapter::GenerateColorTextureGeometry(const DrawCommand& cmd) {
    float x0 = SnapPx(cmd.rect.x);
    float y0 = SnapPx(cmd.rect.y);
    float x1 = SnapPx(cmd.rect.x + cmd.rect.width);
    float y1 = SnapPx(cmd.rect.y + cmd.rect.height);
    float x = x0;
    float y = y0;
    float w = x1 - x0;
    float h = y1 - y0;

    m_CurrentTextureSet = cmd.textureId;

    float type = 4.0f; // Full-color texture (viewport render targets)

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
    // Pixel-snap icon quads; icon bitmaps are authored/rasterized for exact pixel sizes.
    float x0 = SnapPx(cmd.rect.x);
    float y0 = SnapPx(cmd.rect.y);
    float x1 = SnapPx(cmd.rect.x + cmd.rect.width);
    float y1 = SnapPx(cmd.rect.y + cmd.rect.height);
    float x = x0;
    float y = y0;
    float w = x1 - x0;
    float h = y1 - y0;
    
    float type = 0.0f; // Textured
    Color color = cmd.color;
    
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
    // Pixel-snap line endpoints; 1px lines blur heavily when placed at half pixels.
    Point s{SnapPx(cmd.lineStart.x), SnapPx(cmd.lineStart.y)};
    Point e{SnapPx(cmd.lineEnd.x), SnapPx(cmd.lineEnd.y)};
    float dx = e.x - s.x;
    float dy = e.y - s.y;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len > 0.0f) {
        dx /= len;
        dy /= len;
        float px = -dy * (cmd.thickness * 0.5f);
        float py = dx * (cmd.thickness * 0.5f);
        
        float sx = s.x;
        float sy = s.y;
        float sw = dx * len;
        float sh = dy * len;
        
        float type = 0.0f; // Textured (using dummy)
        
        UIVertex2 v0{ {s.x + px, s.y + py}, {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {sx, sy, sw, sh}, {0.0f, type, 0.0f, 0.0f} };
        UIVertex2 v1{ {s.x - px, s.y - py}, {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {sx, sy, sw, sh}, {0.0f, type, 0.0f, 0.0f} };
        UIVertex2 v2{ {e.x - px, e.y - py}, {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {sx, sy, sw, sh}, {0.0f, type, 0.0f, 0.0f} };
        UIVertex2 v3{ {e.x + px, e.y + py}, {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {sx, sy, sw, sh}, {0.0f, type, 0.0f, 0.0f} };
        
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
        
        // Clamp shadow coordinates to viewport bounds to prevent off-screen rendering
        if (sx < 0.0f) {
            sw += sx; // Reduce width by the amount we're clamping
            sx = 0.0f;
        }
        if (sy < 0.0f) {
            sh += sy; // Reduce height by the amount we're clamping
            sy = 0.0f;
        }
        
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
    float x0 = SnapPx(cmd.rect.x);
    float y0 = SnapPx(cmd.rect.y);
    float x1 = SnapPx(cmd.rect.x + cmd.rect.width);
    float y1 = SnapPx(cmd.rect.y + cmd.rect.height);
    float x = x0;
    float y = y0;
    float w = x1 - x0;
    float h = y1 - y0;
    
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
    float x0 = SnapPx(cmd.rect.x);
    float y0 = SnapPx(cmd.rect.y);
    float x1 = SnapPx(cmd.rect.x + cmd.rect.width);
    float y1 = SnapPx(cmd.rect.y + cmd.rect.height);
    float x = x0;
    float y = y0;
    float w = x1 - x0;
    float h = y1 - y0;
    
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

} // namespace WindEffects::Editor::UI
