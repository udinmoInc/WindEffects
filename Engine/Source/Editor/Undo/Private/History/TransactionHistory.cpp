#include "UndoInternal.h"

#include <algorithm>

namespace we::editor::undo {
namespace detail {

class TransactionHistoryImpl final : public ITransactionHistory {
public:
    explicit TransactionHistoryImpl(HistoryConfig config) : m_Config(std::move(config)) {}

    void Configure(const HistoryConfig& config) override { m_Config = config; }
    [[nodiscard]] const HistoryConfig& GetConfig() const noexcept override { return m_Config; }

    [[nodiscard]] std::size_t UndoCount() const noexcept override { return m_Undo.size(); }
    [[nodiscard]] std::size_t RedoCount() const noexcept override { return m_Redo.size(); }
    [[nodiscard]] std::uint64_t TrackedBytes() const noexcept override { return m_Bytes; }
    [[nodiscard]] bool CanUndo() const noexcept override { return !m_Undo.empty(); }
    [[nodiscard]] bool CanRedo() const noexcept override { return !m_Redo.empty(); }

    [[nodiscard]] std::span<const TransactionPtr> UndoStack() const noexcept override { return m_Undo; }
    [[nodiscard]] std::span<const TransactionPtr> RedoStack() const noexcept override { return m_Redo; }

    [[nodiscard]] TransactionPtr PeekUndo() const override {
        return m_Undo.empty() ? nullptr : m_Undo.back();
    }
    [[nodiscard]] TransactionPtr PeekRedo() const override {
        return m_Redo.empty() ? nullptr : m_Redo.back();
    }

    void PushCommitted(TransactionPtr transaction) override {
        if (!transaction) {
            return;
        }
        if (m_Config.clearRedoOnNewTransaction) {
            ClearRedo();
        }
        m_Bytes += transaction->EstimatedBytes();
        m_Undo.push_back(std::move(transaction));
        TrimToBudget();
        PublishDepth();
    }

    [[nodiscard]] bool UndoOne() override {
        if (m_Undo.empty()) {
            return false;
        }
        auto txn = m_Undo.back();
        m_Undo.pop_back();
        if (!txn || !txn->Undo()) {
            if (txn) {
                m_Undo.push_back(std::move(txn));
            }
            return false;
        }
        m_Redo.push_back(std::move(txn));
        UndoDiagnostics::Get().OnUndo();
        PublishDepth();
        return true;
    }

    [[nodiscard]] bool RedoOne() override {
        if (m_Redo.empty()) {
            return false;
        }
        auto txn = m_Redo.back();
        m_Redo.pop_back();
        if (!txn || !txn->Redo()) {
            if (txn) {
                m_Redo.push_back(std::move(txn));
            }
            return false;
        }
        m_Undo.push_back(std::move(txn));
        UndoDiagnostics::Get().OnRedo();
        PublishDepth();
        return true;
    }

    [[nodiscard]] std::size_t UndoMany(std::size_t count) override {
        std::size_t n = 0;
        while (n < count && UndoOne()) {
            ++n;
        }
        return n;
    }

    [[nodiscard]] std::size_t RedoMany(std::size_t count) override {
        std::size_t n = 0;
        while (n < count && RedoOne()) {
            ++n;
        }
        return n;
    }

    [[nodiscard]] bool JumpToCheckpoint(CheckpointId id) override {
        if (!id.IsValid()) {
            return false;
        }
        // Undo until checkpoint is the tip (checkpoint remains on undo stack).
        while (!m_Undo.empty()) {
            const auto& tip = m_Undo.back();
            if (tip && tip->IsCheckpoint() && tip->Id().value == id.value) {
                return true;
            }
            if (!UndoOne()) {
                return false;
            }
        }
        // Redo until checkpoint is tip.
        while (!m_Redo.empty()) {
            if (!RedoOne()) {
                return false;
            }
            const auto& tip = m_Undo.empty() ? TransactionPtr{} : m_Undo.back();
            if (tip && tip->IsCheckpoint() && tip->Id().value == id.value) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] CheckpointId MarkCheckpoint(std::string_view label) override {
        TransactionDescriptor desc;
        desc.label = label.empty() ? "Checkpoint" : std::string(label);
        desc.kind = TransactionKind::Checkpoint;
        desc.canMerge = false;
        desc.isCheckpoint = true;
        auto txn = std::make_shared<TransactionImpl>(
            TransactionId{++m_CheckpointSeq},
            std::move(desc),
            NowMs());
        txn->SetState(TransactionState::Committed);
        CheckpointId id{txn->Id().value};
        m_Checkpoints.push_back(id);
        PushCommitted(std::move(txn));
        m_SavedAtDepth = m_Undo.size();
        return id;
    }

    [[nodiscard]] std::vector<CheckpointId> ListCheckpoints() const override { return m_Checkpoints; }

    [[nodiscard]] bool IsDirty() const noexcept override {
        return m_Undo.size() != m_SavedAtDepth;
    }

    void MarkSaved() override { m_SavedAtDepth = m_Undo.size(); }

    void Clear() override {
        m_Undo.clear();
        m_Redo.clear();
        m_Checkpoints.clear();
        m_Bytes = 0;
        m_SavedAtDepth = 0;
        PublishDepth();
    }

    void ClearRedo() override {
        for (const auto& txn : m_Redo) {
            if (txn) {
                m_Bytes = m_Bytes >= txn->EstimatedBytes() ? m_Bytes - txn->EstimatedBytes() : 0;
            }
        }
        m_Redo.clear();
        PublishDepth();
    }

    void TrimToBudget() override {
        while ((!m_Undo.empty()) &&
               ((m_Config.maxTransactions > 0 && m_Undo.size() > m_Config.maxTransactions) ||
                (m_Config.maxBytes > 0 && m_Bytes > m_Config.maxBytes))) {
            auto& oldest = m_Undo.front();
            if (oldest) {
                m_Bytes = m_Bytes >= oldest->EstimatedBytes() ? m_Bytes - oldest->EstimatedBytes() : 0;
            }
            m_Undo.erase(m_Undo.begin());
            if (m_SavedAtDepth > 0) {
                --m_SavedAtDepth;
            }
        }
        PublishDepth();
    }

private:
    void PublishDepth() {
        UndoDiagnostics::Get().SetHistoryDepth(m_Undo.size(), m_Redo.size(), 0);
    }

    HistoryConfig m_Config{};
    std::vector<TransactionPtr> m_Undo;
    std::vector<TransactionPtr> m_Redo;
    std::vector<CheckpointId> m_Checkpoints;
    std::uint64_t m_Bytes = 0;
    std::size_t m_SavedAtDepth = 0;
    std::uint64_t m_CheckpointSeq = 0;
};

[[nodiscard]] std::unique_ptr<ITransactionHistory> CreateTransactionHistory(HistoryConfig config) {
    return std::make_unique<TransactionHistoryImpl>(std::move(config));
}

} // namespace detail
} // namespace we::editor::undo
