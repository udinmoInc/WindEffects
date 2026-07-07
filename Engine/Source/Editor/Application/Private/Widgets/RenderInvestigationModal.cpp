#include "Widgets/RenderInvestigationModal.hpp"
#include "Layout/OverlayManager.hpp"
#include "Widgets/RenderTargetPreviewWidget.hpp"
#include "Renderer/RenderDebugger.hpp"
#include "Renderer/RenderForensics.hpp"
#include "Renderer/RenderGpuInvestigator.hpp"
#include "Core/PaintContext.hpp"
#include "Core/Theme.hpp"

#include <SDL3/SDL_keycode.h>
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace we::UI {

namespace {

constexpr float kHeaderHeight = 40.0f;
constexpr float kPassPanelWidth = 260.0f;
constexpr float kResourcePanelWidth = 300.0f;
constexpr float kBottomHeight = 220.0f;
constexpr uint32_t kMaxPreviewDrawCols = 320;
constexpr uint32_t kMaxPreviewDrawRows = 240;

std::string F3(float v) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(3) << v;
    return ss.str();
}

std::string V3(const glm::vec3& v) {
    return "(" + F3(v.x) + ", " + F3(v.y) + ", " + F3(v.z) + ")";
}

TextStyle BodyStyle() {
    TextStyle s;
    s.size = Theme::Get().TextSizeProperty - 1.0f;
    s.color = Theme::Get().TextSecondary;
    return s;
}

TextStyle HeaderStyle() {
    TextStyle s;
    s.size = Theme::Get().TextSizeProperty;
    s.color = Theme::Get().TextPrimary;
    s.bold = true;
    return s;
}

Color PassHealthColor(const we::runtime::renderer::GpuPassValidation& pass,
    const we::runtime::renderer::RenderDebuggerFrameSnapshot& snapshot) {
    if (snapshot.firstFailure.detected && pass.passName == snapshot.firstFailure.passName) {
        return { 0.95f, 0.25f, 0.25f, 1.0f };
    }
    if (pass.abnormal) return { 0.95f, 0.75f, 0.2f, 1.0f };
    if (pass.executed) return { 0.35f, 0.85f, 0.45f, 1.0f };
    return Theme::Get().TextSecondary;
}

const char* ViewModeName(InvestigationTextureViewMode mode) {
    switch (mode) {
        case InvestigationTextureViewMode::RGB: return "RGB";
        case InvestigationTextureViewMode::Alpha: return "Alpha";
        case InvestigationTextureViewMode::Red: return "Red";
        case InvestigationTextureViewMode::Green: return "Green";
        case InvestigationTextureViewMode::Blue: return "Blue";
        case InvestigationTextureViewMode::HdrHeatmap: return "HDR Heat";
        case InvestigationTextureViewMode::Luminance: return "Luminance";
        case InvestigationTextureViewMode::FalseColor: return "False Color";
        case InvestigationTextureViewMode::Histogram: return "Histogram";
        default: return "RGB";
    }
}

Color SamplePreview(const std::vector<uint8_t>& rgba, uint32_t width, uint32_t height,
    uint32_t x, uint32_t y, InvestigationTextureViewMode mode) {
    Color c{ 0.12f, 0.12f, 0.14f, 1.0f };
    if (rgba.empty() || width == 0 || height == 0) return c;
    const uint32_t sx = (std::min)(x, width - 1);
    const uint32_t sy = (std::min)(y, height - 1);
    const size_t idx = (static_cast<size_t>(sy) * width + sx) * 4;
    if (idx + 2 >= rgba.size()) return c;
    const float r = rgba[idx] / 255.0f;
    const float g = rgba[idx + 1] / 255.0f;
    const float b = rgba[idx + 2] / 255.0f;
    switch (mode) {
        case InvestigationTextureViewMode::Alpha: return { r, r, r, 1.0f };
        case InvestigationTextureViewMode::Red: return { r, 0.0f, 0.0f, 1.0f };
        case InvestigationTextureViewMode::Green: return { 0.0f, g, 0.0f, 1.0f };
        case InvestigationTextureViewMode::Blue: return { 0.0f, 0.0f, b, 1.0f };
        case InvestigationTextureViewMode::Luminance: {
            const float lum = 0.2126f * r + 0.7152f * g + 0.0722f * b;
            return { lum, lum, lum, 1.0f };
        }
        case InvestigationTextureViewMode::HdrHeatmap: {
            const float lum = 0.2126f * r + 0.7152f * g + 0.0722f * b;
            const float t = std::clamp(lum, 0.0f, 1.0f);
            return { t, 0.25f, 1.0f - t, 1.0f };
        }
        case InvestigationTextureViewMode::FalseColor: {
            const float lum = 0.2126f * r + 0.7152f * g + 0.0722f * b;
            if (lum < 0.33f) return { 0.0f, lum * 3.0f, 1.0f - lum * 3.0f, 1.0f };
            if (lum < 0.66f) return { (lum - 0.33f) * 3.0f, 1.0f, 0.0f, 1.0f };
            return { 1.0f, 1.0f - (lum - 0.66f) * 3.0f, 0.0f, 1.0f };
        }
        default: return { r, g, b, 1.0f };
    }
}

class InvestigationTextureCanvas : public Widget {
public:
    void SetPreview(const std::vector<uint8_t>& rgba, uint32_t width, uint32_t height,
        InvestigationTextureViewMode mode, const std::array<uint32_t, 64>* histogram) {
        m_Rgba = rgba;
        m_Width = width;
        m_Height = height;
        m_Mode = mode;
        m_Histogram = histogram;
    }

    Size Measure(const Size& availableSize) override {
        m_DesiredSize = availableSize;
        return m_DesiredSize;
    }

    void Arrange(const Rect& allottedRect) override {
        m_Geometry = allottedRect;
        m_PreviewRect = allottedRect;
        if (m_Mode == InvestigationTextureViewMode::Histogram) {
            m_PreviewRect.height = (std::max)(40.0f, allottedRect.height * 0.35f);
        } else if (m_Width > 0 && m_Height > 0) {
            const float aspect = static_cast<float>(m_Width) / static_cast<float>(m_Height);
            float drawW = allottedRect.width;
            float drawH = allottedRect.height;
            if (drawW / drawH > aspect) drawW = drawH * aspect;
            else drawH = drawW / aspect;
            m_PreviewRect = Rect{
                allottedRect.x + (allottedRect.width - drawW) * 0.5f,
                allottedRect.y + (allottedRect.height - drawH) * 0.5f,
                drawW,
                drawH
            };
        }
    }

    void Paint(PaintContext& context) override {
        const auto& theme = Theme::Get();
        context.DrawRect(m_Geometry, theme.PanelBackground, 4.0f);

        if (m_Rgba.empty() || m_Width == 0 || m_Height == 0) {
            context.DrawText("No preview yet — waiting for GPU readback (one frame)",
                Point{ m_Geometry.x + 12.0f, m_Geometry.y + 12.0f }, theme.TextSecondary, theme.TextSizeProperty - 1.0f);
            return;
        }

        if (m_Mode == InvestigationTextureViewMode::Histogram && m_Histogram) {
            const float barW = m_Geometry.width / 64.0f;
            uint32_t peak = 1;
            for (uint32_t v : *m_Histogram) peak = (std::max)(peak, v);
            for (int i = 0; i < 64; ++i) {
                const float h = (static_cast<float>((*m_Histogram)[static_cast<size_t>(i)]) / static_cast<float>(peak))
                    * m_PreviewRect.height;
                context.DrawRect(Rect{
                    m_PreviewRect.x + i * barW,
                    m_PreviewRect.y + m_PreviewRect.height - h,
                    (std::max)(barW, 1.0f),
                    h
                }, Color{ 0.45f, 0.65f, 0.95f, 1.0f });
            }
            return;
        }

        const float pxW = m_PreviewRect.width / static_cast<float>(kMaxPreviewDrawCols);
        const float pxH = m_PreviewRect.height / static_cast<float>(kMaxPreviewDrawRows);
        const uint32_t cols = (std::min)(m_Width, kMaxPreviewDrawCols);
        const uint32_t rows = (std::min)(m_Height, kMaxPreviewDrawRows);
        for (uint32_t cy = 0; cy < rows; ++cy) {
            const uint32_t sy = (cy * m_Height) / rows;
            for (uint32_t cx = 0; cx < cols; ++cx) {
                const uint32_t sx = (cx * m_Width) / cols;
                const Color c = SamplePreview(m_Rgba, m_Width, m_Height, sx, sy, m_Mode);
                context.DrawRect(Rect{
                    m_PreviewRect.x + cx * pxW,
                    m_PreviewRect.y + cy * pxH,
                    (std::max)(pxW, 1.0f),
                    (std::max)(pxH, 1.0f)
                }, c);
            }
        }
    }

private:
    std::vector<uint8_t> m_Rgba;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    InvestigationTextureViewMode m_Mode = InvestigationTextureViewMode::RGB;
    const std::array<uint32_t, 64>* m_Histogram = nullptr;
    Rect m_PreviewRect{};
};

int PassSortKey(we::runtime::renderer::ForensicPassId id) {
    using FP = we::runtime::renderer::ForensicPassId;
    switch (id) {
        case FP::Clear: return 0;
        case FP::Shadow: return 10;
        case FP::Geometry: return 20;
        case FP::AtmosphereLUT: return 30;
        case FP::SkyAtmosphere: return 40;
        case FP::VolumetricClouds: return 50;
        case FP::Fog: return 60;
        case FP::AerialPerspective: return 65;
        case FP::Lighting: return 70;
        case FP::Grid: return 75;
        case FP::Gizmos: return 76;
        case FP::SceneComposite: return 80;
        case FP::LuminanceReduce: return 90;
        case FP::Exposure: return 100;
        case FP::Bloom: return 110;
        case FP::ToneMapping: return 120;
        case FP::UI: return 130;
        case FP::Present: return 140;
        default: return 999;
    }
}

} // namespace

// ---------------------------------------------------------------------------
// RenderInvestigationModal
// ---------------------------------------------------------------------------

RenderInvestigationModal::RenderInvestigationModal() {
    m_TextureCanvas = std::make_shared<InvestigationTextureCanvas>();
}

void RenderInvestigationModal::Construct() {
    if (m_Root) {
        return;
    }
    BuildLayout();
}

void RenderInvestigationModal::BuildLayout() {
    m_Root = std::make_shared<VerticalBox>();
    m_Header = std::make_shared<HorizontalBox>();

    m_TitleLabel = std::make_shared<Label>("Render Investigation", Theme::Get().TextPrimary, Theme::Get().TextSizeHeader);
    m_FrameLabel = std::make_shared<Label>("Frame —", Theme::Get().TextSecondary, Theme::Get().TextSizeProperty);
    m_SearchBox = std::make_shared<TextBox>("", [this](const std::string& text) {
        m_SearchQuery = text;
        RebuildPassList();
        RebuildTargetList();
    });
    m_CaptureButton = std::make_shared<Button>("Capture (F9)", []() {
        we::runtime::renderer::RenderGpuInvestigator::Get().RequestBaselineCapture();
        we::runtime::renderer::RenderDebugger::Get().RequestCapture();
    });
    m_CompareButton = std::make_shared<Button>("Compare (Shift+F9)", []() {
        we::runtime::renderer::RenderGpuInvestigator::Get().RequestComparisonCapture();
    });
    m_StepBackButton = std::make_shared<Button>("< Step", [this]() { StepPass(-1); });
    m_StepFwdButton = std::make_shared<Button>("Step >", [this]() { StepPass(1); });
    m_StepLabel = std::make_shared<Label>("Step: live", Theme::Get().TextSecondary, Theme::Get().TextSizeProperty - 1.0f);
    m_CloseButton = std::make_shared<Button>("Close (Esc)", [this]() {
        if (m_OnClose) m_OnClose();
    });
    auto fullscreenBtn = std::make_shared<Button>("Fullscreen", [this]() {
        const auto* target = GetSelectedTarget();
        if (!target || target->previewRgba.empty()) return;
        auto* overlay = OverlayManager::Get();
        if (!overlay) return;
        const Rect root = overlay->GetGeometry();
        auto preview = std::make_shared<RenderTargetPreviewWidget>(
            target->displayName, target->previewRgba, target->previewWidth, target->previewHeight);
        preview->Measure(Size{ root.width, root.height });
        preview->Arrange(Rect{ 0.0f, 0.0f, root.width, root.height });
        preview->SetOnClose([overlay]() { overlay->CloseTopPopup(); });
        overlay->ShowPopup(preview, Point{ 0.0f, 0.0f });
    });

    m_Header->AddChild(m_TitleLabel);
    m_Header->AddChild(m_FrameLabel);
    m_Header->AddChild(m_SearchBox);
    m_Header->AddChild(m_CaptureButton);
    m_Header->AddChild(m_CompareButton);
    m_Header->AddChild(m_StepBackButton);
    m_Header->AddChild(m_StepFwdButton);
    m_Header->AddChild(m_StepLabel);
    m_Header->AddChild(fullscreenBtn);
    m_Header->AddChild(m_CloseButton);

    m_PassList = std::make_shared<VerticalBox>();
    m_PassScroll = std::make_shared<ScrollLayout>();
    m_PassScroll->SetContent(m_PassList);

    m_TargetRow = std::make_shared<HorizontalBox>();
    m_TargetScroll = std::make_shared<ScrollLayout>();
    m_TargetScroll->SetContent(m_TargetRow);

    m_ViewModeRow = std::make_shared<HorizontalBox>();
    const InvestigationTextureViewMode modes[] = {
        InvestigationTextureViewMode::RGB,
        InvestigationTextureViewMode::Alpha,
        InvestigationTextureViewMode::Red,
        InvestigationTextureViewMode::Green,
        InvestigationTextureViewMode::Blue,
        InvestigationTextureViewMode::HdrHeatmap,
        InvestigationTextureViewMode::Luminance,
        InvestigationTextureViewMode::FalseColor,
        InvestigationTextureViewMode::Histogram,
    };
    for (const auto mode : modes) {
        auto btn = std::make_shared<Button>(ViewModeName(mode), [this, mode]() {
            m_ViewMode = mode;
            RefreshResourcePanel();
            RefreshTexturePreview();
        });
        m_ViewModeButtons.push_back(btn);
        m_ViewModeRow->AddChild(btn);
    }

    m_CenterColumn = std::make_shared<VerticalBox>();
    m_CenterColumn->AddChild(m_TargetScroll);
    m_CenterColumn->AddChild(m_ViewModeRow);
    m_CenterColumn->AddChild(m_TextureCanvas);

    m_ResourceLabel = std::make_shared<Label>("Select a render target", Theme::Get().TextSecondary, Theme::Get().TextSizeProperty - 1.0f);
    m_ResourceLabel->SetWrapText(true);
    m_ResourceScroll = std::make_shared<ScrollLayout>();
    m_ResourceScroll->SetContent(m_ResourceLabel);

    m_CenterRightSplitter = std::make_shared<Splitter>(Orientation::Horizontal, 0.72f);
    m_CenterRightSplitter->SetFirstChild(m_CenterColumn);
    m_CenterRightSplitter->SetSecondChild(m_ResourceScroll);

    m_MainSplitter = std::make_shared<Splitter>(Orientation::Horizontal, 0.0f);
    m_MainSplitter->SetResizeMode(Splitter::ResizeMode::FixedFirst);
    m_MainSplitter->SetFixedFirstWidth(kPassPanelWidth);
    m_MainSplitter->SetFirstChild(m_PassScroll);
    m_MainSplitter->SetSecondChild(m_CenterRightSplitter);

    m_FailureLabel = std::make_shared<Label>("", Theme::Get().SelectedAccent, Theme::Get().TextSizeProperty);
    m_FailureLabel->SetWrapText(true);
    m_PixelLabel = std::make_shared<Label>("Pixel Inspector — Ctrl+Click viewport to probe", BodyStyle().color, BodyStyle().size);
    m_PixelLabel->SetWrapText(true);
    m_ShaderLabel = std::make_shared<Label>("Shader Inspector", BodyStyle().color, BodyStyle().size);
    m_ShaderLabel->SetWrapText(true);
    m_GraphLabel = std::make_shared<Label>("Render Graph", BodyStyle().color, BodyStyle().size);
    m_GraphLabel->SetWrapText(true);
    m_WarningsLabel = std::make_shared<Label>("", Color{ 0.95f, 0.75f, 0.2f, 1.0f }, BodyStyle().size);
    m_WarningsLabel->SetWrapText(true);

    m_BottomColumn = std::make_shared<VerticalBox>();
    m_BottomColumn->AddChild(m_FailureLabel);
    m_BottomColumn->AddChild(m_WarningsLabel);
    m_BottomColumn->AddChild(m_PixelLabel);
    m_BottomColumn->AddChild(m_ShaderLabel);
    m_BottomColumn->AddChild(m_GraphLabel);
    m_BottomScroll = std::make_shared<ScrollLayout>();
    m_BottomScroll->SetContent(m_BottomColumn);

    m_MainBottomSplitter = std::make_shared<Splitter>(Orientation::Vertical, 0.72f);
    m_MainBottomSplitter->SetFirstChild(m_MainSplitter);
    m_MainBottomSplitter->SetSecondChild(m_BottomScroll);

    m_Root->AddChild(m_Header);
    m_Root->AddChild(m_MainBottomSplitter);
    AddChild(m_Root);
}

bool RenderInvestigationModal::PassMatchesSearch(const std::string& name) const {
    if (m_SearchQuery.empty()) return true;
    return name.find(m_SearchQuery) != std::string::npos;
}

bool RenderInvestigationModal::TargetMatchesSearch(const std::string& name) const {
    if (m_SearchQuery.empty()) return true;
    return name.find(m_SearchQuery) != std::string::npos;
}

const we::runtime::renderer::GpuPassValidation* RenderInvestigationModal::GetSelectedPass() const {
    const auto& passes = m_Snapshot.gpuInvestigation.passValidations;
    if (m_SelectedPassIndex < 0 || m_SelectedPassIndex >= static_cast<int>(passes.size())) return nullptr;
    return &passes[static_cast<size_t>(m_SelectedPassIndex)];
}

const we::runtime::renderer::GpuRenderTargetInfo* RenderInvestigationModal::GetSelectedTarget() const {
    const auto& targets = m_Snapshot.gpuInvestigation.renderTargets;
    if (m_SelectedTargetIndex < 0 || m_SelectedTargetIndex >= static_cast<int>(targets.size())) return nullptr;
    return &targets[static_cast<size_t>(m_SelectedTargetIndex)];
}

void RenderInvestigationModal::SelectPass(int index) {
    m_SelectedPassIndex = index;
    m_StepPassIndex = index;
    RebuildPassList();
    RebuildTargetList();
    RefreshResourcePanel();
    RefreshPixelPanel();
    RefreshShaderPanel();
    RefreshGraphPanel();
    RefreshTexturePreview();
}

void RenderInvestigationModal::SelectTarget(int index) {
    m_SelectedTargetIndex = index;
    RebuildTargetList();
    RefreshResourcePanel();
    RefreshTexturePreview();
}

void RenderInvestigationModal::RefreshTexturePreview() {
    const auto* target = GetSelectedTarget();
    const we::runtime::renderer::GpuLutInspection* lut = nullptr;
    if (target) {
        for (const auto& l : m_Snapshot.gpuInvestigation.lutInspections) {
            if (l.name.find(target->displayName) != std::string::npos
                || target->displayName.find(l.name) != std::string::npos) {
                lut = &l;
                break;
            }
        }
    }
    if (auto canvas = std::dynamic_pointer_cast<InvestigationTextureCanvas>(m_TextureCanvas)) {
        canvas->SetPreview(
            target ? target->previewRgba : std::vector<uint8_t>{},
            target ? target->previewWidth : 0,
            target ? target->previewHeight : 0,
            m_ViewMode,
            lut ? &lut->luminanceHistogram : nullptr);
    }
    m_LastRefreshPassIndex = m_SelectedPassIndex;
    m_LastRefreshTargetIndex = m_SelectedTargetIndex;
    m_LastRefreshViewMode = m_ViewMode;
}

void RenderInvestigationModal::StepPass(int delta) {
    const int count = static_cast<int>(m_Snapshot.gpuInvestigation.passValidations.size());
    if (count == 0) return;
    if (m_StepPassIndex < 0) m_StepPassIndex = 0;
    m_StepPassIndex = (std::clamp)(m_StepPassIndex + delta, 0, count - 1);
    SelectPass(m_StepPassIndex);
    std::ostringstream ss;
    ss << "Step: " << (m_StepPassIndex + 1) << " / " << count;
    if (const auto* pass = GetSelectedPass()) ss << " — " << pass->passName;
    m_StepLabel->SetText(ss.str());
}

void RenderInvestigationModal::CycleViewMode(int delta) {
    const int count = 9;
    const int current = static_cast<int>(m_ViewMode);
    m_ViewMode = static_cast<InvestigationTextureViewMode>((current + delta + count) % count);
    RefreshResourcePanel();
}

void RenderInvestigationModal::RebuildPassList() {
    m_PassButtons.clear();
    m_PassList->ClearChildren();

    auto header = std::make_shared<Label>("Frame Pipeline", Theme::Get().TextPrimary, Theme::Get().TextSizeProperty);
    header->SetStyle(HeaderStyle());
    m_PassList->AddChild(header);

    std::vector<int> order;
    order.reserve(m_Snapshot.gpuInvestigation.passValidations.size());
    for (int i = 0; i < static_cast<int>(m_Snapshot.gpuInvestigation.passValidations.size()); ++i) {
        order.push_back(i);
    }
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        const auto& pa = m_Snapshot.gpuInvestigation.passValidations[static_cast<size_t>(a)];
        const auto& pb = m_Snapshot.gpuInvestigation.passValidations[static_cast<size_t>(b)];
        return PassSortKey(pa.passId) < PassSortKey(pb.passId);
    });

    for (int idx : order) {
        const auto& pass = m_Snapshot.gpuInvestigation.passValidations[static_cast<size_t>(idx)];
        if (!PassMatchesSearch(pass.passName)) continue;

        std::ostringstream label;
        label << (pass.executed ? "✓ " : "○ ");
        if (pass.abnormal || (m_Snapshot.firstFailure.detected && pass.passName == m_Snapshot.firstFailure.passName)) {
            label << "!! ";
        }
        label << pass.passName;
        if (pass.executed) {
            label << "  " << F3(static_cast<float>(pass.gpuMs)) << "ms";
        }

        const int captured = idx;
        auto btn = std::make_shared<Button>(label.str(), [this, captured]() { SelectPass(captured); });
        m_PassButtons.push_back(btn);
        m_PassList->AddChild(btn);

        auto detail = std::make_shared<Label>("", PassHealthColor(pass, m_Snapshot), Theme::Get().TextSizeProperty - 2.0f);
        std::ostringstream info;
        info << "  CPU " << F3(static_cast<float>(pass.cpuMs)) << "ms"
             << "  RT out=" << pass.outputTextures.size()
             << "  in=" << pass.inputTextures.size();
        if (!pass.outputTextures.empty()) info << "  → " << pass.outputTextures.front();
        detail->SetText(info.str());
        m_PassList->AddChild(detail);
    }
}

void RenderInvestigationModal::RebuildTargetList() {
    m_TargetButtons.clear();
    m_TargetRow->ClearChildren();

    const auto* pass = GetSelectedPass();
    std::vector<int> indices;
    for (int i = 0; i < static_cast<int>(m_Snapshot.gpuInvestigation.renderTargets.size()); ++i) {
        const auto& rt = m_Snapshot.gpuInvestigation.renderTargets[static_cast<size_t>(i)];
        if (!rt.exists) continue;
        if (!TargetMatchesSearch(rt.displayName)) continue;
        if (pass) {
            bool linked = false;
            for (const auto& out : pass->outputTextures) {
                if (rt.displayName.find(out) != std::string::npos || rt.id.find(out) != std::string::npos) {
                    linked = true;
                    break;
                }
            }
            if (!linked && rt.producerPass != pass->passName) continue;
        }
        indices.push_back(i);
    }
    if (indices.empty()) {
        for (int i = 0; i < static_cast<int>(m_Snapshot.gpuInvestigation.renderTargets.size()); ++i) {
            if (m_Snapshot.gpuInvestigation.renderTargets[static_cast<size_t>(i)].exists) indices.push_back(i);
        }
    }
    if (m_SelectedTargetIndex < 0 && !indices.empty()) m_SelectedTargetIndex = indices.front();
    if (m_SelectedTargetIndex >= 0
        && std::find(indices.begin(), indices.end(), m_SelectedTargetIndex) == indices.end()
        && !indices.empty()) {
        m_SelectedTargetIndex = indices.front();
    }

    for (int idx : indices) {
        const auto& rt = m_Snapshot.gpuInvestigation.renderTargets[static_cast<size_t>(idx)];
        const int captured = idx;
        const bool invalid = rt.stats.valid && (rt.stats.nanCount > 0 || rt.stats.infCount > 0 || rt.stats.negativeCount > 0);
        std::string label = rt.displayName;
        if (invalid) label = "⚠ " + label;
        auto btn = std::make_shared<Button>(label, [this, captured]() { SelectTarget(captured); });
        m_TargetButtons.push_back(btn);
        m_TargetRow->AddChild(btn);
    }

    RefreshTexturePreview();
}

void RenderInvestigationModal::RefreshResourcePanel() {
    const auto* target = GetSelectedTarget();
    if (!target) {
        m_ResourceLabel->SetText("Select a render target");
        return;
    }
    std::ostringstream ss;
    ss << "Texture: " << target->displayName << "\n";
    ss << "Producer: " << target->producerPass << "\n";
    if (!target->consumerPasses.empty()) {
        ss << "Consumers: ";
        for (size_t i = 0; i < target->consumerPasses.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << target->consumerPasses[i];
        }
        ss << "\n";
    }
    ss << "Format: " << target->format << "\n";
    ss << "Size: " << target->width << " x " << target->height << "\n";
    ss << "Memory: " << (target->memoryBytes / 1024) << " KB\n";
    if (target->stats.valid) {
        ss << "Min RGB: " << V3({ target->stats.minR, target->stats.minG, target->stats.minB }) << "\n";
        ss << "Max RGB: " << V3({ target->stats.maxR, target->stats.maxG, target->stats.maxB }) << "\n";
        ss << "Avg RGB: " << V3({ target->stats.avgR, target->stats.avgG, target->stats.avgB }) << "\n";
        ss << "NaN: " << target->stats.nanCount
           << "  Inf: " << target->stats.infCount
           << "  Negative: " << target->stats.negativeCount << "\n";
        if (target->stats.nanCount > 0 || target->stats.infCount > 0 || target->stats.negativeCount > 0) {
            ss << "\n⚠ INVALID TEXTURE\n";
        }
    }
    ss << "\nView: " << ViewModeName(m_ViewMode);
    m_ResourceLabel->SetText(ss.str());
}

void RenderInvestigationModal::RefreshPixelPanel() {
    std::ostringstream ss;
    ss << "Pixel (" << m_Snapshot.gpuInvestigation.probePixelX << ", "
       << m_Snapshot.gpuInvestigation.probePixelY << ")  UV="
       << F3(m_Snapshot.gpuInvestigation.probeUV.x) << ","
       << F3(m_Snapshot.gpuInvestigation.probeUV.y) << "\n\n";

    int stageIndex = 0;
    for (const auto& stage : m_Snapshot.gpuInvestigation.pixelPipeline) {
        if (m_StepPassIndex >= 0 && stageIndex > m_StepPassIndex) break;
        ++stageIndex;
        ss << stage.passName;
        if (stage.firstInvalidStage) ss << "  <<< FIRST INVALID";
        ss << "\n";
        if (!stage.valid) { ss << "  (no data)\n  |\n  v\n"; continue; }
        ss << "  HDR " << V3({ stage.hdrRgb.r, stage.hdrRgb.g, stage.hdrRgb.b });
        if (stage.ldrRgb.r > 0.0f || stage.ldrRgb.g > 0.0f || stage.ldrRgb.b > 0.0f) {
            ss << "  LDR " << V3({ stage.ldrRgb.r, stage.ldrRgb.g, stage.ldrRgb.b });
        }
        ss << "  Depth " << F3(stage.depth) << "\n";
        ss << "  ViewDir " << V3(stage.viewDirection) << "  SunDir " << V3(stage.sunDirection) << "\n";
        ss << "  SkyViewUV " << V3({ stage.skyViewUV.x, stage.skyViewUV.y, 0.0f })
           << "  TransUV " << V3({ stage.transmittanceUV.x, stage.transmittanceUV.y, 0.0f }) << "\n";
        if (stage.passId == we::runtime::renderer::ForensicPassId::SkyAtmosphere) {
            ss << "  Rayleigh " << V3(stage.rayleighRGB) << "  Mie " << V3(stage.mieRGB)
               << "  Multi " << V3(stage.multiScatterRGB) << "  Sun " << V3(stage.sunRGB) << "\n";
        }
        ss << "  |\n  v\n";
    }
    m_PixelLabel->SetText(ss.str());
}

void RenderInvestigationModal::RefreshShaderPanel() {
    std::ostringstream ss;
    ss << "=== Shader Inspector @ probe ===\n";
    if (m_Snapshot.gpuInvestigation.shaderTraceValid) {
        const auto& t = m_Snapshot.gpuInvestigation.shaderTrace;
        ss << "SkyAtmosphere\n";
        ss << "  ViewDir " << V3(t.viewDirection) << " len=" << F3(t.viewDirectionLength) << "\n";
        ss << "  SunDir " << V3(t.sunDirection) << " dot=" << F3(t.viewSunDot) << "\n";
        ss << "  HeightKm " << F3(t.atmosphereHeightKm) << "\n";
        ss << "  TransmittanceUV " << V3({ t.transmittanceUV.x, t.transmittanceUV.y, 0.0f })
           << " RGB " << V3(t.transmittanceRGB) << "\n";
        ss << "  SkyViewUV " << V3({ t.skyViewUV.x, t.skyViewUV.y, 0.0f })
           << " RGB " << V3(t.skyViewRGB) << "\n";
        ss << "  Rayleigh " << V3(t.rayleighRGB) << "  Mie " << V3(t.mieRGB)
           << "  MultiScatter " << V3(t.multiScatterRGB) << "\n";
        ss << "  SunDisk " << V3(t.sunDiskRGB) << " mask=" << F3(t.sunDiskMask) << "\n";
        ss << "  FinalHDR " << V3(t.finalHdrRGB) << "\n";
    } else {
        ss << "(no shader trace — probe viewport with Ctrl+Click)\n";
    }
  m_ShaderLabel->SetText(ss.str());
}

void RenderInvestigationModal::RefreshFailureBanner() {
    if (!m_Snapshot.firstFailure.detected) {
        m_FailureLabel->SetText("");
        return;
    }
    const auto& f = m_Snapshot.firstFailure;
    std::ostringstream ss;
    ss << "!!! FIRST FAILURE — " << f.passName;
    if (!f.shader.empty()) ss << "  Shader: " << f.shader;
    ss << "\n";
    if (!f.variable.empty()) ss << "Variable: " << f.variable << "\n";
    if (!f.expectedValue.empty()) ss << "Expected: " << f.expectedValue << "\n";
    if (!f.actualValue.empty()) ss << "Actual: " << f.actualValue << "\n";
    if (!f.reason.empty()) ss << "Reason: " << f.reason << "\n";
    if (!f.minimalFix.empty()) ss << "Suggested fix: " << f.minimalFix;
    m_FailureLabel->SetText(ss.str());
}

void RenderInvestigationModal::RefreshGraphPanel() {
    std::ostringstream ss;
    ss << "=== Render Graph ===\n";
    for (const auto& node : m_Snapshot.gpuInvestigation.graphNodes) {
        Color health = { 0.35f, 0.85f, 0.45f, 1.0f };
        if (node.passName == m_Snapshot.firstFailure.passName) health = { 0.95f, 0.25f, 0.25f, 1.0f };
        else if (!node.executed) health = { 0.5f, 0.5f, 0.5f, 1.0f };
        (void)health;
        const char* mark = node.executed ? "[OK]" : "[--]";
        if (m_Snapshot.firstFailure.detected && node.passName == m_Snapshot.firstFailure.passName) mark = "[FAIL]";
        ss << mark << " " << node.passName;
        if (node.executed) ss << " " << F3(static_cast<float>(node.gpuMs)) << "ms";
        if (!node.producedTarget.empty()) ss << " → " << node.producedTarget;
        ss << "\n";
    }
    for (const auto& edge : m_Snapshot.gpuInvestigation.graphEdges) {
        ss << "  " << edge.fromPass << " --(" << edge.viaResource << ")--> " << edge.toPass << "\n";
    }
    m_GraphLabel->SetText(ss.str());
}

void RenderInvestigationModal::RefreshWarnings() {
    if (m_Snapshot.validationWarnings.empty()) {
        m_WarningsLabel->SetText("");
        return;
    }
    std::ostringstream ss;
    ss << "Validation (" << m_Snapshot.validationWarnings.size() << "):\n";
    size_t count = 0;
    for (const auto& w : m_Snapshot.validationWarnings) {
        ss << "• " << w << "\n";
        if (++count >= 8) {
            ss << "… +" << (m_Snapshot.validationWarnings.size() - count) << " more\n";
            break;
        }
    }
    m_WarningsLabel->SetText(ss.str());
}

void RenderInvestigationModal::UpdateFromSnapshot(const we::runtime::renderer::RenderDebuggerFrameSnapshot& snapshot) {
    if (!m_Root) {
        return;
    }
    const bool newFrame = snapshot.frameNumber != m_LastFrameNumber;
    m_Snapshot = snapshot;
    m_LastFrameNumber = snapshot.frameNumber;

    std::ostringstream frame;
    frame << "Frame " << snapshot.frameNumber
          << "  FPS " << F3(snapshot.fps)
          << "  " << F3(static_cast<float>(snapshot.frameTimeMs)) << "ms";
    m_FrameLabel->SetText(frame.str());

    const size_t passCount = snapshot.gpuInvestigation.passValidations.size();
    const bool passListDirty = m_PassButtons.empty()
        || passCount != m_LastPassCount
        || (newFrame && snapshot.firstFailure.detected);
    if (passListDirty) {
        m_LastPassCount = passCount;
        if (m_SelectedPassIndex < 0 && !snapshot.gpuInvestigation.passValidations.empty()) {
            m_SelectedPassIndex = 0;
        }
        RebuildPassList();
        RebuildTargetList();
    } else if (newFrame) {
        const bool previewDirty = m_SelectedPassIndex != m_LastRefreshPassIndex
            || m_SelectedTargetIndex != m_LastRefreshTargetIndex
            || m_ViewMode != m_LastRefreshViewMode;
        if (previewDirty) {
            RefreshTexturePreview();
        }
    }

    if (!newFrame) {
        return;
    }

    RefreshFailureBanner();
    RefreshWarnings();
    RefreshResourcePanel();
    RefreshPixelPanel();
    RefreshShaderPanel();
    RefreshGraphPanel();
    if (!passListDirty) {
        RefreshTexturePreview();
    }
}

Size RenderInvestigationModal::Measure(const Size& availableSize) {
    m_DesiredSize = availableSize;
    return m_DesiredSize;
}

void RenderInvestigationModal::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    if (!m_Root) {
        return;
    }
    m_Root->Measure(Size{ allottedRect.width, allottedRect.height });
    m_Root->Arrange(allottedRect);
}

void RenderInvestigationModal::Paint(PaintContext& context) {
    context.DrawRect(m_Geometry, Color{ 0.04f, 0.04f, 0.05f, 1.0f });
    if (m_Root) {
        m_Root->Paint(context);
    }
}

void RenderInvestigationModal::OnMouseDown(const MouseEvent& /*event*/) {
    // Consume clicks so the editor underneath does not receive them.
}

void RenderInvestigationModal::OnMouseWheel(const MouseEvent& event) {
    const Point p = event.position;
    if (m_PassScroll->GetGeometry().Contains(p)) m_PassScroll->OnMouseWheel(event);
    else if (m_ResourceScroll->GetGeometry().Contains(p)) m_ResourceScroll->OnMouseWheel(event);
    else if (m_BottomScroll->GetGeometry().Contains(p)) m_BottomScroll->OnMouseWheel(event);
    else if (m_TargetScroll->GetGeometry().Contains(p)) m_TargetScroll->OnMouseWheel(event);
    else m_BottomScroll->OnMouseWheel(event);
}

void RenderInvestigationModal::OnKeyDown(const KeyEvent& event) {
    if (event.keycode == SDLK_ESCAPE && m_OnClose) {
        m_OnClose();
    }
}

// ---------------------------------------------------------------------------
// Host
// ---------------------------------------------------------------------------

namespace {
std::weak_ptr<RenderInvestigationModal> g_OpenModal;
}

void RenderInvestigationModalHost::Show() {
    auto* overlay = OverlayManager::Get();
    if (!overlay) return;
    if (auto existing = g_OpenModal.lock()) {
        return;
    }

    auto& debugger = we::runtime::renderer::RenderDebugger::Get();
    debugger.SetWantsGpuReadback(true);

    overlay->CloseAllPopups();
    auto modal = std::make_shared<RenderInvestigationModal>();
    modal->Construct();
    g_OpenModal = modal;
    modal->SetOnClose([]() {
        we::runtime::renderer::RenderDebugger::Get().SetWantsGpuReadback(false);
        we::runtime::renderer::RenderForensics::Get().DisableEditorInvestigationReadback();
        if (auto* o = OverlayManager::Get()) o->CloseTopPopup();
        g_OpenModal.reset();
    });
    overlay->ShowFullscreenPopup(modal);
}

void RenderInvestigationModalHost::Hide() {
    we::runtime::renderer::RenderDebugger::Get().SetWantsGpuReadback(false);
    we::runtime::renderer::RenderForensics::Get().DisableEditorInvestigationReadback();
    if (auto* overlay = OverlayManager::Get()) overlay->CloseAllPopups();
    g_OpenModal.reset();
}

void RenderInvestigationModalHost::Toggle() {
    if (IsVisible()) Hide();
    else Show();
}

bool RenderInvestigationModalHost::IsVisible() {
    return !g_OpenModal.expired();
}

void RenderInvestigationModalHost::UpdateFromSnapshot(
    const we::runtime::renderer::RenderDebuggerFrameSnapshot& snapshot) {
    if (auto modal = g_OpenModal.lock()) {
        modal->UpdateFromSnapshot(snapshot);
    }
}

} // namespace we::UI
