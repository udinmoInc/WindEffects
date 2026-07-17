#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "WindEffects/Editor/UI/Docking/IDockManager.h"
#include "WindEffects/Editor/UI/Extensions/UIExtensionRegistry.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Layout/Splitter.h"
#include "WindEffects/Editor/UI/Widgets/Panel.h"
#include "WindEffects/Editor/UI/Widgets/DockContainer.h"

#include <unordered_map>

namespace we::editor::shell {
using ::we::runtime::kindui::Widget;
using ::we::runtime::kindui::Splitter;
using ::we::editor::panels::Panel;
using ::we::editor::docking::DockContainer;
using ::we::editor::docking::WorkspaceLayout;
using ::we::editor::docking::DockLayoutNode;
using ::we::editor::extensions::UIExtensionRegistry;

struct DockLayoutBuildResult {
    std::shared_ptr<Widget> root;
    std::shared_ptr<Splitter> mainHorizontalSplitter;
    std::shared_ptr<Splitter> leftCenterSplitter;
    std::shared_ptr<Splitter> toolsViewportSplitter;
    std::shared_ptr<Splitter> rightVerticalSplitter;
    std::shared_ptr<DockContainer> toolsDock;
    std::shared_ptr<DockContainer> viewportDock;
    std::shared_ptr<DockContainer> explorerDock;
    std::unordered_map<std::string, std::shared_ptr<Panel>> panels;
};

class UIFRAMEWORK_API IDockLayoutBuilder {
public:
    virtual ~IDockLayoutBuilder() = default;
    virtual DockLayoutBuildResult Build(
        const WorkspaceLayout& layout,
        const UIExtensionRegistry& extensions,
        float dpiScale) = 0;
};

class UIFRAMEWORK_API DockLayoutBuilder final : public IDockLayoutBuilder {
public:
    DockLayoutBuildResult Build(
        const WorkspaceLayout& layout,
        const UIExtensionRegistry& extensions,
        float dpiScale) override;

private:
    std::shared_ptr<Widget> BuildNode(
        const DockLayoutNode& node,
        const UIExtensionRegistry& extensions,
        float dpiScale,
        DockLayoutBuildResult& result);

    std::shared_ptr<Panel> CreatePanel(
        std::string_view panelId,
        const UIExtensionRegistry& extensions,
        float dpiScale,
        DockLayoutBuildResult& result);
};

} // namespace we::editor::shell
