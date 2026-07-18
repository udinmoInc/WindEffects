#include "Platform/Platform.h"
#include "Widgets/ToolsPanel.h"
#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "WindEffects/Editor/UI/Shell/EditorModeController.h"
#include "WindEffects/Editor/UI/Shell/EditorToolsRegistry.h"
#include "ContentBrowser/Widgets/SearchBox.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Rendering/IconMetrics.h"
#include "KindUI/Core/Animator.h"
#include "KindUI/Core/Geometry.h"
#include "Core/Logger.h"

#include <algorithm>
#include <cmath>
#include <cctype>

namespace we::programs::editor {
using ::we::runtime::kindui::DPIContext;
using ::we::runtime::kindui::IconPainter;
using ::we::runtime::kindui::Animator;
namespace Icons = ::we::runtime::kindui::Icons;
namespace IconMetrics = ::we::runtime::kindui::IconMetrics;


using ::we::editor::shell::EditorModeController;
using ::we::editor::toolspanel::EditorToolsRegistry;
using ::we::editor::toolspanel::EditorToolMode;
using ::we::editor::toolspanel::EditorToolCategory;
using ::we::editor::toolspanel::EditorToolAction;

using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::KeyEvent;
using ::we::runtime::kindui::MouseButton;
using ::we::runtime::kindui::MouseEvent;
using ::we::runtime::kindui::PaintContext;
using ::we::runtime::kindui::Point;
using ::we::runtime::kindui::Rect;
using ::we::runtime::kindui::Size;
using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;
using ::we::editor::widgets::SearchBox;
namespace PanelChrome = ::we::editor::panels::PanelChrome;

namespace {
constexpr float kDragThreshold = 6.0f;

std::string ToUpper(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return value;
}

std::string BuildShortcutFromKeyEvent(const KeyEvent& event) {
    std::string shortcut;
    if (event.ctrlDown) shortcut += "Ctrl+";
    if (event.altDown) shortcut += "Alt+";
    if (event.shiftDown) shortcut += "Shift+";

    if (event.key >= we::platform::KeyCode::A && event.key <= we::platform::KeyCode::Z) {
        shortcut += static_cast<char>('A' + (static_cast<int>(event.key) - static_cast<int>(we::platform::KeyCode::A)));
    } else if (event.key >= we::platform::KeyCode::Num0 && event.key <= we::platform::KeyCode::Num9) {
        shortcut += static_cast<char>('0' + (static_cast<int>(event.key) - static_cast<int>(we::platform::KeyCode::Num0)));
    } else {
        return {};
    }
    return shortcut;
}

bool ShortcutMatches(const std::string& toolShortcut, const KeyEvent& event) {
    if (toolShortcut.empty()) return false;
    const std::string pressed = BuildShortcutFromKeyEvent(event);
    if (pressed.empty()) return false;
    return ToUpper(toolShortcut) == ToUpper(pressed);
}
} // namespace

ToolsPanel::ToolsPanel() {
    m_State.Load();

    m_SearchBox = std::make_shared<::we::editor::widgets::SearchBox>();
    m_SearchBox->SetFillWidth(true);
    m_SearchBox->SetPlaceholder("Search Actors...");
    m_SearchBox->SetOnTextChanged([this](const std::string& text) {
        m_SearchText = text;
        RebuildLayout();
    });
}

ToolsPanel::~ToolsPanel() {
    SaveState();
}

void ToolsPanel::InitializeFromRegistry() {
    EditorModeController::Get().AddModeChangedListener([this](const std::string&) {
        OnModeChanged();
    });
    RebuildLayout();
    HE_INFO("[ToolsPanel] Mode tools panel ready.");
}

void ToolsPanel::OnModeChanged() {
    m_SearchText.clear();
    if (m_SearchBox) m_SearchBox->SetText("");
    m_ModeContentWidget.reset();
    m_ModeContentModeId.clear();
    m_ModeContentSearchText.clear();
    CloseContextMenu();
    RebuildLayout();
}

std::string ToolsPanel::GetActiveModeId() const {
    return EditorModeController::Get().GetActiveModeId();
}

std::string ToolsPanel::GetDrawerContentModeId() const {
    const std::string activeModeId = GetActiveModeId();
    const EditorToolMode* activeMode = EditorToolsRegistry::Get().FindMode(activeModeId);
    if (activeMode && !activeMode->opensToolDrawerByDefault) {
        if (const EditorToolMode* actors = EditorToolsRegistry::Get().FindMode("Actors")) {
            if (actors->customContent) {
                return "Actors";
            }
        }
    }
    return activeModeId;
}

void ToolsPanel::SaveState() const {
    m_State.Save();
}

bool ToolsPanel::IsCategoryExpanded(const std::string& categoryId, bool defaultExpanded) const {
    auto it = m_State.categoryExpanded.find(categoryId);
    if (it == m_State.categoryExpanded.end()) return defaultExpanded;
    return it->second;
}

void ToolsPanel::SetCategoryExpanded(const std::string& categoryId, bool expanded) {
    m_State.categoryExpanded[categoryId] = expanded;
    RebuildLayout();
    SaveState();
}

void ToolsPanel::ExecuteTool(const EditorToolAction* tool) {
    if (!tool) return;
    CloseContextMenu();
    EditorToolsRegistry::Get().RecordToolUsage(tool->id);
    if (tool->onExecute) tool->onExecute();
    RebuildLayout();
}

void ToolsPanel::RebuildModeContent() {
    const std::string contentModeId = GetDrawerContentModeId();
    const EditorToolMode* contentMode = EditorToolsRegistry::Get().FindMode(contentModeId);
    if (!contentMode || !contentMode->customContent) {
        m_ModeContentWidget.reset();
        m_ModeContentModeId.clear();
        return;
    }

    if (!m_ModeContentWidget || m_ModeContentModeId != contentModeId
        || m_ModeContentSearchText != m_SearchText) {
        m_ModeContentWidget = contentMode->customContent(*contentMode, m_SearchText);
        m_ModeContentModeId = contentModeId;
        m_ModeContentSearchText = m_SearchText;
    }

    if (m_ModeContentWidget) {
        m_ModeContentWidget->Measure(Size{ m_PanelRect.width, m_PanelRect.height });
        m_ModeContentWidget->Arrange(m_PanelRect);
    }
}

void ToolsPanel::CloseContextMenu() {
    m_ContextMenuOpen = false;
    m_ContextMenuItems.clear();
    m_ContextMenuHovered = -1;
}

void ToolsPanel::ShowToolContextMenu(const EditorToolAction* tool, const Point& position) {
    if (!tool) return;

    CloseContextMenu();
    m_ContextMenuOpen = true;

    const float menuWidth = 168.0f;
    float menuHeight = PanelChrome::ListRowHeight() * 2.0f + ThemeMetric(MetricToken::Space1) * 2.0f;
    if (tool->onDragStart) menuHeight += PanelChrome::ListRowHeight();
    if (tool->favoritable) menuHeight += PanelChrome::ListRowHeight();

    float menuX = position.x;
    float menuY = position.y;
    if (menuX + menuWidth > m_PanelRect.x + m_PanelRect.width) {
        menuX = m_PanelRect.x + m_PanelRect.width - menuWidth - 4.0f;
    }
    if (menuY + menuHeight > m_PanelRect.y + m_PanelRect.height) {
        menuY = m_PanelRect.y + m_PanelRect.height - menuHeight - 4.0f;
    }

    m_ContextMenuRect = Rect{ menuX, menuY, menuWidth, menuHeight };
    float itemY = menuY + ThemeMetric(MetricToken::Space1);

    auto addItem = [&](const std::string& label, std::function<void()> action) {
        ContextMenuItem item;
        item.label = label;
        item.action = std::move(action);
        item.geometry = Rect{ menuX, itemY, menuWidth, PanelChrome::ListRowHeight() };
        m_ContextMenuItems.push_back(std::move(item));
        itemY += PanelChrome::ListRowHeight();
    };

    addItem("Execute", [this, tool]() { ExecuteTool(tool); });
    if (tool->favoritable) {
        const bool favorite = EditorToolsRegistry::Get().IsFavorite(tool->id);
        addItem(favorite ? "Remove Favorite" : "Add Favorite", [this, tool]() {
            EditorToolsRegistry::Get().ToggleFavorite(tool->id);
            RebuildLayout();
        });
    }
    if (tool->onDragStart) {
        addItem("Drag to Viewport", [tool]() {
            if (tool->onDragStart) tool->onDragStart();
        });
    }
}

bool ToolsPanel::HandleShortcut(const KeyEvent& event) {
    auto tryExecute = [&](const std::vector<const EditorToolAction*>& tools) -> bool {
        for (const auto* tool : tools) {
            if (ShortcutMatches(tool->shortcut, event)) {
                ExecuteTool(tool);
                return true;
            }
        }
        return false;
    };

    const std::string activeModeId = GetActiveModeId();
    if (!m_SearchText.empty()) {
        if (tryExecute(EditorToolsRegistry::Get().SearchTools(m_SearchText, activeModeId))) {
            return true;
        }
    }

    auto categories = EditorToolsRegistry::Get().GetCategoriesForMode(activeModeId);
    for (const auto* category : categories) {
        if (tryExecute(EditorToolsRegistry::Get().GetToolsForCategory(category->id))) {
            return true;
        }
    }
    return false;
}

Size ToolsPanel::Measure(const Size& availableSize) {
    m_DesiredSize = availableSize;
    return m_DesiredSize;
}

void ToolsPanel::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    RebuildLayout();
}

void ToolsPanel::RebuildLayout() {
    const float width = m_Geometry.width;
    if (width < 1.0f) {
        m_PanelRect = {};
        m_Sections.clear();
        m_ToolHits.clear();
        return;
    }

    m_PanelRect = Rect{ m_Geometry.x, m_Geometry.y, width, m_Geometry.height };

    const std::string contentModeId = GetDrawerContentModeId();
    const EditorToolMode* contentMode = EditorToolsRegistry::Get().FindMode(contentModeId);
    const bool useCustomContent = contentMode && contentMode->customContent;

    const float padH = PanelChrome::PanelPaddingH();
    const float searchH = PanelChrome::SearchHeight();
    const float searchRowH = searchH + ThemeMetric(MetricToken::Space1);
    const float rowH = PanelChrome::ListRowHeight();
    const float sectionH = PanelChrome::CategoryHeaderHeight();
    const float iconSize = ThemeMetric(MetricToken::IconSizeTree);
    const float sectionGap = ThemeMetric(MetricToken::Space2);

    float y = m_PanelRect.y + padH;
    if (!useCustomContent) {
        m_SearchRect = Rect{ m_PanelRect.x + padH, y, m_PanelRect.width - padH * 2.0f, searchH };
        m_SearchBox->Measure(Size{ m_SearchRect.width, m_SearchRect.height });
        m_SearchBox->Arrange(m_SearchRect);
        y = m_PanelRect.y + searchRowH;
    } else {
        m_SearchRect = {};
        y = m_PanelRect.y;
    }

    m_ContentRect = Rect{
        m_PanelRect.x,
        y,
        m_PanelRect.width,
        (std::max)(0.0f, m_PanelRect.y + m_PanelRect.height - y)
    };

    m_Sections.clear();
    m_ToolHits.clear();

    auto drawToolList = [&](const std::vector<const EditorToolAction*>& tools, float& yPos) {
        for (const auto* tool : tools) {
            ToolHit hit;
            hit.tool = tool;
            hit.favorite = EditorToolsRegistry::Get().IsFavorite(tool->id);
            hit.geometry = Rect{ m_ContentRect.x, yPos, m_ContentRect.width, rowH };
            m_ToolHits.push_back(hit);
            yPos += rowH;
        }
    };

    // Place Actors (and other custom drawer UIs) always win for modes that provide them.
    if (useCustomContent) {
        RebuildModeContent();
        return;
    }

    m_ModeContentWidget.reset();
    m_ModeContentModeId.clear();

    if (!m_SearchText.empty()) {
        SectionHit section;
        section.categoryId = "__search__";
        section.headerRect = Rect{ m_ContentRect.x, m_ContentRect.y, m_ContentRect.width, sectionH };
        section.expanded = true;
        m_Sections.push_back(section);
        float contentY = m_ContentRect.y + sectionH;
        drawToolList(EditorToolsRegistry::Get().SearchTools(m_SearchText, contentModeId), contentY);
        return;
    }

    float contentY = m_ContentRect.y;
    bool firstSection = true;

    auto addSection = [&](const std::string& id, bool defaultExpanded) -> SectionHit* {
        if (!firstSection) {
            contentY += sectionGap;
        }
        firstSection = false;
        SectionHit section;
        section.categoryId = id;
        section.headerRect = Rect{ m_ContentRect.x, contentY, m_ContentRect.width, sectionH };
        section.expanded = IsCategoryExpanded(id, defaultExpanded);
        m_Sections.push_back(section);
        contentY += sectionH;
        return &m_Sections.back();
    };

    auto favorites = EditorToolsRegistry::Get().GetFavoriteTools(contentModeId);
    if (!favorites.empty()) {
        SectionHit* section = addSection("__favorites__", true);
        if (section->expanded) drawToolList(favorites, contentY);
    }

    const auto& recentIds = EditorToolsRegistry::Get().GetRecentToolIds();
    std::vector<const EditorToolAction*> recentTools;
    for (const auto& toolId : recentIds) {
        const auto* tool = EditorToolsRegistry::Get().FindTool(toolId);
        if (!tool) continue;
        const auto* category = EditorToolsRegistry::Get().FindCategory(tool->categoryId);
        if (!category || category->modeId != contentModeId) continue;
        recentTools.push_back(tool);
        if (recentTools.size() >= 6) break;
    }
    if (!recentTools.empty()) {
        SectionHit* section = addSection("__recent__", true);
        if (section->expanded) drawToolList(recentTools, contentY);
    }

    for (const auto* category : EditorToolsRegistry::Get().GetCategoriesForMode(contentModeId)) {
        SectionHit* section = addSection(category->id, category->defaultExpanded);
        if (section->expanded) {
            drawToolList(EditorToolsRegistry::Get().GetToolsForCategory(category->id), contentY);
        }
    }

    (void)iconSize;
}

void ToolsPanel::Tick(float deltaTime) {
    we::runtime::kindui::Animator::Tick(deltaTime);
}

void ToolsPanel::Paint(PaintContext& context) {
    if (m_Geometry.width < 1.0f) return;

    if (m_ModeContentWidget) {
        m_ModeContentWidget->Paint(context);
    } else {
        const float uiScale = (std::max)(1.0f, we::runtime::kindui::DPIContext::GetScale());
        const float labelFontSize = ThemeMetric(MetricToken::TextSizeBody) * uiScale;
        const float shortcutFontSize = ThemeMetric(MetricToken::TextSizeCaption) * uiScale;
        const float iconSize = static_cast<float>(we::runtime::kindui::IconMetrics::NativeIconTierPx(ThemeMetric(MetricToken::IconSizeTree)));
        const float padH = PanelChrome::PanelPaddingH();
        const float chevronGap = ThemeMetric(MetricToken::Space2) * uiScale;

        PanelChrome::PaintContentRegion(context, m_PanelRect);

        if (m_SearchRect.width > 0.0f) {
            m_SearchBox->Paint(context);
        }

        for (const auto& section : m_Sections) {
            std::string title = section.categoryId;
            if (section.categoryId == "__favorites__") title = "Favorites";
            else if (section.categoryId == "__recent__") title = "Recently Used";
            else if (section.categoryId == "__search__") title = "Search Results";
            else if (const auto* cat = EditorToolsRegistry::Get().FindCategory(section.categoryId)) {
                title = cat->label;
            }

            PanelChrome::PaintCategoryHeader(context, section.headerRect, title, section.expanded, false);
        }

        for (const auto& toolHit : m_ToolHits) {
            if (!toolHit.tool) continue;
            if (toolHit.hovered) {
                PanelChrome::PaintListRowBackground(context, toolHit.geometry, true, false);
            }

            const float rowIconX = toolHit.geometry.x + padH + iconSize + chevronGap;
            const float iconY = toolHit.geometry.y + (toolHit.geometry.height - iconSize) * 0.5f;
            we::runtime::kindui::IconPainter::DrawIcon(context, toolHit.tool->iconName,
                Rect{ rowIconX, iconY, iconSize, iconSize }, ThemeColor(ColorToken::IconPrimary));

            context.DrawText(toolHit.tool->label,
                Point{ rowIconX + iconSize + chevronGap, toolHit.geometry.y + (toolHit.geometry.height - labelFontSize) * 0.5f },
                ThemeColor(ColorToken::TextPrimary), labelFontSize);

            if (!toolHit.tool->shortcut.empty()) {
                const float shortcutWidth = context.GetTextWidth(toolHit.tool->shortcut, shortcutFontSize);
                const float starReserve = iconSize + padH * 2.0f;
                context.DrawText(toolHit.tool->shortcut,
                    Point{ toolHit.geometry.x + toolHit.geometry.width - starReserve - shortcutWidth, toolHit.geometry.y + (toolHit.geometry.height - shortcutFontSize) * 0.5f },
                    ThemeColor(ColorToken::TextDisabled), shortcutFontSize);
            }

            const float starX = toolHit.geometry.x + toolHit.geometry.width - padH - iconSize;
            const char* starIcon = toolHit.favorite ? we::runtime::kindui::Icons::StarFilledName : we::runtime::kindui::Icons::StarName;
            Color starColor = toolHit.favorite ? ThemeColor(ColorToken::IconAccent) : ThemeColor(ColorToken::IconSecondary);
            we::runtime::kindui::IconPainter::DrawIcon(context, starIcon,
                Rect{ starX, iconY, iconSize, iconSize }, starColor);
        }
    }

    if (m_ContextMenuOpen) {
        const float uiScale = (std::max)(1.0f, we::runtime::kindui::DPIContext::GetScale());
        const float labelFontSize = ThemeMetric(MetricToken::TextSizeBody) * uiScale;
        const float rowH = PanelChrome::ListRowHeight();
        context.DrawShadow(m_ContextMenuRect, ThemeColor(ColorToken::ShadowPopup), 3.0f, 8.0f);
        context.DrawRoundedRect(m_ContextMenuRect, ThemeColor(ColorToken::PopupBackground), ThemeMetric(MetricToken::CornerRadiusSmall));
        for (size_t i = 0; i < m_ContextMenuItems.size(); ++i) {
            const auto& item = m_ContextMenuItems[i];
            if (static_cast<int>(i) == m_ContextMenuHovered) {
                context.DrawRect(item.geometry, ThemeColor(ColorToken::HoverBackground));
            }
            context.DrawText(item.label,
                Point{ item.geometry.x + PanelChrome::PanelPaddingH(), item.geometry.y + (rowH - labelFontSize) * 0.5f },
                ThemeColor(ColorToken::TextPrimary), labelFontSize);
        }
    }
}

bool ToolsPanel::HitTest(const Point& position) const {
    return m_Geometry.Contains(position);
}

ToolsPanel::SectionHit* ToolsPanel::HitSectionHeader(const Point& p) {
    for (auto& section : m_Sections) {
        if (section.headerRect.Contains(p)) return &section;
    }
    return nullptr;
}

ToolsPanel::ToolHit* ToolsPanel::HitTool(const Point& p) {
    for (auto& tool : m_ToolHits) {
        if (tool.geometry.Contains(p)) return &tool;
    }
    return nullptr;
}

void ToolsPanel::OnMouseDown(const MouseEvent& event) {
    if (m_ContextMenuOpen) {
        for (size_t i = 0; i < m_ContextMenuItems.size(); ++i) {
            if (m_ContextMenuItems[i].geometry.Contains(event.position)) {
                if (m_ContextMenuItems[i].action) m_ContextMenuItems[i].action();
                CloseContextMenu();
                return;
            }
        }
        CloseContextMenu();
        if (!HitTest(event.position)) return;
    }

    if (!HitTest(event.position)) return;

    if (event.button == MouseButton::Right) {
        if (auto* toolHit = HitTool(event.position)) {
            ShowToolContextMenu(toolHit->tool, event.position);
            return;
        }
        CloseContextMenu();
        return;
    }

    if (m_ModeContentWidget && m_ModeContentWidget->GetGeometry().Contains(event.position)) {
        m_ModeContentWidget->OnMouseDown(event);
        return;
    }

    if (m_SearchRect.Contains(event.position)) {
        m_SearchBox->OnMouseDown(event);
        return;
    }

    if (auto* section = HitSectionHeader(event.position)) {
        SetCategoryExpanded(section->categoryId, !IsCategoryExpanded(section->categoryId, true));
        return;
    }

    if (auto* toolHit = HitTool(event.position)) {
        const float iconSize = ThemeMetric(MetricToken::IconSizeTree);
        const float starX = toolHit->geometry.x + toolHit->geometry.width - PanelChrome::PanelPaddingH() - iconSize;
        if (event.position.x >= starX && toolHit->tool && toolHit->tool->favoritable) {
            EditorToolsRegistry::Get().ToggleFavorite(toolHit->tool->id);
            RebuildLayout();
            return;
        }

        if (toolHit->tool && toolHit->tool->onDragStart) {
            m_PendingDragTool = toolHit->tool;
            m_DragStartPosition = event.position;
            m_DragStarted = false;
            return;
        }

        ExecuteTool(toolHit->tool);
        return;
    }
}

void ToolsPanel::OnMouseMove(const MouseEvent& event) {
    if (m_ModeContentWidget) {
        m_ModeContentWidget->OnMouseMove(event);
    } else {
        for (auto& tool : m_ToolHits) {
            tool.hovered = tool.geometry.Contains(event.position);
        }
    }

    if (m_ContextMenuOpen) {
        m_ContextMenuHovered = -1;
        for (size_t i = 0; i < m_ContextMenuItems.size(); ++i) {
            if (m_ContextMenuItems[i].geometry.Contains(event.position)) {
                m_ContextMenuHovered = static_cast<int>(i);
                break;
            }
        }
        return;
    }

    if (m_PendingDragTool && !m_DragStarted) {
        const float dx = event.position.x - m_DragStartPosition.x;
        const float dy = event.position.y - m_DragStartPosition.y;
        if ((dx * dx + dy * dy) >= (kDragThreshold * kDragThreshold)) {
            m_DragStarted = true;
            if (m_PendingDragTool->onDragStart) m_PendingDragTool->onDragStart();
            m_PendingDragTool = nullptr;
        }
    }
}

void ToolsPanel::OnMouseUp(const MouseEvent& event) {
    if (m_PendingDragTool && !m_DragStarted) {
        ExecuteTool(m_PendingDragTool);
    }
    m_PendingDragTool = nullptr;
    m_DragStarted = false;

    if (m_ModeContentWidget) {
        m_ModeContentWidget->OnMouseUp(event);
    }
}

void ToolsPanel::OnKeyDown(const KeyEvent& event) {
    if (HandleShortcut(event)) return;

    if (m_SearchBox) {
        m_SearchBox->OnKeyDown(event);
    }

    if (m_ModeContentWidget) {
        m_ModeContentWidget->OnKeyDown(event);
    }
}

} // namespace we::programs::editor
