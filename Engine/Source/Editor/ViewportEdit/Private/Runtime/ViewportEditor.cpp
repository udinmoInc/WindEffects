#include "ViewportEditInternal.h"

#include <utility>

namespace we::editor::viewportedit {
namespace detail {

class ViewportEditorImpl final : public IViewportEditor, public IViewportContext {
public:
    explicit ViewportEditorImpl(ViewportEditDependencies deps)
        : m_Deps(std::move(deps))
    {
        m_Selection = CreateSelection();
        m_Snap = CreateSnapProvider(m_Deps.config.snap);
        m_Grid = CreateGridProvider(m_Deps.config.snap.gridSize);
        m_HitTester = CreateHitTester(*this);
        m_Camera = CreateCameraController(*this);
        m_Manipulator = CreateManipulator(*this);
        m_Renderer = CreateRenderer();
        m_DragDrop = CreateDragDrop(*this);
        m_Interaction = CreateInteraction(*this);
        m_SelectionContext = CreateSelectionContext(*m_Selection);
        m_ToolContext = CreateToolContext(*this);
        m_Commands = CreateCommandRouter(*this);
        m_Modes = CreateModeManager(*this);
        m_Workspace = CreateWorkspace(*this, *this, *m_Modes, *m_Commands, *m_SelectionContext);

        RegisterTool(CreateSelectTool());
        RegisterTool(CreateMoveTool());
        RegisterTool(CreateRotateTool());
        RegisterTool(CreateScaleTool());
        m_Modes->RegisterModeInstance(CreateDefaultMode());

        m_Selection->AddListener([this]() { SyncSelectionToScene(); });

        SetActiveTool(ViewportToolId::Select);
        (void)m_Modes->SetActiveMode(ViewportModeId::Select);
        m_ActiveModeKey = std::string(m_Modes->ActiveModeKey());
        m_ActiveModeId = m_Modes->ActiveModeId();
    }

    [[nodiscard]] IViewportSelection& Selection() noexcept override { return *m_Selection; }
    [[nodiscard]] IViewportHitTester& HitTester() noexcept override { return *m_HitTester; }
    [[nodiscard]] IViewportCameraController& Camera() noexcept override { return *m_Camera; }
    [[nodiscard]] IViewportManipulator& Manipulator() noexcept override { return *m_Manipulator; }
    [[nodiscard]] IViewportSnapProvider& Snap() noexcept override { return *m_Snap; }
    [[nodiscard]] IViewportGridProvider& Grid() noexcept override { return *m_Grid; }

    [[nodiscard]] undo::IUndoRuntime* Undo() noexcept override { return m_Deps.undo; }
    [[nodiscard]] undo::ITransactionManager* Transactions() noexcept override {
        return m_Deps.undo ? &m_Deps.undo->Manager() : nullptr;
    }
    [[nodiscard]] property::IPropertyEditorRuntime* PropertyEditor() noexcept override {
        return m_Deps.propertyEditor;
    }
    [[nodiscard]] scene::Scene* Scene() noexcept override { return m_Deps.scene; }
    [[nodiscard]] world::IWorldRuntime* World() noexcept override { return m_Deps.world; }
    [[nodiscard]] engine::EditorCamera* EditorCamera() noexcept override {
        return m_Deps.editorCamera;
    }

    [[nodiscard]] ViewportToolId ActiveTool() const noexcept override { return m_ActiveToolId; }
    [[nodiscard]] ViewportModeId ActiveMode() const noexcept override { return m_ActiveModeId; }
    [[nodiscard]] std::string_view ActiveModeKey() const noexcept override { return m_ActiveModeKey; }
    [[nodiscard]] IViewportSelectionContext* SelectionContextPtr() noexcept override {
        return m_SelectionContext.get();
    }
    [[nodiscard]] float ViewportWidth() const noexcept override { return m_Width; }
    [[nodiscard]] float ViewportHeight() const noexcept override { return m_Height; }

    void SetViewportSize(float w, float h) override {
        m_Width = w;
        m_Height = h;
        if (m_Deps.editorCamera) {
            m_Deps.editorCamera->SetViewportSize(w, h);
        }
    }

    void NotifySelectionChanged() override { SyncSelectionToScene(); }

    [[nodiscard]] IViewportContext& Context() noexcept override { return *this; }
    [[nodiscard]] IViewportInteraction& Interaction() noexcept override { return *m_Interaction; }
    [[nodiscard]] IViewportRenderer& Renderer() noexcept override { return *m_Renderer; }
    [[nodiscard]] IViewportDragDrop& DragDrop() noexcept override { return *m_DragDrop; }
    [[nodiscard]] IViewportWorkspace& Workspace() noexcept override { return *m_Workspace; }
    [[nodiscard]] IViewportModeManager& Modes() noexcept override { return *m_Modes; }
    [[nodiscard]] IViewportCommandRouter& Commands() noexcept override { return *m_Commands; }
    [[nodiscard]] IViewportSelectionContext& SelectionContext() noexcept override {
        return *m_SelectionContext;
    }
    [[nodiscard]] IViewportToolContext& ToolContext() noexcept override { return *m_ToolContext; }

    void SetActiveTool(ViewportToolId tool) override {
        if (m_ActiveTool && m_ActiveTool->GetId() == tool && tool != ViewportToolId::Custom) {
            return;
        }
        if (m_ActiveTool) {
            m_ActiveTool->OnDeactivated(*this);
        }
        m_ActiveTool = FindTool(tool);
        m_ActiveToolId = tool;
        if (m_ActiveTool) {
            m_ActiveTool->OnActivated(*this);
        }
        BindInteractionTool(*m_Interaction, m_ActiveTool);
        m_Manipulator->SetTool(tool);
    }

    void SetActiveTool(std::string_view toolId) override {
        if (auto* tool = FindToolByStableId(toolId)) {
            SetActiveTool(tool->GetId());
            m_ActiveTool = tool;
            BindInteractionTool(*m_Interaction, m_ActiveTool);
            return;
        }
        // Fall back to builtin name match.
        for (std::uint8_t i = 0; i <= static_cast<std::uint8_t>(ViewportToolId::Custom); ++i) {
            const auto id = static_cast<ViewportToolId>(i);
            if (BuiltinToolName(id) == toolId) {
                SetActiveTool(id);
                return;
            }
        }
    }

    [[nodiscard]] ViewportToolId GetActiveTool() const noexcept override { return m_ActiveToolId; }

    void SetActiveMode(ViewportModeId mode) override {
        if (m_Modes->SetActiveMode(mode)) {
            m_ActiveModeId = m_Modes->ActiveModeId();
            m_ActiveModeKey = std::string(m_Modes->ActiveModeKey());
            if (auto* active = m_Modes->ActiveMode()) {
                SetActiveTool(active->PreferredTool());
            }
        }
    }

    void SetActiveMode(std::string_view modeId) override {
        if (m_Modes->SetActiveMode(modeId)) {
            m_ActiveModeId = m_Modes->ActiveModeId();
            m_ActiveModeKey = std::string(m_Modes->ActiveModeKey());
            if (auto* active = m_Modes->ActiveMode()) {
                SetActiveTool(active->PreferredTool());
            }
        }
    }

    [[nodiscard]] ViewportModeId GetActiveMode() const noexcept override { return m_ActiveModeId; }
    [[nodiscard]] std::string_view GetActiveModeKey() const noexcept override {
        return m_ActiveModeKey;
    }

    void RegisterTool(std::unique_ptr<IViewportTool> tool) override {
        if (!tool) {
            return;
        }
        const auto id = tool->GetId();
        const std::string stable(tool->GetStableId());
        m_ToolsByStable[stable] = tool.get();
        m_Tools[id] = std::move(tool);
    }

    void RegisterMode(std::unique_ptr<IViewportMode> mode) override {
        m_Modes->RegisterModeInstance(std::move(mode));
    }

    void RegisterOverlay(std::unique_ptr<IViewportOverlay> overlay) override {
        m_Workspace->RegisterOverlay(std::move(overlay));
    }

    void Tick(float deltaSeconds) override {
        if (auto* mode = m_Modes->ActiveMode()) {
            mode->Tick(*this, deltaSeconds);
        }
        m_Interaction->Tick(deltaSeconds);
        m_Renderer->BeginFrame();
        m_Renderer->DrawGrid();
        m_Renderer->DrawSelection(m_Selection->GetSelected());
        m_Renderer->DrawHover(m_Selection->GetHovered());
        m_Renderer->EndFrame();
        m_Workspace->Tick(deltaSeconds);
    }

    void SyncSelectionToScene() override {
        auto* scene = m_Deps.scene;
        if (!scene) {
            return;
        }
        const auto primary = m_Selection->GetPrimary();
        if (!primary.IsValid()) {
            scene->SetSelectedEntityIndex(-1);
            return;
        }
        scene->SetSelectedEntityId(primary.value);
    }

    void SyncSelectionFromScene() override {
        auto* scene = m_Deps.scene;
        if (!scene) {
            return;
        }
        const auto id = scene->GetSelectedEntityId();
        if (id == 0) {
            if (!m_Selection->IsEmpty()) {
                m_Selection->Clear();
            }
            return;
        }
        const ViewportObjectId obj{id};
        if (m_Selection->GetPrimary() != obj) {
            m_Selection->Set(obj);
        }
    }

private:
    IViewportTool* FindTool(ViewportToolId id) const {
        auto it = m_Tools.find(id);
        return it != m_Tools.end() ? it->second.get() : nullptr;
    }

    IViewportTool* FindToolByStableId(std::string_view id) const {
        auto it = m_ToolsByStable.find(std::string(id));
        return it != m_ToolsByStable.end() ? it->second : nullptr;
    }

    ViewportEditDependencies m_Deps;
    std::unique_ptr<IViewportSelection> m_Selection;
    std::unique_ptr<IViewportHitTester> m_HitTester;
    std::unique_ptr<IViewportCameraController> m_Camera;
    std::unique_ptr<IViewportManipulator> m_Manipulator;
    std::unique_ptr<IViewportSnapProvider> m_Snap;
    std::unique_ptr<IViewportGridProvider> m_Grid;
    std::unique_ptr<IViewportRenderer> m_Renderer;
    std::unique_ptr<IViewportDragDrop> m_DragDrop;
    std::unique_ptr<IViewportInteraction> m_Interaction;
    std::unique_ptr<IViewportModeManager> m_Modes;
    std::unique_ptr<IViewportCommandRouter> m_Commands;
    std::unique_ptr<IViewportSelectionContext> m_SelectionContext;
    std::unique_ptr<IViewportToolContext> m_ToolContext;
    std::unique_ptr<IViewportWorkspace> m_Workspace;

    std::unordered_map<ViewportToolId, std::unique_ptr<IViewportTool>> m_Tools;
    std::unordered_map<std::string, IViewportTool*> m_ToolsByStable;

    IViewportTool* m_ActiveTool = nullptr;
    ViewportToolId m_ActiveToolId = ViewportToolId::Select;
    ViewportModeId m_ActiveModeId = ViewportModeId::Select;
    std::string m_ActiveModeKey = "Select";
    float m_Width = 1.f;
    float m_Height = 1.f;
};

} // namespace detail

std::unique_ptr<IViewportEditor> CreateViewportEditRuntime(const ViewportEditDependencies& deps) {
    return std::make_unique<detail::ViewportEditorImpl>(deps);
}

} // namespace we::editor::viewportedit
