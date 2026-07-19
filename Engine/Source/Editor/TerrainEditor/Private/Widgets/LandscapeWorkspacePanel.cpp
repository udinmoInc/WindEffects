#include "TerrainEditor/Widgets/LandscapeWorkspacePanel.h"
#include "LandscapeWorkspaceInternal.h"
#include "LandscapePanelChrome.h"

#include "KindUI/Core/Animator.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Input/InputEvents.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "Platform/Platform.h"
#include "ViewportEdit/ViewportEditSession.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace we::editor::terrain {
namespace {

using we::runtime::kindui::KeyEvent;
using we::runtime::kindui::MouseButton;
using we::runtime::kindui::MouseEvent;
using we::runtime::kindui::PaintContext;
using we::runtime::kindui::Point;
using we::runtime::kindui::Rect;
using we::runtime::kindui::Size;
namespace Chrome = LandscapePanelChrome;

const char* TabLabel(LandscapeWorkspaceTab tab) {
    switch (tab) {
    case LandscapeWorkspaceTab::Create: return "Create";
    case LandscapeWorkspaceTab::Sculpt: return "Sculpt";
    case LandscapeWorkspaceTab::Paint: return "Paint";
    case LandscapeWorkspaceTab::Manage: return "Manage";
    }
    return "Create";
}

} // namespace

LandscapeWorkspacePanel::LandscapeWorkspacePanel(ILandscapeEditor* editor)
    : m_Editor(editor)
{
    m_TabHover.assign(4, 0.f);
    SyncDefaultTab();
}

LandscapeWorkspacePanel::~LandscapeWorkspacePanel() = default;

void LandscapeWorkspacePanel::RegisterExtension(LandscapeWorkspaceTab tab, ExtensionFactory factory) {
    m_Extensions[static_cast<int>(tab)] = std::move(factory);
    m_NeedsLayout = true;
}

void LandscapeWorkspacePanel::SyncDefaultTab() {
    if (!m_Editor || m_UserSelectedTab) {
        return;
    }
    m_ActiveTab = m_Editor->HasLandscape()
        ? LandscapeWorkspaceTab::Sculpt
        : LandscapeWorkspaceTab::Create;
}

void LandscapeWorkspacePanel::SetActiveTab(LandscapeWorkspaceTab tab) {
    if (m_ActiveTab == tab) {
        return;
    }
    m_ActiveTab = tab;
    m_UserSelectedTab = true;
    m_FocusedField = -1;
    m_EditBuffer.clear();
    m_Scroll.offset = 0.f;
    m_NeedsLayout = true;
}

void LandscapeWorkspacePanel::ActivateSculptTool(runtime_terrain::TerrainBrushOp op) {
    if (!m_Editor) {
        return;
    }
    m_Editor->InstallViewportMode();
    m_Editor->SetBrushOp(op);
    (void)m_Editor->EnsureLandscape();
    if (auto* ve = viewportedit::ViewportEditSession::Editor()) {
        ve->SetActiveMode("Landscape");
        using viewportedit::ViewportToolId;
        ViewportToolId tool = ViewportToolId::LandscapeSculpt;
        switch (op) {
        case runtime_terrain::TerrainBrushOp::Smooth: tool = ViewportToolId::LandscapeSmooth; break;
        case runtime_terrain::TerrainBrushOp::Flatten: tool = ViewportToolId::LandscapeFlatten; break;
        case runtime_terrain::TerrainBrushOp::Noise: tool = ViewportToolId::LandscapeNoise; break;
        case runtime_terrain::TerrainBrushOp::Paint: tool = ViewportToolId::LandscapePaint; break;
        default: break;
        }
        ve->SetActiveTool(tool);
    }
}

Size LandscapeWorkspacePanel::Measure(const Size& availableSize) {
    return availableSize;
}

void LandscapeWorkspacePanel::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    const float pad = Chrome::PanelPad();
    const float tabH = Chrome::TabBarHeight();
    const float footerH = Chrome::PrimaryButtonHeight() + pad * 2.f;

    m_TabBarRect = {allottedRect.x, allottedRect.y, allottedRect.width, tabH};
    m_FooterRect = {
        allottedRect.x,
        allottedRect.y + allottedRect.height - footerH,
        allottedRect.width,
        footerH};
    m_ContentRect = {
        allottedRect.x,
        allottedRect.y + tabH,
        allottedRect.width,
        std::max(0.f, allottedRect.height - tabH - footerH)};

    m_TabRects.clear();
    const float tabW = allottedRect.width * 0.25f;
    for (int i = 0; i < 4; ++i) {
        m_TabRects.push_back(Rect{allottedRect.x + tabW * static_cast<float>(i), allottedRect.y, tabW, tabH});
    }

    m_PrimaryButtonRect = {
        m_FooterRect.x + pad,
        m_FooterRect.y + pad,
        m_FooterRect.width - pad * 2.f,
        Chrome::PrimaryButtonHeight()};

    m_NeedsLayout = true;
    RebuildLayout();
}

void LandscapeWorkspacePanel::RebuildLayout() {
    if (!m_Editor) {
        m_Hits.clear();
        m_ContentHeight = 0.f;
        return;
    }

    SyncDefaultTab();

    const float pad = Chrome::PanelPad();
    const float scrollY = m_Scroll.offset;
    const float originY = m_ContentRect.y + pad - scrollY;
    LandscapeLayoutBuilder builder;
    builder.Reset(m_ContentRect.x + pad, originY, m_ContentRect.width - pad * 2.f);

    switch (m_ActiveTab) {
    case LandscapeWorkspaceTab::Create:
        BuildCreateTab(builder, *m_Editor);
        break;
    case LandscapeWorkspaceTab::Sculpt:
        BuildSculptTab(builder, *m_Editor);
        break;
    case LandscapeWorkspaceTab::Paint:
        BuildPaintTab(builder, *m_Editor);
        break;
    case LandscapeWorkspaceTab::Manage:
        BuildManageTab(builder, *m_Editor, m_ImportPath, m_ExportPath, m_ResizeX, m_ResizeY);
        break;
    }

    m_ContentHeight = std::max(0.f, builder.cursorY - originY + pad);

    m_Hits.clear();
    m_Hits.reserve(builder.targets.size());
    for (auto& src : builder.targets) {
        HitTarget hit{};
        hit.kind = static_cast<std::uint8_t>(src.kind);
        hit.geometry = src.geometry;
        hit.id = src.id;
        hit.onClick = std::move(src.onClick);
        hit.onTextCommit = std::move(src.onTextCommit);
        hit.selected = src.selected;
        hit.toggled = src.toggled;
        hit.isDanger = src.isDanger;
        hit.isPrimary = src.isPrimary;
        hit.editable = src.editable;
        hit.value = std::move(src.value);
        hit.label = std::move(src.label);
        hit.iconHook = src.iconHook;
        m_Hits.push_back(std::move(hit));
    }

    m_Scroll.Sync(m_ContentRect.height, m_ContentHeight);
    m_NeedsLayout = false;
}

int LandscapeWorkspacePanel::HitTest(const Point& p) const {
    for (int i = static_cast<int>(m_Hits.size()) - 1; i >= 0; --i) {
        const auto& hit = m_Hits[static_cast<size_t>(i)];
        const auto kind = static_cast<LandscapeHitKind>(hit.kind);
        if (kind == LandscapeHitKind::None && hit.id != -2) {
            continue;
        }
        if (hit.geometry.Contains(p)) {
            // Info rows are not interactive.
            if (kind == LandscapeHitKind::None && hit.id == -2) {
                continue;
            }
            if (kind == LandscapeHitKind::None && hit.id == -1) {
                continue;
            }
            return i;
        }
    }
    return -1;
}

void LandscapeWorkspacePanel::Paint(PaintContext& context) {
    if (m_NeedsLayout) {
        RebuildLayout();
    }

    Chrome::PaintPanelBackground(context, m_Geometry);
    Chrome::PaintTabBar(context, m_TabBarRect);

    for (int i = 0; i < 4; ++i) {
        const auto tab = static_cast<LandscapeWorkspaceTab>(i);
        Chrome::PaintTab(
            context,
            m_TabRects[static_cast<size_t>(i)],
            TabLabel(tab),
            m_ActiveTab == tab,
            m_TabHover[static_cast<size_t>(i)]);
    }

    context.PushClipRect(m_ContentRect);

    // Section card behind scrollable content
    const float pad = Chrome::PanelPad();
    if (m_ContentRect.height > pad * 2.f) {
        Chrome::PaintSectionCard(
            context,
            Rect{
                m_ContentRect.x + pad * 0.5f,
                m_ContentRect.y + pad * 0.5f,
                m_ContentRect.width - pad,
                std::min(m_ContentRect.height - pad, m_ContentHeight + pad)});
    }

    for (int i = 0; i < static_cast<int>(m_Hits.size()); ++i) {
        auto& hit = m_Hits[static_cast<size_t>(i)];
        const auto kind = static_cast<LandscapeHitKind>(hit.kind);
        const bool hovered = (i == m_HoveredHit);
        const bool focused = (i == m_FocusedField);

        switch (kind) {
        case LandscapeHitKind::None:
            if (hit.id == -1) {
                Chrome::PaintSectionTitle(context, hit.geometry, hit.label);
            } else if (hit.id == -2) {
                Chrome::PaintInfoValue(context, hit.geometry, hit.label, hit.value);
            }
            break;
        case LandscapeHitKind::Chip:
            Chrome::PaintChip(context, hit.geometry, hit.label, hit.iconHook, hit.selected, hit.hoverAnim);
            break;
        case LandscapeHitKind::Field: {
            Chrome::PaintPropertyLabel(
                context,
                Rect{
                    m_ContentRect.x + pad,
                    hit.geometry.y,
                    Chrome::LabelColumnWidth(),
                    hit.geometry.height},
                hit.label);
            const std::string display = focused ? m_EditBuffer : hit.value;
            Chrome::PaintField(context, hit.geometry, display, focused, hovered);
            break;
        }
        case LandscapeHitKind::Toggle:
            Chrome::PaintToggle(context, hit.geometry, hit.label, hit.toggled, hit.hoverAnim);
            break;
        case LandscapeHitKind::Button:
            if (hit.isDanger) {
                Chrome::PaintDangerButton(context, hit.geometry, hit.label, hit.hoverAnim, hit.pressAnim);
            } else {
                Chrome::PaintSecondaryButton(context, hit.geometry, hit.label, hit.hoverAnim, hit.pressAnim);
            }
            break;
        case LandscapeHitKind::LayerRow: {
            Chrome::PaintChip(
                context,
                hit.geometry,
                hit.label.empty() ? "Layer" : hit.label,
                "layers",
                hit.selected,
                hit.hoverAnim);
            break;
        }
        default:
            break;
        }
    }

    const float uiScale = std::max(1.f, we::runtime::kindui::DPIContext::GetScale());
    const auto metrics = m_Scroll.ComputeMetrics(m_ContentRect, m_ContentHeight, uiScale);
    m_Scroll.Paint(context, metrics, m_Scroll.IsThumbHovered());
    context.PopClipRect();

    // Sticky footer — Create CTA only on Create tab; otherwise subtle separator
    context.DrawRect(m_FooterRect, we::runtime::kindui::ResolveColor(we::runtime::kindui::ColorToken::PanelBackground));
    Chrome::PaintSoftSeparator(
        context,
        Rect{m_FooterRect.x, m_FooterRect.y, m_FooterRect.width, 1.f});

    if (m_ActiveTab == LandscapeWorkspaceTab::Create && m_Editor) {
        const char* label = m_Editor->HasLandscape() ? "Create Additional Landscape" : "Create Landscape";
        Chrome::PaintPrimaryButton(context, m_PrimaryButtonRect, label, m_PrimaryHover, m_PrimaryPress);
    }
}

void LandscapeWorkspacePanel::Tick(float deltaTime) {
    we::runtime::kindui::Animator::Tick(deltaTime);
    constexpr float speed = 18.f;

    for (int i = 0; i < static_cast<int>(m_Hits.size()); ++i) {
        auto& hit = m_Hits[static_cast<size_t>(i)];
        const float hoverTarget = (i == m_HoveredHit) ? 1.f : 0.f;
        const float pressTarget = (i == m_PressedHit) ? 1.f : 0.f;
        hit.hoverAnim = we::runtime::kindui::Animator::Damp(hit.hoverAnim, hoverTarget, speed, deltaTime);
        hit.pressAnim = we::runtime::kindui::Animator::Damp(hit.pressAnim, pressTarget, speed, deltaTime);
    }

    m_PrimaryHover = we::runtime::kindui::Animator::Damp(
        m_PrimaryHover, m_PrimaryHovered ? 1.f : 0.f, speed, deltaTime);
    m_PrimaryPress = we::runtime::kindui::Animator::Damp(
        m_PrimaryPress, m_PrimaryPressed ? 1.f : 0.f, speed, deltaTime);

    if (m_NeedsLayout) {
        RebuildLayout();
        InvalidatePaint();
    }
}

void LandscapeWorkspacePanel::OnMouseDown(const MouseEvent& event) {
    if (event.button != MouseButton::Left) {
        return;
    }

    const float uiScale = std::max(1.f, we::runtime::kindui::DPIContext::GetScale());
    const auto metrics = m_Scroll.ComputeMetrics(m_ContentRect, m_ContentHeight, uiScale);
    if (m_Scroll.OnMouseDown(event, metrics, m_ContentRect.height, m_ContentHeight)) {
        InvalidatePaint();
        return;
    }

    for (int i = 0; i < 4; ++i) {
        if (m_TabRects[static_cast<size_t>(i)].Contains(event.position)) {
            SetActiveTab(static_cast<LandscapeWorkspaceTab>(i));
            InvalidatePaint();
            return;
        }
    }

    if (m_ActiveTab == LandscapeWorkspaceTab::Create && m_PrimaryButtonRect.Contains(event.position)) {
        m_PrimaryPressed = true;
        InvalidatePaint();
        return;
    }

    const int hit = HitTest(event.position);
    m_PressedHit = hit;
    if (hit >= 0) {
        auto& target = m_Hits[static_cast<size_t>(hit)];
        if (static_cast<LandscapeHitKind>(target.kind) == LandscapeHitKind::Field && target.editable) {
            if (m_FocusedField >= 0 && m_FocusedField != hit
                && m_FocusedField < static_cast<int>(m_Hits.size()))
            {
                auto& prev = m_Hits[static_cast<size_t>(m_FocusedField)];
                if (prev.onTextCommit) {
                    prev.onTextCommit(m_EditBuffer);
                }
            }
            m_FocusedField = hit;
            m_EditBuffer = target.value;
        } else if (m_FocusedField >= 0) {
            if (m_FocusedField < static_cast<int>(m_Hits.size())) {
                auto& prev = m_Hits[static_cast<size_t>(m_FocusedField)];
                if (prev.onTextCommit) {
                    prev.onTextCommit(m_EditBuffer);
                }
            }
            m_FocusedField = -1;
            m_EditBuffer.clear();
            m_NeedsLayout = true;
        }
    } else if (m_FocusedField >= 0) {
        if (m_FocusedField < static_cast<int>(m_Hits.size())) {
            auto& prev = m_Hits[static_cast<size_t>(m_FocusedField)];
            if (prev.onTextCommit) {
                prev.onTextCommit(m_EditBuffer);
            }
        }
        m_FocusedField = -1;
        m_EditBuffer.clear();
        m_NeedsLayout = true;
    }
    InvalidatePaint();
}

void LandscapeWorkspacePanel::OnMouseMove(const MouseEvent& event) {
    const float uiScale = std::max(1.f, we::runtime::kindui::DPIContext::GetScale());
    const auto metrics = m_Scroll.ComputeMetrics(m_ContentRect, m_ContentHeight, uiScale);
    m_Scroll.OnMouseMove(event, metrics, m_ContentRect.height, m_ContentHeight);

    for (int i = 0; i < 4; ++i) {
        m_TabHover[static_cast<size_t>(i)] =
            m_TabRects[static_cast<size_t>(i)].Contains(event.position) ? 1.f : 0.f;
    }

    m_PrimaryHovered = m_ActiveTab == LandscapeWorkspaceTab::Create
        && m_PrimaryButtonRect.Contains(event.position);
    m_HoveredHit = HitTest(event.position);
    InvalidatePaint();
}

void LandscapeWorkspacePanel::OnMouseUp(const MouseEvent& event) {
    if (event.button != MouseButton::Left) {
        return;
    }

    m_Scroll.OnMouseUp(event);

    if (m_PrimaryPressed) {
        m_PrimaryPressed = false;
        if (m_PrimaryButtonRect.Contains(event.position) && m_Editor) {
            m_Editor->Wizard().State() = m_Editor->Dialog();
            if (m_Editor->CreateFromDialog()) {
                m_UserSelectedTab = true;
                m_ActiveTab = LandscapeWorkspaceTab::Sculpt;
            }
            m_NeedsLayout = true;
        }
        InvalidatePaint();
        return;
    }

    const int hit = HitTest(event.position);
    if (hit >= 0 && hit == m_PressedHit) {
        auto& target = m_Hits[static_cast<size_t>(hit)];
        if (target.onClick) {
            target.onClick();
            m_NeedsLayout = true;
        }
    }
    m_PressedHit = -1;
    InvalidatePaint();
}

void LandscapeWorkspacePanel::OnMouseWheel(const MouseEvent& event) {
    if (!m_ContentRect.Contains(event.position)) {
        return;
    }
    const float uiScale = std::max(1.f, we::runtime::kindui::DPIContext::GetScale());
    m_Scroll.ApplyWheel(event.wheelDeltaY, 40.f * uiScale, m_ContentRect.height, m_ContentHeight);
    m_NeedsLayout = true;
    InvalidatePaint();
}

void LandscapeWorkspacePanel::OnKeyDown(const KeyEvent& event) {
    using we::platform::KeyCode;
    if (m_FocusedField < 0 || m_FocusedField >= static_cast<int>(m_Hits.size())) {
        return;
    }

    if (event.key == KeyCode::Enter || event.key == KeyCode::NumpadEnter) {
        auto& field = m_Hits[static_cast<size_t>(m_FocusedField)];
        if (field.onTextCommit) {
            field.onTextCommit(m_EditBuffer);
        }
        m_FocusedField = -1;
        m_EditBuffer.clear();
        m_NeedsLayout = true;
        InvalidatePaint();
        return;
    }
    if (event.key == KeyCode::Escape) {
        m_FocusedField = -1;
        m_EditBuffer.clear();
        InvalidatePaint();
        return;
    }
    if (event.key == KeyCode::Backspace) {
        if (!m_EditBuffer.empty()) {
            m_EditBuffer.pop_back();
            InvalidatePaint();
        }
        return;
    }

    // Character input via printable keys A-Z / 0-9 / period / minus / slash / underscore
    char ch = 0;
    if (event.key >= KeyCode::A && event.key <= KeyCode::Z) {
        ch = static_cast<char>('a' + (static_cast<int>(event.key) - static_cast<int>(KeyCode::A)));
        if (event.shiftDown) {
            ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        }
    } else if (event.key >= KeyCode::Num0 && event.key <= KeyCode::Num9) {
        ch = static_cast<char>('0' + (static_cast<int>(event.key) - static_cast<int>(KeyCode::Num0)));
    } else if (event.key == KeyCode::Period || event.key == KeyCode::NumpadDecimal) {
        ch = '.';
    } else if (event.key == KeyCode::Minus || event.key == KeyCode::NumpadSubtract) {
        ch = '-';
    } else if (event.key == KeyCode::Slash) {
        ch = '/';
    } else if (event.key == KeyCode::Space) {
        ch = ' ';
    }
    if (ch != 0) {
        m_EditBuffer.push_back(ch);
        InvalidatePaint();
    }
}

bool LandscapeWorkspacePanel::ShowsPointerCursor(const Point& position) const {
    if (m_PrimaryButtonRect.Contains(position) && m_ActiveTab == LandscapeWorkspaceTab::Create) {
        return true;
    }
    for (const auto& tab : m_TabRects) {
        if (tab.Contains(position)) {
            return true;
        }
    }
    return HitTest(position) >= 0;
}

} // namespace we::editor::terrain
