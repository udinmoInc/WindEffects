#include "UndoInternal.h"

namespace we::editor::undo {
namespace detail {

class PropertyTransactionHookAdapter final : public property::IPropertyTransactionHook {
public:
    explicit PropertyTransactionHookAdapter(ITransactionManager& manager)
        : m_Manager(manager)
    {}

    void OnPreEdit(property::PropertyChangeEvent& event) override {
        if (event.cancelled || m_Manager.IsApplying() || m_Manager.IsRecordingSuspended()) {
            return;
        }
        (void)event;
    }

    void OnCommit(
        const property::PropertyChangeEvent& event,
        const serialization::BinaryDiff* diff,
        const serialization::Snapshot* snapshot) override
    {
        (void)diff;
        (void)snapshot;
        if (m_Manager.IsApplying() || m_Manager.IsRecordingSuspended()) {
            return;
        }
        if (event.path.empty() || event.instances.empty()) {
            return;
        }
        std::vector<std::uint8_t> before = event.beforeBytes;
        std::vector<std::uint8_t> after = event.afterBytes;
        if (before.empty() && after.empty()) {
            return;
        }
        (void)m_Manager.RecordPropertyChange(
            event.typeId,
            event.instances,
            event.path,
            std::move(before),
            std::move(after),
            event.path);
    }

    void OnPostEdit(const property::PropertyChangeEvent& event) override { (void)event; }

private:
    ITransactionManager& m_Manager;
};

class EditorWorldUndoHookAdapter final : public world::IEditorWorldHook {
public:
    explicit EditorWorldUndoHookAdapter(ITransactionManager& manager)
        : m_Manager(manager)
    {}

    void OnWorldCreated(world::IWorld& world) override {
        (void)world;
        // World key assignment is left to the editor host (multi-world).
    }

    void OnWorldDestroyed(world::IWorld& world) override { (void)world; }

    void OnSelectionChanged(world::IWorld& world, world::ActorHandle selected) override {
        (void)world;
        (void)selected;
    }

    void OnUndoRedoBoundary(world::IWorld& world) override {
        (void)world;
        // Hosts may call SuspendRecording around external batch restores.
        m_Manager.SuspendRecording(false);
    }

private:
    ITransactionManager& m_Manager;
};

class UndoRuntimeImpl final : public IUndoRuntime {
public:
    explicit UndoRuntimeImpl(UndoDependencies deps)
        : m_Deps(std::move(deps))
    {
        if (m_Deps.typeRegistry) {
            m_Registry = m_Deps.typeRegistry;
        } else {
            m_Registry = &reflection::GetTypeRegistry();
        }

        std::shared_ptr<IMergePolicy> policy;
        if (m_Deps.installDefaultMergePolicy) {
            policy = CreateMergePolicy(
                m_Deps.history.mergePolicy,
                m_Deps.history.mergeWindowMs);
        } else {
            policy = CreateMergePolicy(MergePolicyKind::Never);
        }

        m_Manager = CreateTransactionManager(
            *m_Registry,
            m_Deps.serializer,
            m_Deps.history,
            std::move(policy));

        if (m_Deps.onLog) {
            m_Deps.onLog("UndoRuntime: constructed");
        }
    }

    ~UndoRuntimeImpl() override { Shutdown(); }

    [[nodiscard]] ITransactionManager& Manager() noexcept override { return *m_Manager; }
    [[nodiscard]] const ITransactionManager& Manager() const noexcept override { return *m_Manager; }

    [[nodiscard]] std::shared_ptr<property::IPropertyTransactionHook>
    MakePropertyTransactionHook() override {
        return std::make_shared<PropertyTransactionHookAdapter>(*m_Manager);
    }

    [[nodiscard]] std::shared_ptr<world::IEditorWorldHook> MakeEditorWorldHook() override {
        return std::make_shared<EditorWorldUndoHookAdapter>(*m_Manager);
    }

    void Shutdown() override {
        if (m_Manager) {
            m_Manager->History().Clear();
        }
        m_Manager.reset();
        m_Registry = nullptr;
    }

private:
    UndoDependencies m_Deps;
    reflection::ITypeRegistry* m_Registry = nullptr;
    std::unique_ptr<ITransactionManager> m_Manager;
};

} // namespace detail

std::unique_ptr<IUndoRuntime> CreateUndoRuntime(UndoDependencies deps) {
    return std::make_unique<detail::UndoRuntimeImpl>(std::move(deps));
}

} // namespace we::editor::undo
