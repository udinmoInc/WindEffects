#include "PrefabEditor/IPrefabEditor.h"
#include "PrefabEditor/PrefabSession.h"

#include <cstdlib>
#include <string>

namespace we::editor::prefab {
namespace {

std::shared_ptr<IPrefabEditor> g_Editor;

[[nodiscard]] bool ParseGuid(std::string_view text, runtime::PrefabGuid& out) {
    const auto sep = text.find(':');
    if (sep == std::string_view::npos) {
        return false;
    }
    const auto hiStr = std::string(text.substr(0, sep));
    const auto loStr = std::string(text.substr(sep + 1));
    out.hi = std::strtoull(hiStr.c_str(), nullptr, 10);
    out.lo = std::strtoull(loStr.c_str(), nullptr, 10);
    return out.IsValid();
}

[[nodiscard]] const char* CommandLabel(runtime::PrefabCommandId id) {
    using Id = runtime::PrefabCommandId;
    switch (id) {
    case Id::CreateFromSelection: return "Create";
    case Id::SpawnAtOrigin: return "Spawn";
    case Id::ApplyInstance: return "Apply";
    case Id::RevertInstance: return "Revert";
    case Id::UpdateAllInstances: return "UpdateAll";
    case Id::BreakLink: return "BreakLink";
    case Id::RestoreLink: return "RestoreLink";
    case Id::DuplicateAsset: return "Duplicate";
    case Id::ConvertToActors: return "ConvertToActors";
    case Id::SelectSourceAsset: return "SelectSource";
    case Id::ValidateAsset: return "Validate";
    }
    return "Command";
}

class PrefabEditorImpl final : public IPrefabEditor {
public:
    explicit PrefabEditorImpl(PrefabEditorDependencies deps)
        : m_Deps(std::move(deps))
    {
        if (m_Deps.ownRuntime && !m_Deps.prefabRuntime) {
            runtime::PrefabDependencies rtDeps;
            rtDeps.scene = m_Deps.scene;
            rtDeps.config = m_Deps.config;
            rtDeps.onLog = m_Deps.onLog;
            m_OwnedRuntime = runtime::CreatePrefabRuntime(rtDeps);
            m_Deps.prefabRuntime = m_OwnedRuntime.get();
        }
        if (m_Deps.prefabRuntime && m_Deps.onLog) {
            m_Deps.onLog("PrefabEditor initialized");
        }
    }

    runtime::IPrefabRuntime& Runtime() noexcept override { return *m_Deps.prefabRuntime; }
    const runtime::IPrefabRuntime& Runtime() const noexcept override { return *m_Deps.prefabRuntime; }

    bool ExecuteCommand(
        runtime::PrefabCommandId id,
        const runtime::PrefabCommandContext& ctx) override
    {
        if (!m_Deps.prefabRuntime) {
            return false;
        }
        auto& commands = m_Deps.prefabRuntime->Manager().Commands();
        if (!commands.CanExecute(id, ctx)) {
            return false;
        }
        if (!commands.Execute(id, ctx)) {
            return false;
        }
        if (!m_Deps.recordTransaction) {
            return true;
        }

        const auto label = std::string("Prefab: ") + CommandLabel(id);
        const runtime::PrefabCommandContext captured = ctx;
        const runtime::PrefabCommandId cmdId = id;
        auto* runtimePtr = m_Deps.prefabRuntime;

        return m_Deps.recordTransaction(
            label,
            [runtimePtr, cmdId, captured]() {
                return UndoCommand(*runtimePtr, cmdId, captured);
            },
            [runtimePtr, cmdId, captured]() {
                return runtimePtr->Manager().Commands().Execute(cmdId, captured);
            });
    }

    runtime::PrefabInstanceId SpawnFromPayload(
        std::string_view payloadType,
        std::string_view payloadData,
        const we::math::Vec3& worldPosition,
        std::uint64_t parentEntityId) override
    {
        if (!m_Deps.prefabRuntime) {
            return {};
        }
        auto& mgr = m_Deps.prefabRuntime->Manager();
        runtime::PrefabGuid guid{};

        if (payloadType == "prefab-guid" || payloadType == "prefab") {
            if (!ParseGuid(payloadData, guid)) {
                if (auto asset = mgr.Registry().FindByPath(payloadData)) {
                    guid = asset->GetGuid();
                }
            }
        } else if (payloadType == "asset") {
            auto asset = mgr.Registry().FindByPath(payloadData);
            if (!asset) {
                asset = mgr.Importer().Import(payloadData);
            }
            if (asset) {
                guid = asset->GetGuid();
            }
        }

        if (!guid.IsValid()) {
            return {};
        }

        runtime::PrefabCommandContext ctx;
        ctx.spawn.prefab = guid;
        ctx.spawn.position = worldPosition;
        ctx.spawn.parentEntityId = parentEntityId;
        if (!ExecuteCommand(runtime::PrefabCommandId::SpawnAtOrigin, ctx)) {
            return {};
        }
        auto ids = mgr.Spawner().ListInstances(guid);
        return ids.empty() ? runtime::PrefabInstanceId{} : ids.back();
    }

    bool ConvertSelectionToPrefab(
        std::uint64_t rootEntityId,
        std::string_view name,
        std::string_view assetPath) override
    {
        runtime::PrefabCommandContext ctx;
        ctx.rootEntityId = rootEntityId;
        ctx.name = std::string(name);
        ctx.path = std::string(assetPath);
        return ExecuteCommand(runtime::PrefabCommandId::CreateFromSelection, ctx);
    }

    void Tick(float) override {}

    void Shutdown() override {
        if (m_Deps.onLog) {
            m_Deps.onLog("PrefabEditor shutdown");
        }
        if (m_OwnedRuntime) {
            m_OwnedRuntime->Shutdown();
            m_OwnedRuntime.reset();
            m_Deps.prefabRuntime = nullptr;
        }
    }

private:
    static bool UndoCommand(
        runtime::IPrefabRuntime& runtime,
        runtime::PrefabCommandId id,
        const runtime::PrefabCommandContext& ctx)
    {
        auto& mgr = runtime.Manager();
        using Id = runtime::PrefabCommandId;
        switch (id) {
        case Id::SpawnAtOrigin: {
            auto ids = mgr.Spawner().ListInstances(ctx.spawn.prefab);
            if (!ids.empty()) {
                return mgr.Spawner().Despawn(ids.back());
            }
            return false;
        }
        case Id::BreakLink: {
            runtime::PrefabGuid guid = ctx.asset;
            if (!guid.IsValid()) {
                if (auto* inst = mgr.Spawner().FindInstance(ctx.instance)) {
                    guid = inst->GetSourceGuid();
                }
            }
            return mgr.RestoreLink(ctx.instance, guid);
        }
        case Id::RestoreLink:
            return mgr.BreakLink(ctx.instance);
        case Id::CreateFromSelection: {
            if (auto asset = mgr.Registry().FindByPath(ctx.path)) {
                return mgr.Registry().Unregister(asset->GetGuid());
            }
            return false;
        }
        case Id::ApplyInstance:
        case Id::RevertInstance:
        case Id::UpdateAllInstances:
        case Id::DuplicateAsset:
        case Id::ConvertToActors:
        case Id::SelectSourceAsset:
        case Id::ValidateAsset:
            return true;
        }
        return false;
    }

    PrefabEditorDependencies m_Deps;
    std::unique_ptr<runtime::IPrefabRuntime> m_OwnedRuntime;
};

} // namespace

std::unique_ptr<IPrefabEditor> CreatePrefabEditor(const PrefabEditorDependencies& deps) {
    if (!deps.prefabRuntime && !deps.ownRuntime) {
        return nullptr;
    }
    return std::make_unique<PrefabEditorImpl>(deps);
}

void PrefabSession::Install(std::shared_ptr<IPrefabEditor> editor) {
    g_Editor = std::move(editor);
}

void PrefabSession::Clear() noexcept {
    g_Editor.reset();
}

IPrefabEditor* PrefabSession::Editor() noexcept {
    return g_Editor.get();
}

runtime::IPrefabRuntime* PrefabSession::Runtime() noexcept {
    return g_Editor ? &g_Editor->Runtime() : nullptr;
}

std::shared_ptr<IPrefabEditor> PrefabSession::EditorShared() noexcept {
    return g_Editor;
}

bool PrefabSession::IsInstalled() noexcept {
    return static_cast<bool>(g_Editor);
}

} // namespace we::editor::prefab
