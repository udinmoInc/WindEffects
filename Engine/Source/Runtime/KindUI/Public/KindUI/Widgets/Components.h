#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Layout/ScrollLayout.h"
#include "KindUI/Widgets/Label.h"
#include "KindUI/Widgets/TextBox.h"
#include "KindUI/Widgets/VirtualList.h"

#include <functional>
#include <string>

namespace we::runtime::kindui {

/// Higher-level reusable components for Launcher/Editor composition.

class KINDUI_API EmptyState : public Column {
public:
    explicit EmptyState(std::string title, std::string subtitle = {});
};

class KINDUI_API StatusBadge : public Widget {
public:
    explicit StatusBadge(std::string text);
    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void SetText(std::string text);

private:
    std::string m_Text;
};

class KINDUI_API DialogChrome : public Column {
public:
    DialogChrome(std::string title, const std::shared_ptr<Widget>& body);
};

class KINDUI_API ToolbarBar : public Row {
public:
    ToolbarBar();
};

class KINDUI_API SkeletonBlock : public Widget {
public:
    SkeletonBlock();
    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void Tick(float deltaTime) override;

private:
    float m_Pulse = 0.0f;
};

[[nodiscard]] KINDUI_API std::shared_ptr<SearchBoxControl> MakeSearchBar(std::string placeholder = "Search...");
[[nodiscard]] KINDUI_API std::shared_ptr<EmptyState> MakeEmptyState(std::string title, std::string subtitle = {});
[[nodiscard]] KINDUI_API std::shared_ptr<StatusBadge> MakeStatusBadge(std::string text);
[[nodiscard]] KINDUI_API std::shared_ptr<SectionHeader> MakeSectionHeader(std::string title);
[[nodiscard]] KINDUI_API std::shared_ptr<Card> MakeCard();
[[nodiscard]] KINDUI_API std::shared_ptr<SkeletonBlock> MakeSkeleton();

} // namespace we::runtime::kindui
