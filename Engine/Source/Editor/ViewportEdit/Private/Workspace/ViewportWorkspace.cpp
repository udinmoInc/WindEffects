#include "ViewportEditInternal.h"

#include "Undo/UndoTypes.h"

#include <algorithm>
#include <unordered_map>

namespace we::editor::viewportedit {
namespace detail {
namespace {

class ViewportCommandRouterImpl final : public IViewportCommandRouter {
public:
    explicit ViewportCommandRouterImpl(IViewportContext& context)
        : m_Context(context)
    {}

    void Register(std::string_view commandId, ViewportCommandFactory factory) override {
        if (!commandId.empty() && factory) {
            m_Factories[std::string(commandId)] = std::move(factory);
        }
    }

    void Unregister(std::string_view commandId) override {
        m_Factories.erase(std::string(commandId));
    }

    [[nodiscard]] bool Has(std::string_view commandId) const noexcept override {
        return m_Factories.contains(std::string(commandId));
    }

    [[nodiscard]] bool Execute(std::string_view commandId) override {
        auto it = m_Factories.find(std::string(commandId));
        if (it == m_Factories.end() || !it->second) {
            return false;
        }
        std::shared_ptr<IViewportCommand> cmd(it->second().release());
        if (!cmd) {
            return false;
        }
        cmd->Execute();
        auto* tx = m_Context.Transactions();
        if (tx) {
            const bool ok = tx->RecordCustom(
                cmd->GetName(),
                undo::TransactionKind::Generic,
                std::string(cmd->GetName()),
                [cmd]() {
                    cmd->Undo();
                    return true;
                },
                [cmd]() {
                    cmd->Execute();
                    return true;
                });
            if (ok) {
                ViewportEditDiagnostics::Get().OnTransaction();
            }
        }
        ViewportEditDiagnostics::Get().OnCommandExecuted();
        return true;
    }

    [[nodiscard]] bool RecordCustom(
        std::string_view label,
        std::function<bool()> undoFn,
        std::function<bool()> redoFn) override
    {
        auto* tx = m_Context.Transactions();
        if (!tx) {
            // No undo stack — still run redo to apply.
            return redoFn ? redoFn() : false;
        }
        const bool ok = tx->RecordCustom(
            label,
            undo::TransactionKind::Generic,
            std::string(label),
            std::move(undoFn),
            std::move(redoFn));
        if (ok) {
            ViewportEditDiagnostics::Get().OnCommandExecuted();
            ViewportEditDiagnostics::Get().OnTransaction();
        }
        return ok;
    }

private:
    IViewportContext& m_Context;
    std::unordered_map<std::string, ViewportCommandFactory> m_Factories;
};

class ViewportSelectionContextImpl final : public IViewportSelectionContext {
public:
    explicit ViewportSelectionContextImpl(IViewportSelection& selection)
        : m_Selection(selection)
    {}

    [[nodiscard]] IViewportSelection& Selection() noexcept override { return m_Selection; }
    [[nodiscard]] const IViewportSelection& Selection() const noexcept override {
        return m_Selection;
    }

    [[nodiscard]] bool IsSelectable(ViewportObjectId id) const noexcept override {
        if (!id.IsValid()) {
            return false;
        }
        if (m_Filter) {
            return m_Filter(id);
        }
        return !m_Selection.IsLocked(id) && !m_Selection.IsHidden(id);
    }

    void SetFilter(std::function<bool(ViewportObjectId)> filter) override {
        m_Filter = std::move(filter);
    }

    void ClearFilter() override { m_Filter = {}; }

    void SetPrimaryTerrain(ViewportObjectId id) override { m_PrimaryTerrain = id; }
    [[nodiscard]] ViewportObjectId PrimaryTerrain() const noexcept override {
        return m_PrimaryTerrain;
    }

private:
    IViewportSelection& m_Selection;
    std::function<bool(ViewportObjectId)> m_Filter;
    ViewportObjectId m_PrimaryTerrain{};
};

class ViewportToolContextImpl final : public IViewportToolContext {
public:
    explicit ViewportToolContextImpl(IViewportContext& context)
        : m_Context(context)
    {}

    [[nodiscard]] IViewportContext& EditorContext() noexcept override { return m_Context; }
    [[nodiscard]] ViewportToolId ActiveToolId() const noexcept override {
        return m_Context.ActiveTool();
    }
    [[nodiscard]] ViewportModeId ActiveModeId() const noexcept override {
        return m_Context.ActiveMode();
    }
    [[nodiscard]] std::string_view ActiveModeKey() const noexcept override {
        return m_Context.ActiveModeKey();
    }
    [[nodiscard]] float ViewportWidth() const noexcept override {
        return m_Context.ViewportWidth();
    }
    [[nodiscard]] float ViewportHeight() const noexcept override {
        return m_Context.ViewportHeight();
    }

private:
    IViewportContext& m_Context;
};

class ViewportWorkspaceImpl final : public IViewportWorkspace {
public:
    ViewportWorkspaceImpl(
        IViewportEditor& editor,
        IViewportContext& context,
        IViewportModeManager& modes,
        IViewportCommandRouter& commands,
        IViewportSelectionContext& selectionContext)
        : m_Editor(editor)
        , m_Context(context)
        , m_Modes(modes)
        , m_Commands(commands)
        , m_SelectionContext(selectionContext)
    {}

    [[nodiscard]] IViewportEditor& Editor() noexcept override { return m_Editor; }
    [[nodiscard]] IViewportContext& Context() noexcept override { return m_Context; }
    [[nodiscard]] IViewportModeManager& Modes() noexcept override { return m_Modes; }
    [[nodiscard]] IViewportCommandRouter& Commands() noexcept override { return m_Commands; }
    [[nodiscard]] IViewportSelectionContext& SelectionContext() noexcept override {
        return m_SelectionContext;
    }

    void RegisterOverlay(std::unique_ptr<IViewportOverlay> overlay) override {
        if (overlay) {
            m_Overlays.push_back(std::move(overlay));
        }
    }

    void UnregisterOverlay(std::string_view overlayId) override {
        m_Overlays.erase(
            std::remove_if(
                m_Overlays.begin(),
                m_Overlays.end(),
                [&](const std::unique_ptr<IViewportOverlay>& o) {
                    return o && o->GetId() == overlayId;
                }),
            m_Overlays.end());
    }

    [[nodiscard]] std::span<const std::unique_ptr<IViewportOverlay>> Overlays() const noexcept override {
        return m_Overlays;
    }

    void RegisterRenderExtension(std::unique_ptr<IViewportRenderExtension> extension) override {
        if (extension) {
            m_Extensions.push_back(std::move(extension));
            std::sort(m_Extensions.begin(), m_Extensions.end(), [](const auto& a, const auto& b) {
                return a->SortOrder() < b->SortOrder();
            });
        }
    }

    void UnregisterRenderExtension(std::string_view extensionId) override {
        m_Extensions.erase(
            std::remove_if(
                m_Extensions.begin(),
                m_Extensions.end(),
                [&](const std::unique_ptr<IViewportRenderExtension>& e) {
                    return e && e->GetId() == extensionId;
                }),
            m_Extensions.end());
    }

    void PushInteractionLayer(std::unique_ptr<IViewportInteractionLayer> layer) override {
        if (!layer) {
            return;
        }
        m_LayerStorage.push_back(std::move(layer));
        RebuildLayerPointers();
        BindInteractionLayers(m_Editor.Interaction(), &m_LayerPointers);
    }

    void PopInteractionLayer(std::string_view layerId) override {
        m_LayerStorage.erase(
            std::remove_if(
                m_LayerStorage.begin(),
                m_LayerStorage.end(),
                [&](const std::unique_ptr<IViewportInteractionLayer>& l) {
                    return l && l->GetId() == layerId;
                }),
            m_LayerStorage.end());
        RebuildLayerPointers();
        BindInteractionLayers(m_Editor.Interaction(), &m_LayerPointers);
    }

    void RegisterTool(std::unique_ptr<IViewportTool> tool) override {
        m_Editor.RegisterTool(std::move(tool));
    }

    [[nodiscard]] IViewportTool* FindTool(ViewportToolId id) noexcept override {
        // Tools live on the editor; workspace forwards via SetActiveTool path.
        (void)id;
        return nullptr;
    }

    [[nodiscard]] IViewportTool* FindTool(std::string_view) noexcept override { return nullptr; }

    void Tick(float deltaSeconds) override {
        for (auto& ext : m_Extensions) {
            if (ext && ext->IsEnabled()) {
                ext->OnBeginFrame(m_Context);
            }
        }
        for (auto& ext : m_Extensions) {
            if (ext && ext->IsEnabled()) {
                ext->OnDrawWorld(m_Context);
            }
        }
        for (auto& overlay : m_Overlays) {
            if (overlay && overlay->IsEnabled()) {
                overlay->Draw(m_Context, m_Context.ViewportWidth(), m_Context.ViewportHeight());
                overlay->Draw(m_Context.ViewportWidth(), m_Context.ViewportHeight());
            }
        }
        for (auto& ext : m_Extensions) {
            if (ext && ext->IsEnabled()) {
                ext->OnDrawOverlay(m_Context);
                ext->OnEndFrame(m_Context);
            }
        }
        (void)deltaSeconds;
    }

    [[nodiscard]] std::vector<IViewportInteractionLayer*>& LayerPointers() noexcept {
        return m_LayerPointers;
    }

private:
    void RebuildLayerPointers() {
        m_LayerPointers.clear();
        m_LayerPointers.reserve(m_LayerStorage.size());
        for (auto& layer : m_LayerStorage) {
            if (layer && layer->IsEnabled()) {
                m_LayerPointers.push_back(layer.get());
            }
        }
        std::sort(m_LayerPointers.begin(), m_LayerPointers.end(), [](auto* a, auto* b) {
            return static_cast<int>(a->Priority()) > static_cast<int>(b->Priority());
        });
    }

    IViewportEditor& m_Editor;
    IViewportContext& m_Context;
    IViewportModeManager& m_Modes;
    IViewportCommandRouter& m_Commands;
    IViewportSelectionContext& m_SelectionContext;
    std::vector<std::unique_ptr<IViewportOverlay>> m_Overlays;
    std::vector<std::unique_ptr<IViewportRenderExtension>> m_Extensions;
    std::vector<std::unique_ptr<IViewportInteractionLayer>> m_LayerStorage;
    std::vector<IViewportInteractionLayer*> m_LayerPointers;
};

} // namespace

std::unique_ptr<IViewportCommandRouter> CreateCommandRouter(IViewportContext& context) {
    return std::make_unique<ViewportCommandRouterImpl>(context);
}

std::unique_ptr<IViewportSelectionContext> CreateSelectionContext(IViewportSelection& selection) {
    return std::make_unique<ViewportSelectionContextImpl>(selection);
}

std::unique_ptr<IViewportToolContext> CreateToolContext(IViewportContext& context) {
    return std::make_unique<ViewportToolContextImpl>(context);
}

std::unique_ptr<IViewportWorkspace> CreateWorkspace(
    IViewportEditor& editor,
    IViewportContext& context,
    IViewportModeManager& modes,
    IViewportCommandRouter& commands,
    IViewportSelectionContext& selectionContext)
{
    return std::make_unique<ViewportWorkspaceImpl>(
        editor, context, modes, commands, selectionContext);
}

} // namespace detail
} // namespace we::editor::viewportedit
