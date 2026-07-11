#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "WindEffects/Editor/UI/Docking/IDockManager.h"
#include "WindEffects/Editor/UI/Extensions/UIExtensionRegistry.h"

#include <unordered_map>

namespace we::UI {
class Widget;
class Panel;
class Splitter;
class DockContainer;
}

namespace WindEffects::Editor::UI {

struct DockLayoutBuildResult {
    std::shared_ptr<we::UI::Widget> root;
    std::shared_ptr<we::UI::Splitter> mainHorizontalSplitter;
    std::shared_ptr<we::UI::Splitter> leftCenterSplitter;
    std::shared_ptr<we::UI::Splitter> toolsViewportSplitter;
    std::shared_ptr<we::UI::Splitter> rightVerticalSplitter;
    std::shared_ptr<we::UI::DockContainer> toolsDock;
    std::shared_ptr<we::UI::DockContainer> viewportDock;
    std::shared_ptr<we::UI::DockContainer> explorerDock;
    std::unordered_map<std::string, std::shared_ptr<we::UI::Panel>> panels;
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
    std::shared_ptr<we::UI::Widget> BuildNode(
        const DockLayoutNode& node,
        const UIExtensionRegistry& extensions,
        float dpiScale,
        DockLayoutBuildResult& result);

    std::shared_ptr<we::UI::Panel> CreatePanel(
        std::string_view panelId,
        const UIExtensionRegistry& extensions,
        float dpiScale,
        DockLayoutBuildResult& result);
};

UIFRAMEWORK_API void BridgeLegacyEditorRegistry(UIExtensionRegistry& extensions);

} // namespace WindEffects::Editor::UI
