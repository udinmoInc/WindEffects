#include "UndoInternal.h"

#include <algorithm>

namespace we::editor::undo {
namespace detail {

class TransactionManagerImpl final : public ITransactionManager {
public:
    TransactionManagerImpl(
        reflection::ITypeRegistry& registry,
        serialization::ISerializer* serializer,
        HistoryConfig historyConfig,
        std::shared_ptr<IMergePolicy> mergePolicy)
        : m_Registry(registry)
        , m_Serializer(serializer)
        , m_History(CreateTransactionHistory(std::move(historyConfig)))
        , m_MergePolicy(std::move(mergePolicy))
    {}

    [[nodiscard]] ITransactionHistory& History() noexcept override { return *m_History; }
    [[nodiscard]] const ITransactionHistory& History() const noexcept override { return *m_History; }

    void SetMergePolicy(std::shared_ptr<IMergePolicy> policy) override {
        m_MergePolicy = std::move(policy);
    }

    void AddListener(std::shared_ptr<ITransactionListener> listener) override {
        if (listener) {
            m_Listeners.push_back(std::move(listener));
        }
    }

    void RemoveListener(const ITransactionListener* listener) override {
        m_Listeners.erase(
            std::remove_if(
                m_Listeners.begin(),
                m_Listeners.end(),
                [listener](const std::shared_ptr<ITransactionListener>& p) {
                    return p.get() == listener;
                }),
            m_Listeners.end());
    }

    [[nodiscard]] TransactionId BeginTransaction(const TransactionDescriptor& desc) override {
        if (m_Suspended || m_Applying) {
            return {};
        }
        TransactionDescriptor copy = desc;
        if (!copy.worldKey.IsValid()) {
            copy.worldKey = m_ActiveWorld;
        }
        auto txn = std::make_shared<TransactionImpl>(
            TransactionId{++m_NextId},
            std::move(copy),
            NowMs());
        m_Open.push_back(txn);
        NotifyBegun(*txn);
        UndoDiagnostics::Get().SetHistoryDepth(
            m_History->UndoCount(), m_History->RedoCount(), m_Open.size());
        return txn->Id();
    }

    [[nodiscard]] bool EndTransaction() override {
        if (m_Open.empty()) {
            return false;
        }
        auto txn = m_Open.back();
        m_Open.pop_back();
        if (!txn) {
            return false;
        }
        txn->SetState(TransactionState::Committed);
        if (m_History->GetConfig().compressOnCommit) {
            txn->Compress();
        }

        if (!m_Open.empty()) {
            // Nested: fold commands into parent transaction.
            FoldIntoParent(*txn);
            NotifyCommitted(*txn);
            UndoDiagnostics::Get().SetHistoryDepth(
                m_History->UndoCount(), m_History->RedoCount(), m_Open.size());
            return true;
        }

        // Try merge with previous undo tip.
        if (m_MergePolicy && txn->CanMerge()) {
            if (auto tip = m_History->PeekUndo()) {
                if (m_MergePolicy->CanMergeTransactions(*tip, *txn) && tip->CommandCount() == 1 &&
                    txn->CommandCount() == 1) {
                    if (tip->Commands()[0] && txn->Commands()[0] &&
                        tip->Commands()[0]->TryMergeWith(*txn->Commands()[0])) {
                        UndoDiagnostics::Get().OnMerge();
                        UndoDiagnostics::Get().OnCommit(txn->EstimatedBytes());
                        NotifyCommitted(*txn);
                        NotifyHistory();
                        return true;
                    }
                }
            }
        }

        m_History->PushCommitted(txn);
        UndoDiagnostics::Get().OnCommit(txn->EstimatedBytes());
        NotifyCommitted(*txn);
        NotifyHistory();
        UndoDiagnostics::Get().SetHistoryDepth(
            m_History->UndoCount(), m_History->RedoCount(), m_Open.size());
        return true;
    }

    [[nodiscard]] bool CancelTransaction() override {
        if (m_Open.empty()) {
            return false;
        }
        auto txn = m_Open.back();
        m_Open.pop_back();
        if (!txn) {
            return false;
        }
        // Reverse commands already applied inside the open transaction.
        (void)txn->Undo();
        txn->SetState(TransactionState::Cancelled);
        UndoDiagnostics::Get().OnCancel();
        NotifyCancelled(*txn);
        NotifyHistory();
        return true;
    }

    [[nodiscard]] bool HasOpenTransaction() const noexcept override { return !m_Open.empty(); }
    [[nodiscard]] std::uint32_t NestingDepth() const noexcept override {
        return static_cast<std::uint32_t>(m_Open.size());
    }
    [[nodiscard]] ITransaction* ActiveTransaction() noexcept override {
        return m_Open.empty() ? nullptr : m_Open.back().get();
    }

    [[nodiscard]] bool Execute(TransactionCommandPtr command) override {
        if (!command || m_Suspended || m_Applying) {
            return false;
        }
        const bool autoWrap = m_Open.empty();
        if (autoWrap) {
            TransactionDescriptor desc;
            desc.label = std::string(command->Label());
            desc.kind = command->Kind();
            desc.worldKey = m_ActiveWorld;
            (void)BeginTransaction(desc);
        }
        // Apply forward now.
        if (!command->Redo()) {
            if (autoWrap) {
                (void)CancelTransaction();
            }
            return false;
        }
        if (!m_Open.empty()) {
            m_Open.back()->AddCommand(std::move(command));
        }
        if (autoWrap) {
            return EndTransaction();
        }
        return true;
    }

    [[nodiscard]] bool RecordPropertyChange(
        reflection::TypeId typeId,
        std::span<void* const> instances,
        std::string_view path,
        std::vector<std::uint8_t> beforeBytes,
        std::vector<std::uint8_t> afterBytes,
        std::string_view label) override
    {
        if (m_Suspended || m_Applying) {
            return false;
        }
        if (beforeBytes == afterBytes) {
            return true;
        }
        std::vector<void*> copies(instances.begin(), instances.end());
        auto cmd = MakePropertyChangeCommand(
            m_Registry,
            typeId,
            std::move(copies),
            std::string(path),
            std::move(beforeBytes),
            std::move(afterBytes),
            std::string(label));
        // Change already applied by caller (PropertyEditor) — do not Redo again.
        const bool autoWrap = m_Open.empty();
        if (autoWrap) {
            TransactionDescriptor desc;
            desc.label = std::string(cmd->Label());
            desc.kind = TransactionKind::PropertyChange;
            desc.worldKey = m_ActiveWorld;
            (void)BeginTransaction(desc);
        }
        if (!m_Open.empty()) {
            m_Open.back()->AddCommand(std::move(cmd));
            UndoDiagnostics::Get().OnPropertyEdit();
        }
        if (autoWrap) {
            return EndTransaction();
        }
        return true;
    }

    [[nodiscard]] bool RecordSnapshotSwap(
        reflection::TypeId typeId,
        void* instance,
        serialization::Snapshot before,
        serialization::Snapshot after,
        std::string_view label,
        TransactionKind kind) override
    {
        if (m_Suspended || m_Applying || !instance) {
            return false;
        }
        auto cmd = MakeSnapshotSwapCommand(
            m_Serializer,
            m_Registry,
            typeId,
            instance,
            std::move(before),
            std::move(after),
            std::string(label),
            kind);
        const bool autoWrap = m_Open.empty();
        if (autoWrap) {
            TransactionDescriptor desc;
            desc.label = std::string(label);
            desc.kind = kind;
            desc.worldKey = m_ActiveWorld;
            (void)BeginTransaction(desc);
        }
        if (!m_Open.empty()) {
            m_Open.back()->AddCommand(std::move(cmd));
        }
        if (autoWrap) {
            return EndTransaction();
        }
        return true;
    }

    [[nodiscard]] bool RecordCustom(
        std::string_view label,
        TransactionKind kind,
        std::string mergeKey,
        std::function<bool()> undoFn,
        std::function<bool()> redoFn,
        std::uint64_t estimatedBytes) override
    {
        auto cmd = MakeCustomCommand(
            std::string(label),
            kind,
            std::move(mergeKey),
            std::move(undoFn),
            std::move(redoFn),
            estimatedBytes);
        // Custom: assume redo already applied by caller unless we auto-execute.
        const bool autoWrap = m_Open.empty();
        if (autoWrap) {
            TransactionDescriptor desc;
            desc.label = std::string(label);
            desc.kind = kind;
            desc.worldKey = m_ActiveWorld;
            (void)BeginTransaction(desc);
        }
        if (!m_Open.empty()) {
            m_Open.back()->AddCommand(std::move(cmd));
        }
        if (autoWrap) {
            return EndTransaction();
        }
        return true;
    }

    [[nodiscard]] bool Undo() override {
        if (HasOpenTransaction()) {
            return false;
        }
        m_Applying = true;
        const bool ok = m_History->UndoOne();
        m_Applying = false;
        if (ok) {
            NotifyHistory();
            for (auto it = m_Listeners.rbegin(); it != m_Listeners.rend(); ++it) {
                if (*it) {
                    if (auto tip = m_History->PeekRedo()) {
                        (*it)->OnTransactionUndone(*tip);
                    }
                }
            }
        }
        return ok;
    }

    [[nodiscard]] bool Redo() override {
        if (HasOpenTransaction()) {
            return false;
        }
        m_Applying = true;
        const bool ok = m_History->RedoOne();
        m_Applying = false;
        if (ok) {
            NotifyHistory();
            for (auto& listener : m_Listeners) {
                if (listener) {
                    if (auto tip = m_History->PeekUndo()) {
                        listener->OnTransactionRedone(*tip);
                    }
                }
            }
        }
        return ok;
    }

    [[nodiscard]] bool CanUndo() const noexcept override {
        return !HasOpenTransaction() && m_History->CanUndo();
    }
    [[nodiscard]] bool CanRedo() const noexcept override {
        return !HasOpenTransaction() && m_History->CanRedo();
    }

    [[nodiscard]] bool IsApplying() const noexcept override { return m_Applying; }
    void SuspendRecording(bool suspend) override { m_Suspended = suspend; }
    [[nodiscard]] bool IsRecordingSuspended() const noexcept override { return m_Suspended; }

    void SetActiveWorldKey(UndoWorldKey key) override { m_ActiveWorld = key; }
    [[nodiscard]] UndoWorldKey ActiveWorldKey() const noexcept override { return m_ActiveWorld; }

private:
    void FoldIntoParent(TransactionImpl& nested) {
        if (m_Open.empty()) {
            return;
        }
        std::vector<TransactionCommandPtr> children(
            nested.Commands().begin(), nested.Commands().end());
        m_Open.back()->AddCommand(MakeCompositeCommand(
            std::string(nested.Label()), nested.Kind(), std::move(children)));
    }

    void NotifyBegun(const ITransaction& txn) {
        for (auto& listener : m_Listeners) {
            if (listener) {
                listener->OnTransactionBegun(txn);
            }
        }
    }
    void NotifyCommitted(const ITransaction& txn) {
        for (auto& listener : m_Listeners) {
            if (listener) {
                listener->OnTransactionCommitted(txn);
            }
        }
    }
    void NotifyCancelled(const ITransaction& txn) {
        for (auto& listener : m_Listeners) {
            if (listener) {
                listener->OnTransactionCancelled(txn);
            }
        }
    }
    void NotifyHistory() {
        for (auto& listener : m_Listeners) {
            if (listener) {
                listener->OnHistoryChanged();
            }
        }
    }

    reflection::ITypeRegistry& m_Registry;
    serialization::ISerializer* m_Serializer = nullptr;
    std::unique_ptr<ITransactionHistory> m_History;
    std::shared_ptr<IMergePolicy> m_MergePolicy;
    std::vector<std::shared_ptr<TransactionImpl>> m_Open;
    std::vector<std::shared_ptr<ITransactionListener>> m_Listeners;
    UndoWorldKey m_ActiveWorld{};
    std::uint64_t m_NextId = 0;
    bool m_Applying = false;
    bool m_Suspended = false;
};

// Fix EndTransaction nested path — remove broken code by rewriting EndTransaction cleanly in a follow-up if needed.

[[nodiscard]] std::unique_ptr<ITransactionManager> CreateTransactionManager(
    reflection::ITypeRegistry& registry,
    serialization::ISerializer* serializer,
    HistoryConfig historyConfig,
    std::shared_ptr<IMergePolicy> mergePolicy)
{
    return std::make_unique<TransactionManagerImpl>(
        registry, serializer, std::move(historyConfig), std::move(mergePolicy));
}

} // namespace detail
} // namespace we::editor::undo
