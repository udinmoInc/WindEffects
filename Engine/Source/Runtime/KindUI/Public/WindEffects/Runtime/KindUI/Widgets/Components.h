#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "WindEffects/Runtime/UI/Core/Widgets/DesignSystemControls.h"
#include "WindEffects/Runtime/UI/Layout/Flex.h"
#include "WindEffects/Runtime/UI/Layout/ScrollLayout.h"
#include "WindEffects/Runtime/UI/Widgets/Label.h"
#include "WindEffects/Runtime/UI/Widgets/TextBox.h"
#include "WindEffects/Runtime/UI/Widgets/VirtualList.h"

#include <functional>
#include <string>

namespace WindEffects::Editor::UI {

/// Higher-level reusable components for Launcher/Editor composition.

class UI_API EmptyState : public Column {
public:
    explicit EmptyState(std::string title, std::string subtitle = {});
};

class UI_API StatusBadge : public Widget {
public:
    explicit StatusBadge(std::string text);
    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void SetText(std::string text);

private:
    std::string m_Text;
};

class UI_API DialogChrome : public Column {
public:
    DialogChrome(std::string title, const std::shared_ptr<Widget>& body);
};

class UI_API ToolbarBar : public Row {
public:
    ToolbarBar();
};

class UI_API SkeletonBlock : public Widget {
public:
    SkeletonBlock();
    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void Tick(float deltaTime) override;

private:
    float m_Pulse = 0.0f;
};

[[nodiscard]] UI_API std::shared_ptr<SearchBoxControl> MakeSearchBar(std::string placeholder = "Search...");
[[nodiscard]] UI_API std::shared_ptr<EmptyState> MakeEmptyState(std::string title, std::string subtitle = {});
[[nodiscard]] UI_API std::shared_ptr<StatusBadge> MakeStatusBadge(std::string text);
[[nodiscard]] UI_API std::shared_ptr<SectionHeader> MakeSectionHeader(std::string title);
[[nodiscard]] UI_API std::shared_ptr<Card> MakeCard();
[[nodiscard]] UI_API std::shared_ptr<SkeletonBlock> MakeSkeleton();

} // namespace WindEffects::Editor::UI
