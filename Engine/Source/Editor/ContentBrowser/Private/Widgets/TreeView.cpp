#include "Platform/Platform.h"
#include "ContentBrowser/Widgets/TreeView.h"
#include "KindUI/Core/EventSystem.h"
#include "KindUI/Layout/OverlayManager.h"
#include "KindUI/Layout/ScrollViewport.h"
#include "Services/ContentBrowserFolderArt.h"
#include "Services/ContentBrowserBlueprintArt.h"
#include "KindUI/Core/PaintContext.h"
#include "WindEffects/Editor/UI/Panel/PanelChrome.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Input/InputEvents.h"
#include "KindUI/Rendering/IconMetrics.h"
#include <algorithm>
#include <cmath>
#include <functional>

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;
using ::we::runtime::kindui::Point;

namespace we::editor::contentbrowser {
using ::we::runtime::kindui::IconPainter;
using ::we::runtime::kindui::MouseButton;
using ::we::runtime::kindui::KeyCodeToChar;
namespace Icons = ::we::runtime::kindui::Icons;
namespace IconMetrics = ::we::runtime::kindui::IconMetrics;
namespace PanelChrome = ::we::editor::panels::PanelChrome;
using ::we::runtime::kindui::DPIContext;


namespace {

using ::we::editor::contentbrowser::ContentBrowserBlueprintArt;
using ::we::editor::contentbrowser::ContentBrowserFolderArt;

bool IsFolderIconName(const std::string& iconName) {
    return iconName == Icons::FolderName;
}

bool IsBlueprintIconName(const std::string& iconName) {
    return iconName == "blueprint";
}

constexpr float kMinTreeZoom = 0.75f;
constexpr float kMaxTreeZoom = 1.75f;
constexpr float kTreeZoomStep = 0.1f;

float TreeUiScale() {
    return (std::max)(1.0f, DPIContext::GetScale());
}

void PaintTreeNodeIcon(PaintContext& context, const TreeNode& node, const Rect& iconRect, bool hovered) {
    if (node.iconTexture != we::rhi::RHIDescriptorSetHandle::Invalid) {
        context.DrawTexture(iconRect, node.iconTexture);
        return;
    }
    if (IsFolderIconName(node.iconName)) {
        ContentBrowserFolderArt::Get().PaintSmallIcon(context, iconRect, hovered, node.expanded);
        return;
    }
    if (IsBlueprintIconName(node.iconName)) {
        ContentBrowserBlueprintArt::Get().PaintSmallIcon(context, iconRect, hovered);
        return;
    }
    if (!node.iconName.empty()) {
        IconPainter::DrawIcon(context, node.iconName, iconRect, we::runtime::kindui::ResolveColor(ColorToken::IconPrimary));
    }
}

struct TreeMenuItem {
    std::string label;
    std::function<void()> onClick;
    bool enabled = true;
};

class TreeContextMenu : public Widget {
public:
    TreeContextMenu(std::vector<TreeMenuItem> items, std::function<void()> onDismiss)
        : m_Items(std::move(items)), m_OnDismiss(std::move(onDismiss)) {}

    Size Measure(const Size& availableSize) override {
        (void)availableSize;
        float maxWidth = 200.0f;
        for (const auto& item : m_Items) {
            maxWidth = std::max(maxWidth, 24.0f + static_cast<float>(item.label.size()) * 7.0f + 24.0f);
        }
        m_DesiredSize = Size{ maxWidth, 6.0f + m_Items.size() * 26.0f };
        return m_DesiredSize;
    }

    void Arrange(const Rect& allottedRect) override { m_Geometry = allottedRect; }

    void Paint(PaintContext& context) override {
        context.DrawShadow(m_Geometry, ThemeColor(ColorToken::ShadowSubtle), 4.0f, 10.0f);
        context.DrawRoundedRect(m_Geometry, ThemeColor(ColorToken::PopupBackground), ThemeMetric(MetricToken::CornerRadiusSmall));
        context.DrawRoundedRectOutline(m_Geometry, ThemeColor(ColorToken::BorderDefault), 1.0f, ThemeMetric(MetricToken::CornerRadiusSmall));

        float y = m_Geometry.y + 3.0f;
        for (size_t i = 0; i < m_Items.size(); ++i) {
            const auto& item = m_Items[i];
            Rect row{ m_Geometry.x + 2.0f, y, m_Geometry.width - 4.0f, 24.0f };
            if (!item.enabled) {
                y += 26.0f;
                continue;
            }
            if (static_cast<int>(i) == m_Hovered) {
                context.DrawRoundedRect(row, ThemeColor(ColorToken::HoverBackground), 3.0f);
            }
            context.DrawText(item.label, Point{ row.x + 10.0f, row.y + 5.0f }, ThemeColor(ColorToken::TextPrimary), 11.0f);
            y += 26.0f;
        }
    }

    void OnMouseMove(const MouseEvent& event) override {
        m_Hovered = -1;
        float y = m_Geometry.y + 3.0f;
        for (size_t i = 0; i < m_Items.size(); ++i) {
            Rect row{ m_Geometry.x + 2.0f, y, m_Geometry.width - 4.0f, 24.0f };
            if (row.Contains(event.position)) {
                m_Hovered = static_cast<int>(i);
                break;
            }
            y += 26.0f;
        }
    }

    void OnMouseDown(const MouseEvent& event) override {
        if (event.button != MouseButton::Left) {
            return;
        }
        float y = m_Geometry.y + 3.0f;
        for (size_t i = 0; i < m_Items.size(); ++i) {
            Rect row{ m_Geometry.x + 2.0f, y, m_Geometry.width - 4.0f, 24.0f };
            if (row.Contains(event.position) && m_Items[i].enabled && m_Items[i].onClick) {
                m_Items[i].onClick();
                if (auto* overlay = GetPopupHost()) {
                    overlay->CloseAllPopups();
                }
                if (m_OnDismiss) {
                    m_OnDismiss();
                }
                return;
            }
            y += 26.0f;
        }
    }

private:
    std::vector<TreeMenuItem> m_Items;
    std::function<void()> m_OnDismiss;
    int m_Hovered = -1;
};

} // namespace

TreeView::TreeView()
    : m_Style(WidgetStyle::TreeItem())
{
    m_Root = std::make_shared<TreeNode>();
    m_Root->id = "root";
    m_Root->label = "Root";
}

void TreeView::SyncScrollMetrics() {
    m_Scroll.Sync(m_Geometry.height, m_ContentHeight);
    m_ScrollMetrics = m_Scroll.ComputeMetrics(m_Geometry, m_ContentHeight, TreeUiScale());
}

void TreeView::ScrollSelectionIntoView() {
    if (m_SelectedIds.empty()) {
        return;
    }

    const std::string& selectedId = m_SelectedIds.back();
    const float rowHeight = m_ItemHeight * TreeUiScale();
    for (const auto& item : m_RenderList) {
        if (item.node->id != selectedId) {
            continue;
        }
        const float top = static_cast<float>(item.flatIndex) * rowHeight;
        const float bottom = top + rowHeight;
        if (m_Scroll.ScrollToRange(top, bottom, m_Geometry.height, m_ContentHeight)) {
            Arrange(m_Geometry);
        }
        break;
    }
}

Size TreeView::Measure(const Size& availableSize) {
    if (m_RenderListDirty) {
        BuildRenderList();
    }
    m_ContentHeight = static_cast<float>(m_RenderList.size()) * m_ItemHeight * TreeUiScale();
    return Size{ availableSize.width, availableSize.height };
}

void TreeView::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    if (m_RenderListDirty) {
        BuildRenderList();
    }
    SyncScrollMetrics();
    UpdateVisibleRange();

    const float rowHeight = m_ItemHeight * TreeUiScale();
    const float viewportX = m_ScrollMetrics.viewport.x;
    const float viewportWidth = m_ScrollMetrics.viewport.width;
    float y = m_ScrollMetrics.viewport.y - m_Scroll.offset;
    for (size_t i = 0; i < m_RenderList.size(); ++i) {
        auto& item = m_RenderList[i];
        item.flatIndex = static_cast<int>(i);
        item.geometry = Rect{
            viewportX + item.depth * m_IndentWidth,
            y,
            std::max(0.0f, viewportWidth - item.depth * m_IndentWidth),
            rowHeight
        };
        y += rowHeight;
    }
}

void TreeView::Tick(float deltaTime) {
    Widget::Tick(deltaTime);
    if (!m_RenamingId.empty()) {
        m_RenameCursorBlink += deltaTime;
    }
}

void TreeView::Paint(PaintContext& context) {
    ::we::editor::panels::PanelChrome::PaintContentRegion(context, m_Geometry);

    if (m_RenderList.empty()) {
        return;
    }

    SyncScrollMetrics();
    UpdateVisibleRange();

    const float fontSize = m_Style.text.size * TreeUiScale();
    const float viewTop = m_ScrollMetrics.viewport.y;
    const float viewBottom = m_ScrollMetrics.viewport.y + m_ScrollMetrics.viewport.height;

    context.PushClipRect(m_ScrollMetrics.viewport);

    for (int i = m_FirstVisibleIndex; i <= m_LastVisibleIndex && i < static_cast<int>(m_RenderList.size()); ++i) {
        const auto& item = m_RenderList[static_cast<size_t>(i)];
        const auto& node = item.node;
        const float rowHeight = item.geometry.height;

        if (item.geometry.y + item.geometry.height < viewTop ||
            item.geometry.y > viewBottom) {
            continue;
        }

        Color bgColor{0, 0, 0, 0};
        const bool selected = IsSelected(node->id);
        const bool hovered = node->id == m_HoveredId;
        if (selected || hovered) {
            Rect rowRect = item.geometry;
            rowRect.x = m_ScrollMetrics.viewport.x;
            rowRect.width = m_ScrollMetrics.viewport.width;
            ::we::editor::panels::PanelChrome::PaintListRowBackground(context, rowRect, hovered, selected);
        }

        if (node->id == m_DropTargetId && m_Dragging) {
            Rect dropLine{
                m_ScrollMetrics.viewport.x + 4.0f,
                item.geometry.y,
                m_ScrollMetrics.viewport.width - 8.0f,
                2.0f
            };
            context.DrawRect(dropLine, ThemeColor(ColorToken::AccentPrimary));
        }

        if (!node->children.empty()) {
            const float tier = static_cast<float>(IconMetrics::StandardGlyphTierPx());
            const float chevronX = item.geometry.x + ThemeMetric(MetricToken::Space1) * TreeUiScale();
            const float centerY = item.geometry.y + rowHeight * 0.5f;
            const char* chevronIcon = node->expanded ? Icons::ChevronDownName : Icons::ChevronRightName;
            Rect chevronControl{ chevronX, centerY - tier * 0.5f, tier, tier };
            IconPainter::DrawIcon(context, chevronIcon,
                IconMetrics::PlaceGlyphCentered(chevronControl, tier),
                ThemeColor(ColorToken::TextSecondary));
        }

        const float iconSize = static_cast<float>(IconMetrics::GlyphTierPx(MetricToken::IconSizeTree));
        const float iconX = item.geometry.x + ThemeMetric(MetricToken::Space2)
            + static_cast<float>(IconMetrics::StandardGlyphTierPx())
            + ThemeMetric(MetricToken::Space1) * TreeUiScale();
        if (!node->iconName.empty() || node->iconTexture != we::rhi::RHIDescriptorSetHandle::Invalid) {
            Rect iconBand{ iconX, item.geometry.y, iconSize, rowHeight };
            Rect iconRect = IconMetrics::PlaceGlyphCentered(iconBand, iconSize);
            PaintTreeNodeIcon(context, *node, iconRect, node->id == m_HoveredId);
        }

        const float textX = iconX + iconSize + ThemeMetric(MetricToken::Space2) * TreeUiScale();
        const float textY = item.geometry.y + (rowHeight - fontSize) * 0.5f;
        Color textColor = node->locked ? ThemeColor(ColorToken::TextSecondary) * 0.6f : ThemeColor(ColorToken::TextPrimary);
        if (!node->visible) {
            textColor = ThemeColor(ColorToken::TextSecondary) * 0.45f;
        }

        if (node->id == m_RenamingId) {
            Rect editBg{ textX - 4.0f, item.geometry.y + 2.0f, item.geometry.width - 80.0f, rowHeight - 4.0f };
            context.DrawRoundedRect(editBg, ThemeColor(ColorToken::InputBackground), 3.0f);
            context.DrawRoundedRectOutline(editBg, ThemeColor(ColorToken::AccentPrimary), 1.0f, 3.0f);
            context.DrawText(m_RenameBuffer, Point{ textX, textY }, ThemeColor(ColorToken::TextPrimary), fontSize);
            if (static_cast<int>(m_RenameCursorBlink * 2.0f) % 2 == 0) {
                const float cursorX = textX + context.GetTextWidth(m_RenameBuffer, fontSize) + 1.0f;
                context.DrawRect(Rect{ cursorX, textY, 1.0f, fontSize }, ThemeColor(ColorToken::TextPrimary));
            }
        } else {
            // Draw text with search highlighting
            if (!m_SearchQuery.empty()) {
                const std::string& label = node->label;
                const std::string& query = m_SearchQuery;
                
                // Find match positions
                size_t matchStart = 0;
                size_t matchEnd = 0;
                bool foundMatch = false;
                
                // Simple case-insensitive search for highlighting
                for (size_t searchIdx = 0; searchIdx <= label.size() - query.size() && !foundMatch; ++searchIdx) {
                    bool matches = true;
                    for (size_t j = 0; j < query.size(); ++j) {
                        if (std::tolower(label[searchIdx + j]) != std::tolower(query[j])) {
                            matches = false;
                            break;
                        }
                    }
                    if (matches) {
                        matchStart = searchIdx;
                        matchEnd = searchIdx + query.size();
                        foundMatch = true;
                    }
                }
                
                if (foundMatch) {
                    // Draw text before match
                    const std::string beforeMatch = label.substr(0, matchStart);
                    float currentX = textX;
                    if (!beforeMatch.empty()) {
                        context.DrawText(beforeMatch, Point{ currentX, textY }, textColor, fontSize);
                        currentX += context.GetTextWidth(beforeMatch, fontSize);
                    }
                    
                    // Draw highlighted match
                    const std::string matchText = label.substr(matchStart, matchEnd - matchStart);
                    const float matchWidth = context.GetTextWidth(matchText, fontSize);
                    Rect highlightRect{ currentX, textY, matchWidth, fontSize };
                    context.DrawRoundedRect(highlightRect, ThemeColor(ColorToken::AccentPrimary) * 0.3f, 2.0f);
                    context.DrawText(matchText, Point{ currentX, textY }, ThemeColor(ColorToken::AccentPrimary), fontSize);
                    currentX += matchWidth;
                    
                    // Draw text after match
                    const std::string afterMatch = label.substr(matchEnd);
                    if (!afterMatch.empty()) {
                        context.DrawText(afterMatch, Point{ currentX, textY }, textColor, fontSize);
                    }
                } else {
                    context.DrawText(label, Point{ textX, textY }, textColor, fontSize);
                }
            } else {
                context.DrawText(node->label, Point{ textX, textY }, textColor, fontSize);
            }
        }

        if (m_ShowRowControls) {
        const float accessorySize = static_cast<float>(IconMetrics::GlyphTierPx(MetricToken::IconSizeTree));
        const float accessoryGap = ThemeMetric(MetricToken::Space1) * TreeUiScale();
        const float rowPad = ThemeMetric(MetricToken::Space2) * TreeUiScale();
        const float eyeX = item.geometry.x + item.geometry.width - accessorySize - rowPad;
        const float centerY = item.geometry.y + rowHeight * 0.5f;
        const Color eyeColor = node->visible ? ThemeColor(ColorToken::TextSecondary) : ThemeColor(ColorToken::TextSecondary) * 0.45f;
        const char* eyeIcon = node->visible ? Icons::EyeName : Icons::EyeOffName;
        Rect eyeControl{ eyeX, centerY - accessorySize * 0.5f, accessorySize, accessorySize };
        IconPainter::DrawIcon(context, eyeIcon,
            IconMetrics::PlaceGlyphCentered(eyeControl, accessorySize), eyeColor);

        const float lockX = eyeX - accessorySize - accessoryGap;
        const Color lockColor = node->locked ? ThemeColor(ColorToken::Warning) : ThemeColor(ColorToken::TextSecondary) * 0.55f;
        const char* lockIcon = node->locked ? Icons::LockName : Icons::UnlockName;
        Rect lockControl{ lockX, centerY - accessorySize * 0.5f, accessorySize, accessorySize };
        IconPainter::DrawIcon(context, lockIcon,
            IconMetrics::PlaceGlyphCentered(lockControl, accessorySize), lockColor);
        }
    }

    context.PopClipRect();
    m_Scroll.Paint(context, m_ScrollMetrics, m_Scroll.IsThumbHovered());
}

void TreeView::OnMouseDown(const MouseEvent& event) {
    SyncScrollMetrics();
    if (m_Scroll.OnMouseDown(event, m_ScrollMetrics, m_Geometry.height, m_ContentHeight)) {
        Arrange(m_Geometry);
        return;
    }

    if (event.button == MouseButton::Right) {
        RenderItem* item = GetItemAtPosition(event.position);
        if (item) {
            HandleSelection(item->node->id, event.shiftDown, event.ctrlDown);
            ShowContextMenu(item->node->id, event.position);
        }
        return;
    }

    if (!m_RenamingId.empty()) {
        CommitRename();
    }

    RenderItem* item = GetItemAtPosition(event.position);
    if (!item) {
        return;
    }

    const auto& node = item->node;
    const float rowHeight = m_ItemHeight * TreeUiScale();

    if (!node->children.empty()) {
        const float tier = static_cast<float>(IconMetrics::StandardGlyphTierPx());
        const float chevronX = item->geometry.x + ThemeMetric(MetricToken::Space1) * TreeUiScale();
        Rect chevronRect{ chevronX, item->geometry.y + (rowHeight - tier) * 0.5f, tier, tier };
        if (chevronRect.Contains(event.position)) {
            ToggleExpand(node->id);
            return;
        }
    }

    const float accessorySize = static_cast<float>(IconMetrics::GlyphTierPx(MetricToken::IconSizeTree));
    const float accessoryGap = ThemeMetric(MetricToken::Space1) * TreeUiScale();
    const float rowPad = ThemeMetric(MetricToken::Space2) * TreeUiScale();
    const float eyeX = item->geometry.x + item->geometry.width - accessorySize - rowPad;
    const float lockX = eyeX - accessorySize - accessoryGap;
    Rect lockRect{ lockX, item->geometry.y + (rowHeight - accessorySize) * 0.5f, accessorySize, accessorySize };
    if (lockRect.Contains(event.position)) {
        node->locked = !node->locked;
        if (m_OnLockToggled) {
            m_OnLockToggled(node->id, node->locked);
        }
        return;
    }

    Rect eyeRect{ eyeX, item->geometry.y + (rowHeight - accessorySize) * 0.5f, accessorySize, accessorySize };
    if (eyeRect.Contains(event.position)) {
        node->visible = !node->visible;
        if (m_OnVisibilityToggled) {
            m_OnVisibilityToggled(node->id, node->visible);
        }
        return;
    }

    HandleSelection(node->id, event.shiftDown, event.ctrlDown);
    m_Dragging = false;
    m_DragSourceId = node->id;
    m_DragStart = event.position;
}

void TreeView::OnMouseUp(const MouseEvent& event) {
    m_Scroll.OnMouseUp(event);

    if (m_Dragging && !m_DropTargetId.empty() && m_DropTargetId != m_DragSourceId) {
        if (m_OnReparentRequested) {
            m_OnReparentRequested(m_DragSourceId, m_DropTargetId);
        }
    }
    m_Dragging = false;
    m_DropTargetId.clear();

    static std::string lastClickedId;
    static uint64_t lastClickTime = 0;

    RenderItem* item = GetItemAtPosition(event.position);
    if (!item) {
        return;
    }

    const auto& platform = we::platform::Platform::Get();
    const uint64_t now = platform.GetHighResolutionCounter();
    const uint64_t freq = platform.GetHighResolutionFrequency();
    const double elapsed = static_cast<double>(now - lastClickTime) / static_cast<double>(freq);

    if (item->node->id == lastClickedId && elapsed < 0.3) {
        if (m_OnItemDoubleClicked) {
            m_OnItemDoubleClicked(item->node->id);
        }
        BeginRename(item->node->id);
    }

    lastClickedId = item->node->id;
    lastClickTime = now;
}

void TreeView::OnMouseMove(const MouseEvent& event) {
    SyncScrollMetrics();
    m_Scroll.OnMouseMove(event, m_ScrollMetrics, m_Geometry.height, m_ContentHeight);
    if (m_Scroll.IsDraggingThumb()) {
        Arrange(m_Geometry);
        return;
    }

    RenderItem* item = GetItemAtPosition(event.position);
    m_HoveredId = item ? item->node->id : "";

    if (!m_DragSourceId.empty()) {
        const float dx = event.position.x - m_DragStart.x;
        const float dy = event.position.y - m_DragStart.y;
        if (!m_Dragging && std::sqrt(dx * dx + dy * dy) > 5.0f) {
            m_Dragging = true;
        }
        if (m_Dragging) {
            m_DropTargetId = item ? item->node->id : "";
        }
    }
}

void TreeView::OnMouseWheel(const MouseEvent& event) {
    if (event.ctrlDown) {
        SetZoomLevel(m_ZoomLevel + event.wheelDeltaY * kTreeZoomStep);
        Arrange(m_Geometry);
        return;
    }

    SyncScrollMetrics();
    m_Scroll.ApplyWheel(
        event.wheelDeltaY,
        m_ItemHeight * TreeUiScale() * 0.75f,
        m_Geometry.height,
        m_ContentHeight);
    Arrange(m_Geometry);
}

void TreeView::SetItemHeight(float height) {
    m_BaseItemHeight = std::max(12.0f, height);
    m_ItemHeight = m_BaseItemHeight * m_ZoomLevel;
}

void TreeView::SetIndentWidth(float width) {
    m_BaseIndentWidth = std::max(8.0f, width);
    m_IndentWidth = m_BaseIndentWidth * m_ZoomLevel;
}

void TreeView::SetZoomLevel(float zoomLevel) {
    m_ZoomLevel = std::clamp(zoomLevel, kMinTreeZoom, kMaxTreeZoom);
    m_ItemHeight = m_BaseItemHeight * m_ZoomLevel;
    m_IndentWidth = m_BaseIndentWidth * m_ZoomLevel;
    m_Style.text.size = std::clamp(13.0f * m_ZoomLevel, 10.0f, 20.0f);
}

void TreeView::OnKeyDown(const KeyEvent& event) {
    if (!m_RenamingId.empty()) {
        if (event.key == we::platform::KeyCode::Escape) {
            CancelRename();
            return;
        }
        if (event.key == we::platform::KeyCode::Enter || event.key == we::platform::KeyCode::NumpadEnter) {
            CommitRename();
            return;
        }
        if (event.key == we::platform::KeyCode::Backspace && !m_RenameBuffer.empty()) {
            m_RenameBuffer.pop_back();
            return;
        }
        if (const char ch = KeyCodeToChar(event.key, event.shiftDown); ch != '\0' && m_RenameBuffer.size() < 96) {
            m_RenameBuffer.push_back(ch);
        }
        return;
    }

    if (event.key == we::platform::KeyCode::F2 && !m_SelectedIds.empty()) {
        BeginRename(m_SelectedIds.back());
    }
}

void TreeView::SetRoot(const std::shared_ptr<TreeNode>& root) {
    m_Root = root;
    m_SelectedIds.clear();
    MarkRenderListDirty();
    BuildRenderList();
}

void TreeView::AddItem(const std::shared_ptr<TreeNode>& item, const std::string& parentId) {
    if (parentId.empty()) {
        m_Root->children.push_back(item);
    } else if (auto parent = FindNode(parentId)) {
        parent->children.push_back(item);
    }
    MarkRenderListDirty();
    BuildRenderList();
}

void TreeView::RemoveItem(const std::string& id) {
    std::function<bool(std::vector<std::shared_ptr<TreeNode>>&)> removeRecursive =
        [&](std::vector<std::shared_ptr<TreeNode>>& nodes) -> bool {
            for (auto it = nodes.begin(); it != nodes.end(); ++it) {
                if ((*it)->id == id) {
                    nodes.erase(it);
                    return true;
                }
                if (removeRecursive((*it)->children)) {
                    return true;
                }
            }
            return false;
        };
    removeRecursive(m_Root->children);
    MarkRenderListDirty();
    BuildRenderList();
}

void TreeView::Clear() {
    m_Root->children.clear();
    m_SelectedIds.clear();
    MarkRenderListDirty();
    BuildRenderList();
}

void TreeView::SetSelectedId(const std::string& id) {
    m_SelectedIds = id.empty() ? std::vector<std::string>{} : std::vector<std::string>{ id };
}

void TreeView::SetSelectedIds(const std::vector<std::string>& ids) {
    m_SelectedIds = ids;
}

std::string TreeView::GetSelectedId() const {
    return m_SelectedIds.empty() ? std::string{} : m_SelectedIds.back();
}

void TreeView::BuildRenderList() {
    if (!m_RenderListDirty && !m_RenderList.empty()) {
        return;
    }

    m_RenderList.clear();

    // Fuzzy match helper function
    auto fuzzyMatch = [](const std::string& text, const std::string& pattern) -> bool {
        if (pattern.empty()) return true;
        
        size_t textIdx = 0;
        size_t patternIdx = 0;
        
        while (textIdx < text.size() && patternIdx < pattern.size()) {
            if (std::tolower(text[textIdx]) == std::tolower(pattern[patternIdx])) {
                patternIdx++;
            }
            textIdx++;
        }
        
        return patternIdx == pattern.size();
    };

    // Check if node matches filter options
    auto matchesFilter = [this, fuzzyMatch](const std::shared_ptr<TreeNode>& node) -> bool {
        // Search query filter
        if (!m_SearchQuery.empty() && !fuzzyMatch(node->label, m_SearchQuery)) {
            return false;
        }
        
        // Hidden items filter
        if (!m_FilterOptions.showHidden && !node->visible) {
            return false;
        }
        
        // Locked items filter
        if (!m_FilterOptions.showLocked && node->locked) {
            return false;
        }
        
        // Empty folders filter
        if (!m_FilterOptions.showEmptyFolders && node->children.empty() && node->iconName == Icons::FolderName) {
            return false;
        }
        
        return true;
    };

    std::function<void(const std::shared_ptr<TreeNode>&, int, bool)> buildRecursive =
        [&](const std::shared_ptr<TreeNode>& node, int depth, bool parentMatches) {
            if (node->id != "root") {
                const bool nodeMatches = matchesFilter(node);
                const bool shouldShow = nodeMatches || parentMatches;
                
                if (shouldShow) {
                    m_RenderList.push_back({ node, depth, 0, Rect{} });
                }
                
                // Always expand children if searching or if parent matches
                if (nodeMatches || parentMatches || !m_SearchQuery.empty()) {
                    for (const auto& child : node->children) {
                        buildRecursive(child, depth + 1, shouldShow);
                    }
                } else if (node->expanded) {
                    for (const auto& child : node->children) {
                        buildRecursive(child, depth + 1, false);
                    }
                }
            } else {
                // Root node - process children
                for (const auto& child : node->children) {
                    buildRecursive(child, 0, false);
                }
            }
        };

    buildRecursive(m_Root, 0, false);

    m_ContentHeight = static_cast<float>(m_RenderList.size()) * m_ItemHeight * TreeUiScale();
    m_RenderListDirty = false;
}

void TreeView::UpdateVisibleRange() {
    if (m_RenderList.empty()) {
        m_FirstVisibleIndex = 0;
        m_LastVisibleIndex = -1;
        return;
    }

    const float rowHeight = m_ItemHeight * TreeUiScale();
    const float viewTop = m_Geometry.y;
    const float viewBottom = m_Geometry.y + m_Geometry.height;
    const int overscan = 2;

    m_FirstVisibleIndex = static_cast<int>(std::floor(m_Scroll.offset / rowHeight));
    m_FirstVisibleIndex = std::max(0, m_FirstVisibleIndex - overscan);

    const int visibleCount = static_cast<int>(std::ceil(m_Geometry.height / rowHeight)) + overscan * 2;
    m_LastVisibleIndex = std::min(static_cast<int>(m_RenderList.size()) - 1, m_FirstVisibleIndex + visibleCount);

    (void)viewTop;
    (void)viewBottom;
}

TreeView::RenderItem* TreeView::GetItemAtPosition(const Point& pos) {
    for (auto& item : m_RenderList) {
        if (item.geometry.Contains(pos)) {
            return &item;
        }
    }
    return nullptr;
}

std::shared_ptr<TreeNode> TreeView::FindNode(const std::string& id) {
    std::function<std::shared_ptr<TreeNode>(const std::shared_ptr<TreeNode>&)> findRecursive =
        [&](const std::shared_ptr<TreeNode>& node) -> std::shared_ptr<TreeNode> {
            if (node->id == id) {
                return node;
            }
            for (const auto& child : node->children) {
                if (auto found = findRecursive(child)) {
                    return found;
                }
            }
            return nullptr;
        };

    if (m_Root->id == id) {
        return m_Root;
    }
    for (const auto& child : m_Root->children) {
        if (auto found = findRecursive(child)) {
            return found;
        }
    }
    return nullptr;
}

void TreeView::ToggleExpand(const std::string& id) {
    if (auto node = FindNode(id)) {
        node->expanded = !node->expanded;
    }
    MarkRenderListDirty();
    BuildRenderList();
    Arrange(m_Geometry);
}

void TreeView::BeginRename(const std::string& id) {
    if (auto node = FindNode(id)) {
        m_RenamingId = id;
        m_RenameBuffer = node->label;
        m_RenameCursorBlink = 0.0f;
    }
}

void TreeView::CommitRename() {
    if (m_RenamingId.empty()) {
        return;
    }
    if (auto node = FindNode(m_RenamingId)) {
        if (!m_RenameBuffer.empty() && m_RenameBuffer != node->label) {
            node->label = m_RenameBuffer;
            if (m_OnRenameCommitted) {
                m_OnRenameCommitted(m_RenamingId, m_RenameBuffer);
            }
        }
    }
    m_RenamingId.clear();
    m_RenameBuffer.clear();
}

void TreeView::CancelRename() {
    m_RenamingId.clear();
    m_RenameBuffer.clear();
}

void TreeView::ShowContextMenu(const std::string& id, const Point& position) {
    auto makeItem = [](const std::string& label, std::function<void()> onClick, bool enabled = true) {
        TreeMenuItem item;
        item.label = label;
        item.onClick = std::move(onClick);
        item.enabled = enabled;
        return item;
    };

    std::vector<TreeMenuItem> items;
    items.push_back(makeItem("Rename", [this, id]() { BeginRename(id); }));
    items.push_back(makeItem("Duplicate", []() {}));
    items.push_back(makeItem("Delete", []() {}));
    items.push_back(makeItem("Create Child Actor", []() {}));

    auto menu = std::make_shared<TreeContextMenu>(items, nullptr);
    if (auto* overlay = GetPopupHost()) {
        overlay->CloseAllPopups();
        overlay->ShowPopup(menu, position);
    }
}

void TreeView::HandleSelection(const std::string& id, bool shift, bool ctrl) {
    if (ctrl) {
        auto it = std::find(m_SelectedIds.begin(), m_SelectedIds.end(), id);
        if (it != m_SelectedIds.end()) {
            m_SelectedIds.erase(it);
        } else {
            m_SelectedIds.push_back(id);
        }
    } else if (shift && !m_SelectedIds.empty()) {
        const std::string anchor = m_SelectedIds.back();
        bool inRange = false;
        m_SelectedIds.clear();
        for (const auto& item : m_RenderList) {
            if (item.node->id == anchor || item.node->id == id) {
                inRange = !inRange;
                m_SelectedIds.push_back(item.node->id);
                if (item.node->id == anchor && item.node->id == id) {
                    break;
                }
                if (!inRange && (item.node->id == anchor || item.node->id == id)) {
                    break;
                }
                continue;
            }
            if (inRange) {
                m_SelectedIds.push_back(item.node->id);
            }
        }
        if (m_SelectedIds.empty()) {
            m_SelectedIds.push_back(id);
        }
    } else {
        m_SelectedIds = { id };
    }

    if (m_OnSelectionChanged) {
        m_OnSelectionChanged(m_SelectedIds);
    }

    ScrollSelectionIntoView();
}

bool TreeView::IsSelected(const std::string& id) const {
    return std::find(m_SelectedIds.begin(), m_SelectedIds.end(), id) != m_SelectedIds.end();
}

int TreeView::GetVisibleRowCount() const {
    return static_cast<int>(std::ceil(m_Geometry.height / m_ItemHeight));
}

void TreeView::SetSearchQuery(const std::string& query) {
    m_SearchQuery = query;
    MarkRenderListDirty();
    BuildRenderList();
    Arrange(m_Geometry);
}

bool TreeView::ShowsPointerCursor(const Point& position) const {
    return m_ScrollMetrics.showsScrollbar &&
        (m_ScrollMetrics.thumb.Contains(position) || m_ScrollMetrics.track.Contains(position));
}

} // namespace we::editor::contentbrowser