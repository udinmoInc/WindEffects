#include "KindUI/Rendering/UIWidgetAdapter.h"
#include "KindUI/Profiling/UiPathDiagnostics.h"
#include "KindUI/Core/ColorSpace.h"
#include "KindUI/Profiling/UiColorDebug.h"
#include "KindUI/Profiling/UiColorPipelineDiagnostic.h"
#include "KindUI/Profiling/UiColorCompositionDiagnostic.h"
#include "KindUI/Tokens/SurfaceRole.h"
#include "KindUI/Rendering/IconRenderer.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Core/WindIcon.h"
#include "KindUI/Rendering/TextUIService.h"
#include "Core/Logger.h"
#include "Core/FrameCounter.h"
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <functional>
#include <sstream>

namespace we::runtime::kindui {

namespace {
inline float SnapPx(float v) {
    // Snap to nearest pixel in UI space (pixel-center mapping happens in the vertex shader).
    return std::floor(v + 0.5f);
}

bool IsEnvEnabled(const char* name) {
    const char* v = std::getenv(name);
    return v != nullptr && v[0] != '\0' && v[0] != '0';
}

void DumpWidgetLayoutTree(const std::shared_ptr<Widget>& widget, int depth, int& count) {
    if (!widget || count > 200) {
        return;
    }
    ++count;
    const Rect& g = widget->GetGeometry();
    const Size& d = widget->GetDesiredSize();
    std::ostringstream line;
    line << "[KindUI Layout] ";
    for (int i = 0; i < depth; ++i) {
        line << "  ";
    }
    line << (widget->GetId().empty() ? "?" : widget->GetId())
         << " desired=(" << d.width << "x" << d.height << ")"
         << " final=(" << g.x << "," << g.y << " " << g.width << "x" << g.height << ")"
         << " grow=" << widget->GetFlexGrow()
         << " visible=" << (widget->IsVisible() ? 1 : 0)
         << " children=" << widget->GetChildren().size();
    HE_INFO(line.str());
    for (const auto& child : widget->GetChildren()) {
        DumpWidgetLayoutTree(child, depth + 1, count);
    }
}
} // namespace

UIWidgetAdapter::UIWidgetAdapter()
    : m_Renderer(nullptr)
    , m_Width(0)
    , m_Height(0)
    , m_CurrentTextureSet(we::rhi::RHIDescriptorSetHandle::Invalid)
    , m_DefaultTextureSet(we::rhi::RHIDescriptorSetHandle::Invalid)
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
                                     uint32_t width, uint32_t height,
                                     bool runLayout) {
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

    if (UiColorPipelineDiagnostic::IsEnabled()
        && !UiColorCompositionDiagnostic::IsEnabled()) {
        m_CurrentTextureSet = m_DefaultTextureSet;
        m_CurrentScissor = {0, 0, m_Width, m_Height};
        const uint32_t indexBefore = static_cast<uint32_t>(m_Indices.size());
        UiColorPipelineDiagnostic::Get().AppendTestGrid(
            m_Vertices,
            m_Indices,
            static_cast<float>(width),
            static_cast<float>(height));
        const uint32_t indexAdded = static_cast<uint32_t>(m_Indices.size()) - indexBefore;
        if (indexAdded > 0) {
            AddOrMergeBatch(indexAdded);
        }
        m_LastBuiltWidth = width;
        m_LastBuiltHeight = height;
        return;
    }
    
    // Helper lambda to count widgets (diagnostics only — avoid per-frame tree walk).
    if (Widget::s_GlobalDiagnostics) {
        std::function<void(const std::shared_ptr<Widget>&)> CountWidgets = [&](const std::shared_ptr<Widget>& w) {
            if (!w) return;
            Widget::s_GlobalDiagnostics->totalWidgetCount++;
            if (w->IsVisible()) Widget::s_GlobalDiagnostics->visibleWidgetCount++;
            else Widget::s_GlobalDiagnostics->hiddenWidgetCount++;
            for (const auto& child : w->GetChildren()) {
                CountWidgets(child);
            }
        };
        CountWidgets(root);
        Widget::s_GlobalDiagnostics->layoutPassCount++;
    }

    // Skip Measure/Arrange only when this root was already laid out for this size
    // AND the tree is not dirty. After ViewBuilder::Reconcile, children often have
    // zero/stale geometry while the root still reports the full viewport — skipping
    // layout in that case paints everything at the origin.
    const Rect& existing = root->GetGeometry();
    const bool sizeMatches =
        existing.x == 0.0f && existing.y == 0.0f &&
        std::abs(existing.width - static_cast<float>(width)) < 0.5f &&
        std::abs(existing.height - static_cast<float>(height)) < 0.5f;
    const bool alreadyLaidOut = sizeMatches && !root->NeedsLayout();
    if (runLayout && !alreadyLaidOut) {
        UiPathDiagnostics::Get().OnLayoutPass();
        root->Measure(Size{static_cast<float>(width), static_cast<float>(height)});
        root->Arrange(Rect{0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)});
        root->ClearSubtreeLayoutDirty();
    }

    if (IsEnvEnabled("WE_KINDUI_DUMP_LAYOUT")) {
        static uint64_t s_LastDumpFrame = 0;
        const uint64_t frame = we::runtime::core::FrameCounter::GetFrameNumber();
        if (frame != s_LastDumpFrame && (frame < 5 || (frame % 120) == 0)) {
            s_LastDumpFrame = frame;
            int count = 0;
            HE_INFO("[KindUI Layout] --- frame " + std::to_string(frame)
                + " viewport " + std::to_string(width) + "x" + std::to_string(height)
                + (alreadyLaidOut ? " (layout skipped)" : " (layout run)") + " ---");
            DumpWidgetLayoutTree(root, 0, count);
        }
    }
    
    PaintContext paintCtx;
    if (TextUIService* textService = m_Renderer->GetTextUIService()) {
        paintCtx.SetTextUIService(textService);
    }
    if (Widget::s_GlobalDiagnostics) {
        Widget::s_GlobalDiagnostics->paintCalls++;
        UiPathDiagnostics::Get().SetWidgetsVisited(Widget::s_GlobalDiagnostics->totalWidgetCount);
    }
    UiColorCompositionDiagnostic::InvokeProbeRegistrar(root);

    UiPathDiagnostics::Get().OnPaintPass();
    // Opaque full-frame base: type-5 replace compositing, never alpha-blend against swapchain clear.
    paintCtx.DrawSurface(
        Rect{0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)},
        SurfaceRole::Workspace,
        0.0f,
        "WorkspaceBackdrop");
    root->Paint(paintCtx);

    auto& compositionDiag = UiColorCompositionDiagnostic::Get();
    if (compositionDiag.ShouldInjectPanelFlatOverride()) {
        const Rect overrideRect = compositionDiag.GetPanelFlatOverrideRect();
        const Rect inner = {
            overrideRect.x + 8.0f,
            overrideRect.y + 8.0f,
            std::max(0.0f, overrideRect.width - 16.0f),
            std::max(0.0f, overrideRect.height - 16.0f)
        };
        if (inner.width >= 4.0f && inner.height >= 4.0f) {
            paintCtx.DrawSurface(inner, SurfaceRole::Panel, 0.0f, "CompositionFlatOverride");
            compositionDiag.NotifyPanelFlatOverrideInjected();
        }
    }

    if (UiColorDebug::IsSemanticAuditEnabled()) {
        UiColorDebug::Get().AuditPaintCommands(paintCtx.GetCommands());
    }

    if (UiColorCompositionDiagnostic::IsEnabled()) {
        compositionDiag.RecordDrawCommands(paintCtx.GetCommands());
    }

    m_Diagnostics.paintCommandsRecorded = static_cast<uint32_t>(paintCtx.GetCommands().size());
    UiPathDiagnostics::Get().SetPaintCommands(m_Diagnostics.paintCommandsRecorded);
    root->ClearSubtreePaintDirty();
    
    // Convert paint commands to geometry
    const auto& commands = paintCtx.GetCommands();
    for (const auto& cmd : commands) {
        ConvertDrawCommand(cmd);
    }

    if (UiColorDebug::IsOverlayEnabled()) {
        m_CurrentTextureSet = m_DefaultTextureSet;
        m_CurrentScissor = {0, 0, m_Width, m_Height};
        const uint32_t indexBefore = static_cast<uint32_t>(m_Indices.size());
        UiColorDebug::Get().AppendOverlaySwatches(
            m_Vertices,
            m_Indices,
            static_cast<float>(width),
            static_cast<float>(height));
        const uint32_t indexAdded = static_cast<uint32_t>(m_Indices.size()) - indexBefore;
        if (indexAdded > 0) {
            AddOrMergeBatch(indexAdded);
        }
    }

    m_LastBuiltWidth = width;
    m_LastBuiltHeight = height;
}

void UIWidgetAdapter::AddOrMergeBatch(
    uint32_t indexCount,
    bool isText,
    uint32_t atlasW,
    uint32_t atlasH,
    float msdfRange,
    bool opaqueReplace)
{
    if (indexCount == 0) return;

    const float scX = static_cast<float>(m_CurrentScissor.x);
    const float scY = static_cast<float>(m_CurrentScissor.y);
    const float scW = static_cast<float>(m_CurrentScissor.width);
    const float scH = static_cast<float>(m_CurrentScissor.height);

    if (!m_Batches.empty()) {
        auto& last = m_Batches.back();
        if (last.textureSet == m_CurrentTextureSet &&
            last.isText == isText &&
            last.opaqueReplace == opaqueReplace &&
            last.scissor[0] == scX &&
            last.scissor[1] == scY &&
            last.scissor[2] == scW &&
            last.scissor[3] == scH &&
            (!isText || (last.atlasWidth == atlasW && last.atlasHeight == atlasH && last.msdfPixelRange == msdfRange)))
        {
            last.indexCount += indexCount;
            return;
        }
    }

    UIRenderBatch batch{};
    batch.textureSet = m_CurrentTextureSet;
    batch.indexCount = indexCount;
    batch.firstIndex = static_cast<uint32_t>(m_Indices.size()) - indexCount;
    batch.vertexOffset = 0;
    batch.scissor[0] = scX;
    batch.scissor[1] = scY;
    batch.scissor[2] = scW;
    batch.scissor[3] = scH;
    batch.stencilRef = 0;
    batch.isText = isText;
    batch.opaqueReplace = opaqueReplace;
    batch.atlasWidth = atlasW;
    batch.atlasHeight = atlasH;
    batch.msdfPixelRange = msdfRange;

    m_Batches.push_back(batch);
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
    
    // Clamp scissor to screen bounds.
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
        return;
    }
    if (cmd.color.a == 0.0f) {
        return;
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

    const bool opaqueFill = ColorSpace::IsOpaqueAuthoring(cmd.color);
    constexpr float solidType = 5.0f;

    // Opaque surfaces: type-5 solid quad + replace blending (One, Zero).
    if (opaqueFill) {
        UIVertex2 v0{{x, y}, {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, 1.0f}, {0, 0, 0, 0}, {0.0f, solidType, 0.0f, 0.0f}};
        UIVertex2 v1{{x + w, y}, {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, 1.0f}, {0, 0, 0, 0}, {0.0f, solidType, 0.0f, 0.0f}};
        UIVertex2 v2{{x + w, y + h}, {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, 1.0f}, {0, 0, 0, 0}, {0.0f, solidType, 0.0f, 0.0f}};
        UIVertex2 v3{{x, y + h}, {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, 1.0f}, {0, 0, 0, 0}, {0.0f, solidType, 0.0f, 0.0f}};

        const uint32_t startIndex = static_cast<uint32_t>(m_Vertices.size());
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

        AddOrMergeBatch(6, false, 0, 0, 0.0f, true);

        if (UiColorDebug::IsEnabled()) {
            ColorToken token{};
            if (UiColorDebug::TryMatchToken(cmd.color, token)) {
                UiColorDebug::Get().TraceVertex(
                    "UIWidgetAdapter::GenerateRectGeometry",
                    token,
                    cmd.color,
                    cmd.rect,
                    solidType);
            }
        }
        return;
    }
    
    // Semi-transparent rects still use SDF feathering + alpha blending.
    float type = 1.0f;
    
    UIVertex2 v0{ {x,     y},     {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {x, y, w, h}, {cmd.borderRadius, type, 0.0f, 0.0f} };
    UIVertex2 v1{ {x + w, y},     {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {x, y, w, h}, {cmd.borderRadius, type, 0.0f, 0.0f} };
    UIVertex2 v2{ {x + w, y + h}, {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {x, y, w, h}, {cmd.borderRadius, type, 0.0f, 0.0f} };
    UIVertex2 v3{ {x,     y + h}, {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, cmd.color.a}, {x, y, w, h}, {cmd.borderRadius, type, 0.0f, 0.0f} };
    
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

    AddOrMergeBatch(6);

    if (UiColorDebug::IsEnabled()) {
        ColorToken token{};
        if (UiColorDebug::TryMatchToken(cmd.color, token)) {
            UiColorDebug::Get().TraceVertex(
                "UIWidgetAdapter::GenerateRectGeometry",
                token,
                cmd.color,
                cmd.rect,
                type);
        }
    }
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
    we::rhi::RHIDescriptorSetHandle fontDescriptor = we::rhi::RHIDescriptorSetHandle::Invalid;
    const uint32_t startTotalIndex = static_cast<uint32_t>(m_Indices.size());
    UIRenderBatch batchInfo;
    if (!textService->GenerateTextGeometry(cmd, m_Vertices, m_Indices, fontDescriptor, &batchInfo)) {
        ++m_Diagnostics.textDrawsSkipped;
        return;
    }

    m_CurrentTextureSet = fontDescriptor;
    const uint32_t textIndexCount = static_cast<uint32_t>(m_Indices.size()) - startTotalIndex;
    m_Diagnostics.textGlyphsResolved += textIndexCount / 6;
    m_Diagnostics.textVerticesGenerated += static_cast<uint32_t>(m_Vertices.size());
    m_Diagnostics.textIndicesGenerated += static_cast<uint32_t>(m_Indices.size());

    AddOrMergeBatch(textIndexCount, batchInfo.isText, batchInfo.atlasWidth, batchInfo.atlasHeight, batchInfo.msdfPixelRange);
    ++m_Diagnostics.textBatchesCreated;

    if (textService->IsDebugEnabled()) {
        const auto& debugGlyphs = textService->LastDebugGlyphs();
        if (!debugGlyphs.empty()) {
            const uint32_t debugStart = static_cast<uint32_t>(m_Indices.size());
            constexpr float outlineType = 1.0f;
            constexpr float outlineRadius = 0.0f;
            for (const auto& g : debugGlyphs) {
                const float x = g.bounds.x;
                const float y = g.bounds.y;
                const float w = std::max(g.bounds.width, 1.0f);
                const float h = std::max(g.bounds.height, 1.0f);
                // Magenta wireframe-ish fill at low alpha so bounds are visible over missing glyphs.
                const Color c{1.0f, 0.2f, 0.8f, 0.35f};
                UIVertex2 v0{{x, y}, {0.5f, 0.5f}, {c.r, c.g, c.b, c.a}, {x, y, w, h}, {outlineRadius, outlineType, 0.0f, 0.0f}};
                UIVertex2 v1{{x + w, y}, {0.5f, 0.5f}, {c.r, c.g, c.b, c.a}, {x, y, w, h}, {outlineRadius, outlineType, 0.0f, 0.0f}};
                UIVertex2 v2{{x + w, y + h}, {0.5f, 0.5f}, {c.r, c.g, c.b, c.a}, {x, y, w, h}, {outlineRadius, outlineType, 0.0f, 0.0f}};
                UIVertex2 v3{{x, y + h}, {0.5f, 0.5f}, {c.r, c.g, c.b, c.a}, {x, y, w, h}, {outlineRadius, outlineType, 0.0f, 0.0f}};
                const uint32_t base = static_cast<uint32_t>(m_Vertices.size());
                m_Vertices.push_back(v0);
                m_Vertices.push_back(v1);
                m_Vertices.push_back(v2);
                m_Vertices.push_back(v3);
                m_Indices.push_back(base + 0);
                m_Indices.push_back(base + 1);
                m_Indices.push_back(base + 2);
                m_Indices.push_back(base + 2);
                m_Indices.push_back(base + 3);
                m_Indices.push_back(base + 0);
            }
            UIRenderBatch debugBatch;
            debugBatch.textureSet = m_Renderer->GetDummyDescriptorSet();
            debugBatch.indexCount = static_cast<uint32_t>(m_Indices.size()) - debugStart;
            debugBatch.firstIndex = debugStart;
            debugBatch.vertexOffset = 0;
            debugBatch.scissor[0] = static_cast<float>(m_CurrentScissor.x);
            debugBatch.scissor[1] = static_cast<float>(m_CurrentScissor.y);
            debugBatch.scissor[2] = static_cast<float>(m_CurrentScissor.width);
            debugBatch.scissor[3] = static_cast<float>(m_CurrentScissor.height);
            debugBatch.isText = false;
            m_Batches.push_back(debugBatch);
        }
    }
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
    
    AddOrMergeBatch(6);
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

    AddOrMergeBatch(6);

    if (UiColorDebug::IsEnabled()) {
        UiColorDebug::Get().TraceViewportTexture(cmd.rect);
    }
}

void UIWidgetAdapter::GenerateIconGeometry(const DrawCommand& cmd) {
    if (!m_Renderer || !m_Renderer->GetIconRenderer()) {
        return;
    }

    if (cmd.iconStem.empty() || cmd.iconSizePx == 0) {
        return;
    }

    WindIconRef icon{ cmd.iconStem.c_str(), cmd.iconSizePx };

    IconRenderer* iconRenderer = m_Renderer->GetIconRenderer();
    const IconDrawInfo drawInfo = iconRenderer->GetIconDrawInfo(icon);

    if (!drawInfo.valid || drawInfo.descriptorSet == we::rhi::RHIDescriptorSetHandle::Invalid) {
        return;
    }

    m_CurrentTextureSet = drawInfo.descriptorSet;

    const float w = cmd.rect.width > 0.0f ? cmd.rect.width : static_cast<float>(icon.sizePx);
    const float h = cmd.rect.height > 0.0f ? cmd.rect.height : static_cast<float>(icon.sizePx);
    const float x = SnapPx(cmd.rect.x + (cmd.rect.width - w) * 0.5f);
    const float y = SnapPx(cmd.rect.y + (cmd.rect.height - h) * 0.5f);

    const float type = drawInfo.shaderType;
    const Color color = Color::White();

    UIVertex2 v0{ {x,     y},     {drawInfo.uvMin[0], drawInfo.uvMin[1]}, {color.r, color.g, color.b, color.a}, {x, y, w, h}, {0.0f, type, 0.0f, 0.0f} };
    UIVertex2 v1{ {x + w, y},     {drawInfo.uvMax[0], drawInfo.uvMin[1]}, {color.r, color.g, color.b, color.a}, {x, y, w, h}, {0.0f, type, 0.0f, 0.0f} };
    UIVertex2 v2{ {x + w, y + h}, {drawInfo.uvMax[0], drawInfo.uvMax[1]}, {color.r, color.g, color.b, color.a}, {x, y, w, h}, {0.0f, type, 0.0f, 0.0f} };
    UIVertex2 v3{ {x,     y + h}, {drawInfo.uvMin[0], drawInfo.uvMax[1]}, {color.r, color.g, color.b, color.a}, {x, y, w, h}, {0.0f, type, 0.0f, 0.0f} };
    
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
    
    AddOrMergeBatch(6);
}

void UIWidgetAdapter::GenerateLineGeometry(const DrawCommand& cmd) {
    Point s{SnapPx(cmd.lineStart.x), SnapPx(cmd.lineStart.y)};
    Point e{SnapPx(cmd.lineEnd.x), SnapPx(cmd.lineEnd.y)};
    const float thickness = std::max(1.0f, SnapPx(cmd.thickness));
    const float dx = e.x - s.x;
    const float dy = e.y - s.y;
    const float len = std::sqrt(dx * dx + dy * dy);
    if (len <= 0.0f) {
        return;
    }

    const Color color = cmd.color;
    const bool opaqueFill = ColorSpace::IsOpaqueAuthoring(color);
    const float alpha = opaqueFill ? 1.0f : color.a;

    auto emitSolidRect = [&](float rx, float ry, float rw, float rh) {
        const float x0 = SnapPx(rx);
        const float y0 = SnapPx(ry);
        const float x1 = SnapPx(rx + rw);
        const float y1 = SnapPx(ry + rh);
        const float rectW = x1 - x0;
        const float rectH = y1 - y0;
        if (rectW <= 0.0f || rectH <= 0.0f) {
            return;
        }
        constexpr float solidType = 5.0f;
        UIVertex2 v0{{x0, y0}, {0.5f, 0.5f}, {color.r, color.g, color.b, alpha}, {0, 0, 0, 0}, {0.0f, solidType, 0.0f, 0.0f}};
        UIVertex2 v1{{x1, y0}, {0.5f, 0.5f}, {color.r, color.g, color.b, alpha}, {0, 0, 0, 0}, {0.0f, solidType, 0.0f, 0.0f}};
        UIVertex2 v2{{x1, y1}, {0.5f, 0.5f}, {color.r, color.g, color.b, alpha}, {0, 0, 0, 0}, {0.0f, solidType, 0.0f, 0.0f}};
        UIVertex2 v3{{x0, y1}, {0.5f, 0.5f}, {color.r, color.g, color.b, alpha}, {0, 0, 0, 0}, {0.0f, solidType, 0.0f, 0.0f}};
        const uint32_t startIndex = static_cast<uint32_t>(m_Vertices.size());
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
        AddOrMergeBatch(6, false, 0, 0, 0.0f, opaqueFill);
    };

    // Axis-aligned lines: pixel-snapped solid fills for crisp 1px separators.
    if (std::abs(dx) < 0.5f) {
        const float x = s.x - thickness * 0.5f;
        const float y = std::min(s.y, e.y);
        const float h = std::max(len, 1.0f);
        emitSolidRect(x, y, thickness, h);
        return;
    }
    if (std::abs(dy) < 0.5f) {
        const float x = std::min(s.x, e.x);
        const float y = s.y - thickness * 0.5f;
        const float w = std::max(len, 1.0f);
        emitSolidRect(x, y, w, thickness);
        return;
    }

    // Diagonal lines: solid quad tint (type 5) — no texture dependency.
    float ndx = dx / len;
    float ndy = dy / len;
    const float px = -ndy * (thickness * 0.5f);
    const float py = ndx * (thickness * 0.5f);
    const float solidType = 5.0f;

    UIVertex2 v0{{s.x + px, s.y + py}, {0.5f, 0.5f}, {color.r, color.g, color.b, alpha}, {0, 0, 0, 0}, {0.0f, solidType, 0.0f, 0.0f}};
    UIVertex2 v1{{s.x - px, s.y - py}, {0.5f, 0.5f}, {color.r, color.g, color.b, alpha}, {0, 0, 0, 0}, {0.0f, solidType, 0.0f, 0.0f}};
    UIVertex2 v2{{e.x - px, e.y - py}, {0.5f, 0.5f}, {color.r, color.g, color.b, alpha}, {0, 0, 0, 0}, {0.0f, solidType, 0.0f, 0.0f}};
    UIVertex2 v3{{e.x + px, e.y + py}, {0.5f, 0.5f}, {color.r, color.g, color.b, alpha}, {0, 0, 0, 0}, {0.0f, solidType, 0.0f, 0.0f}};

    const uint32_t startIndex = static_cast<uint32_t>(m_Vertices.size());
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
    AddOrMergeBatch(6, false, 0, 0, 0.0f, opaqueFill);
}

void UIWidgetAdapter::GenerateShadowGeometry(const DrawCommand& cmd) {
    // Three tight layers keep shadows soft without muddy dark halos on dark surfaces.
    const int numLayers = 3;
    const float shadowSpread = cmd.blur / static_cast<float>(numLayers);
    const float baseAlpha = cmd.color.a / (static_cast<float>(numLayers) * 1.35f);

    const float x0 = SnapPx(cmd.rect.x);
    const float y0 = SnapPx(cmd.rect.y);
    const float x1 = SnapPx(cmd.rect.x + cmd.rect.width);
    const float y1 = SnapPx(cmd.rect.y + cmd.rect.height);
    const float w = x1 - x0;
    const float h = y1 - y0;

    for (int i = 0; i < numLayers; ++i) {
        const float expand = (i + 1) * shadowSpread;
        float sx = SnapPx(x0 - expand);
        float sy = SnapPx(y0 - expand);
        float sw = SnapPx(x1 + expand) - sx;
        float sh = SnapPx(y1 + expand) - sy;
        const float alpha = baseAlpha * (1.0f - static_cast<float>(i) / static_cast<float>(numLayers));

        if (sx < 0.0f) {
            sw += sx;
            sx = 0.0f;
        }
        if (sy < 0.0f) {
            sh += sy;
            sy = 0.0f;
        }

        const float type = 1.0f;
        const float r = cmd.borderRadius + expand;

        UIVertex2 v0{ {sx,      sy},      {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, alpha}, {sx, sy, sw, sh}, {r, type, 0.0f, 0.0f} };
        UIVertex2 v1{ {sx + sw, sy},      {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, alpha}, {sx, sy, sw, sh}, {r, type, 0.0f, 0.0f} };
        UIVertex2 v2{ {sx + sw, sy + sh}, {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, alpha}, {sx, sy, sw, sh}, {r, type, 0.0f, 0.0f} };
        UIVertex2 v3{ {sx,      sy + sh}, {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, alpha}, {sx, sy, sw, sh}, {r, type, 0.0f, 0.0f} };

        const uint32_t startIndex = static_cast<uint32_t>(m_Vertices.size());
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

    AddOrMergeBatch(numLayers * 6);
}

void UIWidgetAdapter::GenerateGradientGeometry(const DrawCommand& cmd) {
    const bool sameOpaqueColor =
        ColorSpace::IsOpaqueAuthoring(cmd.color)
        && ColorSpace::IsOpaqueAuthoring(cmd.colorBottom)
        && std::fabs(cmd.color.r - cmd.colorBottom.r) < 0.001f
        && std::fabs(cmd.color.g - cmd.colorBottom.g) < 0.001f
        && std::fabs(cmd.color.b - cmd.colorBottom.b) < 0.001f;
    if (sameOpaqueColor && cmd.borderRadius <= 0.0f) {
        DrawCommand solid = cmd;
        solid.colorBottom = cmd.color;
        GenerateRectGeometry(solid);
        return;
    }

    float x0 = SnapPx(cmd.rect.x);
    float y0 = SnapPx(cmd.rect.y);
    float x1 = SnapPx(cmd.rect.x + cmd.rect.width);
    float y1 = SnapPx(cmd.rect.y + cmd.rect.height);
    float x = x0;
    float y = y0;
    float w = x1 - x0;
    float h = y1 - y0;
    
    float type = 1.0f;
    
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
    
    AddOrMergeBatch(6);
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
    
    float type = 2.0f;
    const float thickness = std::max(1.0f, SnapPx(cmd.thickness));
    const bool opaqueFill = ColorSpace::IsOpaqueAuthoring(cmd.color);
    const float opaqueHard = opaqueFill ? 1.0f : 0.0f;
    const float fillAlpha = opaqueFill ? 1.0f : cmd.color.a;
    
    UIVertex2 v0{ {x,     y},     {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, fillAlpha}, {x, y, w, h}, {cmd.borderRadius, type, thickness, opaqueHard} };
    UIVertex2 v1{ {x + w, y},     {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, fillAlpha}, {x, y, w, h}, {cmd.borderRadius, type, thickness, opaqueHard} };
    UIVertex2 v2{ {x + w, y + h}, {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, fillAlpha}, {x, y, w, h}, {cmd.borderRadius, type, thickness, opaqueHard} };
    UIVertex2 v3{ {x,     y + h}, {0.5f, 0.5f}, {cmd.color.r, cmd.color.g, cmd.color.b, fillAlpha}, {x, y, w, h}, {cmd.borderRadius, type, thickness, opaqueHard} };
    
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
    
    AddOrMergeBatch(6, false, 0, 0, 0.0f, opaqueFill);
}

} // namespace we::runtime::kindui
