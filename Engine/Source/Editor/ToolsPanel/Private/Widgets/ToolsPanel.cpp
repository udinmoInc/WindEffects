#include "Widgets/ToolsPanel.h"
#include "EditorModeController.h"
#include "EditorToolsRegistry.h"
#include "Widgets/SearchBox.h"
#include "Core/PaintContext.h"
#include "WindEffects/Editor/UI/Theming/ThemeToken.h"
#include "Core/Icon.h"
#include "Core/DPIContext.h"
#include "Core/Animator.h"
#include "Core/Geometry.h"
#include "Core/Logger.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <SDL3/SDL_keyboard.h>

namespace we::programs::editor {

using WindEffects::Editor::UI::Color;
using WindEffects::Editor::UI::KeyEvent;
using WindEffects::Editor::UI::MouseButton;
using WindEffects::Editor::UI::MouseEvent;
using WindEffects::Editor::UI::PaintContext;
using WindEffects::Editor::UI::Point;
using WindEffects::Editor::UI::Rect;
using WindEffects::Editor::UI::Size;
using WindEffects::Editor::UI::ThemeToken;

namespace {
constexpr float kSearchHeight = 28.0f;
constexpr float kIconSize = 24.0f;
constexpr float kToolRowHeight = 30.0f;
constexpr float kSectionHeaderHeight = 26.0f;
constexpr float kPadding = 8.0f;
constexpr float kContextMenuItemHeight = 24.0f;
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

    if (event.keycode >= SDLK_A && event.keycode <= SDLK_Z) {
        shortcut += static_cast<char>('A' + (event.keycode - SDLK_A));
    } else if (event.keycode >= SDLK_0 && event.keycode <= SDLK_9) {
        shortcut += static_cast<char>('0' + (event.keycode - SDLK_0));
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

    m_SearchBox = std::make_shared<WindEffects::Editor::UI::SearchBox>();
    m_SearchBox->SetFillWidth(true);
    m_SearchBox->SetPlaceholder("Search tools...");
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
    const EditorToolMode* activeMode = EditorToolsRegistry::Get().FindMode(GetActiveModeId());
    if (!activeMode || !activeMode->customContent) {
        m_ModeContentWidget.reset();
        m_ModeContentModeId.clear();
        return;
    }

    const std::string activeModeId = GetActiveModeId();
    if (!m_ModeContentWidget || m_ModeContentModeId != activeModeId
        || m_ModeContentSearchText != m_SearchText) {
        m_ModeContentWidget = activeMode->customContent(*activeMode, m_SearchText);
        m_ModeContentModeId = activeModeId;
        m_ModeContentSearchText = m_SearchText;
    }

    if (m_ModeContentWidget) {
        m_ModeContentWidget->Measure(Size{ m_ContentRect.width, m_ContentRect.height });
        m_ModeContentWidget->Arrange(m_ContentRect);
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
    float menuHeight = kContextMenuItemHeight * 2.0f + 8.0f;
    if (tool->onDragStart) menuHeight += kContextMenuItemHeight;
    if (tool->favoritable) menuHeight += kContextMenuItemHeight;

    float menuX = position.x;
    float menuY = position.y;
    if (menuX + menuWidth > m_PanelRect.x + m_PanelRect.width) {
        menuX = m_PanelRect.x + m_PanelRect.width - menuWidth - 4.0f;
    }
    if (menuY + menuHeight > m_PanelRect.y + m_PanelRect.height) {
        menuY = m_PanelRect.y + m_PanelRect.height - menuHeight - 4.0f;
    }

    m_ContextMenuRect = Rect{ menuX, menuY, menuWidth, menuHeight };
    float itemY = menuY + 4.0f;

    auto addItem = [&](const std::string& label, std::function<void()> action) {
        ContextMenuItem item;
        item.label = label;
        item.action = std::move(action);
        item.geometry = Rect{ menuX, itemY, menuWidth, kContextMenuItemHeight };
        m_ContextMenuItems.push_back(std::move(item));
        itemY += kContextMenuItemHeight;
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

    const EditorToolMode* activeMode = EditorToolsRegistry::Get().FindMode(GetActiveModeId());
    const bool useCustomContent = activeMode && activeMode->customContent;

    float y = m_PanelRect.y + kPadding;
    if (!useCustomContent) {
        m_SearchRect = Rect{ m_PanelRect.x + kPadding, y, m_PanelRect.width - kPadding * 2.0f, kSearchHeight };
        m_SearchBox->Measure(Size{ m_SearchRect.width, m_SearchRect.height });
        m_SearchBox->Arrange(m_SearchRect);
        y = m_SearchRect.y + m_SearchRect.height + 4.0f;
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

    float contentY = m_ContentRect.y + kPadding;
    const float contentWidth = m_ContentRect.width - kPadding * 2.0f;
    const std::string activeModeId = GetActiveModeId();

    auto drawToolList = [&](const std::vector<const EditorToolAction*>& tools, float& yPos) {
        for (const auto* tool : tools) {
            ToolHit hit;
            hit.tool = tool;
            hit.favorite = EditorToolsRegistry::Get().IsFavorite(tool->id);
            hit.geometry = Rect{ m_ContentRect.x + kPadding, yPos, contentWidth, kToolRowHeight };
            m_ToolHits.push_back(hit);
            yPos += kToolRowHeight + 2.0f;
        }
    };

    if (!m_SearchText.empty()) {
        SectionHit section;
        section.categoryId = "__search__";
        section.headerRect = Rect{ m_ContentRect.x + kPadding, contentY, contentWidth, kSectionHeaderHeight };
        section.expanded = true;
        m_Sections.push_back(section);
        contentY += kSectionHeaderHeight + 4.0f;
        drawToolList(EditorToolsRegistry::Get().SearchTools(m_SearchText, activeModeId), contentY);
        return;
    }

    auto favorites = EditorToolsRegistry::Get().GetFavoriteTools(activeModeId);
    if (!favorites.empty()) {
        SectionHit section;
        section.categoryId = "__favorites__";
        section.headerRect = Rect{ m_ContentRect.x + kPadding, contentY, contentWidth, kSectionHeaderHeight };
        section.expanded = IsCategoryExpanded("__favorites__", true);
        m_Sections.push_back(section);
        contentY += kSectionHeaderHeight + 4.0f;
        if (section.expanded) drawToolList(favorites, contentY);
    }

    const auto& recentIds = EditorToolsRegistry::Get().GetRecentToolIds();
    std::vector<const EditorToolAction*> recentTools;
    for (const auto& toolId : recentIds) {
        const auto* tool = EditorToolsRegistry::Get().FindTool(toolId);
        if (!tool) continue;
        const auto* category = EditorToolsRegistry::Get().FindCategory(tool->categoryId);
        if (!category || category->modeId != activeModeId) continue;
        recentTools.push_back(tool);
        if (recentTools.size() >= 6) break;
    }
    if (!recentTools.empty()) {
        SectionHit section;
        section.categoryId = "__recent__";
        section.headerRect = Rect{ m_ContentRect.x + kPadding, contentY, contentWidth, kSectionHeaderHeight };
        section.expanded = IsCategoryExpanded("__recent__", true);
        m_Sections.push_back(section);
        contentY += kSectionHeaderHeight + 4.0f;
        if (section.expanded) drawToolList(recentTools, contentY);
    }

    const EditorToolMode* modeForContent = EditorToolsRegistry::Get().FindMode(activeModeId);
    if (modeForContent && modeForContent->customContent) {
        RebuildModeContent();
        return;
    }

    m_ModeContentWidget.reset();
    m_ModeContentModeId.clear();

    for (const auto* category : EditorToolsRegistry::Get().GetCategoriesForMode(activeModeId)) {
        SectionHit section;
        section.categoryId = category->id;
        section.expanded = IsCategoryExpanded(category->id, category->defaultExpanded);
        section.headerRect = Rect{ m_ContentRect.x + kPadding, contentY, contentWidth, kSectionHeaderHeight };
        m_Sections.push_back(section);
        contentY += kSectionHeaderHeight + 4.0f;
        if (section.expanded) {
            drawToolList(EditorToolsRegistry::Get().GetToolsForCategory(category->id), contentY);
        }
    }
}

void ToolsPanel::Tick(float deltaTime) {
    WindEffects::Editor::UI::Animator::Tick(deltaTime);
}

void ToolsPanel::Paint(PaintContext& context) {
    if (m_Geometry.width < 1.0f) return;
    const float uiScale = (std::max)(1.0f, WindEffects::Editor::UI::DPIContext::GetScale());
    const float labelFontSize = 11.0f * uiScale;
    const float shortcutFontSize = 10.0f * uiScale;
    context.DrawRect(m_PanelRect, ThemeColor(ThemeToken::PanelBackground));

    {
        m_SearchBox->Paint(context);

        for (const auto& section : m_Sections) {
            context.DrawRect(section.headerRect, ThemeColor(ThemeToken::HeaderBackground));

            std::string title = section.categoryId;
            if (section.categoryId == "__favorites__") title = "Favorites";
            else if (section.categoryId == "__recent__") title = "Recently Used";
            else if (section.categoryId == "__search__") title = "Search Results";
            else if (const auto* cat = EditorToolsRegistry::Get().FindCategory(section.categoryId)) {
                title = cat->label;
            }

            const char* chevron = section.expanded ? WindEffects::Editor::UI::Icons::ChevronDownName : WindEffects::Editor::UI::Icons::ChevronRightName;
            WindEffects::Editor::UI::IconPainter::DrawIcon(context, chevron,
                Rect{ section.headerRect.x + 4.0f, section.headerRect.y + 5.0f, 16.0f, 16.0f }, ThemeColor(ThemeToken::TextSecondary));
            context.DrawText(title, Point{ section.headerRect.x + 22.0f, section.headerRect.y + 6.0f }, ThemeColor(ThemeToken::TextPrimary), labelFontSize, true);
        }

        for (const auto& toolHit : m_ToolHits) {
            if (!toolHit.tool) continue;
            if (toolHit.hovered) {
                context.DrawRoundedRect(toolHit.geometry, ThemeColor(ThemeToken::HoverBackground), 4.0f);
            }

            WindEffects::Editor::UI::IconPainter::DrawIcon(context, toolHit.tool->iconName,
                Rect{ toolHit.geometry.x + 6.0f, toolHit.geometry.y + 3.0f, kIconSize, kIconSize }, ThemeColor(ThemeToken::TextPrimary));

            context.DrawText(toolHit.tool->label,
                Point{ toolHit.geometry.x + 34.0f, toolHit.geometry.y + 7.0f }, ThemeColor(ThemeToken::TextPrimary), labelFontSize);

            if (!toolHit.tool->shortcut.empty()) {
                const float shortcutWidth = context.GetTextWidth(toolHit.tool->shortcut, shortcutFontSize);
                context.DrawText(toolHit.tool->shortcut,
                    Point{ toolHit.geometry.x + toolHit.geometry.width - shortcutWidth - 28.0f, toolHit.geometry.y + 8.0f },
                    ThemeColor(ThemeToken::TextDisabled), shortcutFontSize);
            }

            const char* starIcon = toolHit.favorite ? WindEffects::Editor::UI::Icons::StarFilledName : WindEffects::Editor::UI::Icons::StarName;
            Color starColor = toolHit.favorite ? ThemeColor(ThemeToken::Warning) : ThemeColor(ThemeToken::TextDisabled);
            WindEffects::Editor::UI::IconPainter::DrawIcon(context, starIcon,
                Rect{ toolHit.geometry.x + toolHit.geometry.width - 22.0f, toolHit.geometry.y + 5.0f, 16.0f, 16.0f },
                starColor);
        }

        if (m_ModeContentWidget) {
            m_ModeContentWidget->Paint(context);
        }
    }

    if (m_ContextMenuOpen) {
        context.DrawShadow(m_ContextMenuRect, Color{ 0.0f, 0.0f, 0.0f, 0.15f }, 4.0f, 12.0f);
        context.DrawRoundedRect(m_ContextMenuRect, ThemeColor(ThemeToken::PanelBackground), 4.0f);
        for (size_t i = 0; i < m_ContextMenuItems.size(); ++i) {
            const auto& item = m_ContextMenuItems[i];
            if (static_cast<int>(i) == m_ContextMenuHovered) {
                context.DrawRect(item.geometry, ThemeColor(ThemeToken::HoverBackground));
            }
            context.DrawText(item.label, Point{ item.geometry.x + 10.0f, item.geometry.y + 6.0f },
                ThemeColor(ThemeToken::TextPrimary), labelFontSize);
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

    if (m_SearchRect.Contains(event.position)) {
        m_SearchBox->OnMouseDown(event);
        return;
    }

    if (auto* section = HitSectionHeader(event.position)) {
        SetCategoryExpanded(section->categoryId, !IsCategoryExpanded(section->categoryId, true));
        return;
    }

    if (auto* toolHit = HitTool(event.position)) {
        const float starX = toolHit->geometry.x + toolHit->geometry.width - 22.0f;
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

    if (m_ModeContentWidget) {
        m_ModeContentWidget->OnMouseDown(event);
    }
}

void ToolsPanel::OnMouseMove(const MouseEvent& event) {
    for (auto& tool : m_ToolHits) {
        tool.hovered = tool.geometry.Contains(event.position);
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

    if (m_ModeContentWidget) {
        m_ModeContentWidget->OnMouseMove(event);
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
