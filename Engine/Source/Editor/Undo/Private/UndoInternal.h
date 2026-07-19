#pragma once

#include "Undo/IMergePolicy.h"
#include "Undo/IScopedTransaction.h"
#include "Undo/ITransaction.h"
#include "Undo/ITransactionCommand.h"
#include "Undo/ITransactionHistory.h"
#include "Undo/ITransactionListener.h"
#include "Undo/ITransactionManager.h"
#include "Undo/IUndoRuntime.h"
#include "Undo/TransactionCommands.h"
#include "Undo/UndoDiagnostics.h"
#include "Undo/UndoTypes.h"

#include "PropertyEditor/IPropertyTransactionHook.h"
#include "Reflection/ITypeRegistry.h"
#include "Reflection/PropertyPath.h"
#include "Reflection/ReflectionOps.h"
#include "Serialization/Delta.h"
#include "Serialization/ISerializer.h"
#include "World/Integration/IWorldHooks.h"

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace we::editor::undo {
namespace detail {

[[nodiscard]] inline std::uint64_t NowMs() {
    using clock = std::chrono::steady_clock;
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(clock::now().time_since_epoch()).count());
}

class PropertyChangeCommand final : public ITransactionCommand {
public:
    PropertyChangeCommand(
        reflection::ITypeRegistry& registry,
        reflection::TypeId typeId,
        std::vector<void*> instances,
        std::string path,
        std::vector<std::uint8_t> beforeBytes,
        std::vector<std::uint8_t> afterBytes,
        std::string label)
        : m_Registry(registry)
        , m_TypeId(typeId)
        , m_Instances(std::move(instances))
        , m_Path(std::move(path))
        , m_Before(std::move(beforeBytes))
        , m_After(std::move(afterBytes))
        , m_Label(label.empty() ? ("Edit " + m_Path) : std::move(label))
        , m_MergeKey(m_Path)
    {}

    [[nodiscard]] std::string_view Label() const noexcept override { return m_Label; }
    [[nodiscard]] TransactionKind Kind() const noexcept override {
        return TransactionKind::PropertyChange;
    }
    [[nodiscard]] std::uint64_t EstimatedBytes() const noexcept override {
        return m_Before.size() + m_After.size() + m_Path.size();
    }
    [[nodiscard]] std::string_view MergeKey() const noexcept override { return m_MergeKey; }

    [[nodiscard]] bool Redo() override { return Apply(m_After); }
    [[nodiscard]] bool Undo() override { return Apply(m_Before); }

    [[nodiscard]] bool TryMergeWith(const ITransactionCommand& newer) override {
        const auto* other = dynamic_cast<const PropertyChangeCommand*>(&newer);
        if (!other) {
            return false;
        }
        if (other->m_TypeId != m_TypeId || other->m_Path != m_Path ||
            other->m_Instances != m_Instances) {
            return false;
        }
        // Keep original before; adopt newest after.
        m_After = other->m_After;
        return true;
    }

    void Compress() override {
        // Leaf bytes are already minimal; nothing further unless equal.
        if (m_Before == m_After) {
            m_Before.clear();
            m_After.clear();
        }
    }

private:
    [[nodiscard]] bool Apply(const std::vector<std::uint8_t>& bytes) const {
        if (bytes.empty() && m_Before.empty() && m_After.empty()) {
            return true;
        }
        reflection::ReflectionPatch patch;
        patch.typeId = m_TypeId;
        reflection::ReflectionPatchEntry entry;
        entry.path = m_Path;
        entry.bytes = bytes;
        patch.entries.push_back(std::move(entry));
        bool ok = true;
        for (void* instance : m_Instances) {
            if (!instance) {
                ok = false;
                continue;
            }
            if (!reflection::ApplyPatch(m_Registry, instance, patch)) {
                // Fallback: property path raw set
                if (!reflection::SetPropertyPathRaw(
                        m_Registry,
                        m_TypeId,
                        instance,
                        m_Path,
                        bytes.data(),
                        bytes.size())) {
                    ok = false;
                }
            }
        }
        return ok;
    }

    reflection::ITypeRegistry& m_Registry;
    reflection::TypeId m_TypeId;
    std::vector<void*> m_Instances;
    std::string m_Path;
    std::vector<std::uint8_t> m_Before;
    std::vector<std::uint8_t> m_After;
    std::string m_Label;
    std::string m_MergeKey;
};

class SnapshotSwapCommand final : public ITransactionCommand {
public:
    SnapshotSwapCommand(
        serialization::ISerializer* serializer,
        reflection::ITypeRegistry& registry,
        reflection::TypeId typeId,
        void* instance,
        serialization::Snapshot before,
        serialization::Snapshot after,
        std::string label,
        TransactionKind kind)
        : m_Serializer(serializer)
        , m_Registry(registry)
        , m_TypeId(typeId)
        , m_Instance(instance)
        , m_Before(std::move(before))
        , m_After(std::move(after))
        , m_Label(std::move(label))
        , m_Kind(kind)
    {}

    [[nodiscard]] std::string_view Label() const noexcept override { return m_Label; }
    [[nodiscard]] TransactionKind Kind() const noexcept override { return m_Kind; }
    [[nodiscard]] std::uint64_t EstimatedBytes() const noexcept override {
        return m_Before.bytes.size() + m_After.bytes.size();
    }

    [[nodiscard]] bool Redo() override { return Restore(m_After); }
    [[nodiscard]] bool Undo() override { return Restore(m_Before); }

private:
    [[nodiscard]] bool Restore(const serialization::Snapshot& snap) const {
        if (!m_Instance) {
            return false;
        }
        if (m_Serializer) {
            return serialization::RestoreSnapshot(*m_Serializer, m_TypeId, m_Instance, snap);
        }
        // Without serializer, treat bytes as a Reflection full-object patch is not available;
        // require serializer for snapshot commands.
        (void)m_Registry;
        return false;
    }

    serialization::ISerializer* m_Serializer = nullptr;
    reflection::ITypeRegistry& m_Registry;
    reflection::TypeId m_TypeId;
    void* m_Instance = nullptr;
    serialization::Snapshot m_Before;
    serialization::Snapshot m_After;
    std::string m_Label;
    TransactionKind m_Kind = TransactionKind::Generic;
};

class CustomCommand final : public ITransactionCommand {
public:
    CustomCommand(
        std::string label,
        TransactionKind kind,
        std::string mergeKey,
        std::function<bool()> undoFn,
        std::function<bool()> redoFn,
        std::uint64_t estimatedBytes)
        : m_Label(std::move(label))
        , m_Kind(kind)
        , m_MergeKey(std::move(mergeKey))
        , m_Undo(std::move(undoFn))
        , m_Redo(std::move(redoFn))
        , m_Bytes(estimatedBytes)
    {}

    [[nodiscard]] std::string_view Label() const noexcept override { return m_Label; }
    [[nodiscard]] TransactionKind Kind() const noexcept override { return m_Kind; }
    [[nodiscard]] std::uint64_t EstimatedBytes() const noexcept override { return m_Bytes; }
    [[nodiscard]] std::string_view MergeKey() const noexcept override { return m_MergeKey; }

    [[nodiscard]] bool Redo() override { return m_Redo ? m_Redo() : false; }
    [[nodiscard]] bool Undo() override { return m_Undo ? m_Undo() : false; }

private:
    std::string m_Label;
    TransactionKind m_Kind;
    std::string m_MergeKey;
    std::function<bool()> m_Undo;
    std::function<bool()> m_Redo;
    std::uint64_t m_Bytes = 0;
};

class CompositeCommand final : public ITransactionCommand {
public:
    CompositeCommand(std::string label, TransactionKind kind, std::vector<TransactionCommandPtr> children)
        : m_Label(std::move(label))
        , m_Kind(kind)
        , m_Children(std::move(children))
    {}

    [[nodiscard]] std::string_view Label() const noexcept override { return m_Label; }
    [[nodiscard]] TransactionKind Kind() const noexcept override { return m_Kind; }
    [[nodiscard]] std::uint64_t EstimatedBytes() const noexcept override {
        std::uint64_t total = 0;
        for (const auto& child : m_Children) {
            if (child) {
                total += child->EstimatedBytes();
            }
        }
        return total;
    }

    [[nodiscard]] bool Redo() override {
        for (auto& child : m_Children) {
            if (child && !child->Redo()) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] bool Undo() override {
        for (auto it = m_Children.rbegin(); it != m_Children.rend(); ++it) {
            if (*it && !(*it)->Undo()) {
                return false;
            }
        }
        return true;
    }

    void Compress() override {
        for (auto& child : m_Children) {
            if (child) {
                child->Compress();
            }
        }
    }

private:
    std::string m_Label;
    TransactionKind m_Kind;
    std::vector<TransactionCommandPtr> m_Children;
};

class TransactionImpl final : public ITransaction {
public:
    TransactionImpl(TransactionId id, TransactionDescriptor desc, std::uint64_t timestampMs)
        : m_Id(id)
        , m_Desc(std::move(desc))
        , m_TimestampMs(timestampMs)
    {}

    [[nodiscard]] TransactionId Id() const noexcept override { return m_Id; }
    [[nodiscard]] std::string_view Label() const noexcept override { return m_Desc.label; }
    [[nodiscard]] TransactionKind Kind() const noexcept override { return m_Desc.kind; }
    [[nodiscard]] TransactionState State() const noexcept override { return m_State; }
    [[nodiscard]] UndoWorldKey WorldKey() const noexcept override { return m_Desc.worldKey; }
    [[nodiscard]] std::uint64_t TimestampMs() const noexcept override { return m_TimestampMs; }
    [[nodiscard]] std::uint64_t EstimatedBytes() const noexcept override {
        std::uint64_t total = 0;
        for (const auto& cmd : m_Commands) {
            if (cmd) {
                total += cmd->EstimatedBytes();
            }
        }
        return total;
    }
    [[nodiscard]] std::size_t CommandCount() const noexcept override { return m_Commands.size(); }
    [[nodiscard]] std::span<const TransactionCommandPtr> Commands() const noexcept override {
        return m_Commands;
    }
    [[nodiscard]] bool CanMerge() const noexcept override { return m_Desc.canMerge; }
    [[nodiscard]] bool IsCheckpoint() const noexcept override { return m_Desc.isCheckpoint; }

    void AddCommand(TransactionCommandPtr command) override {
        if (!command) {
            return;
        }
        if (!m_Commands.empty() && m_Commands.back() &&
            m_Commands.back()->TryMergeWith(*command)) {
            UndoDiagnostics::Get().OnMerge();
            return;
        }
        m_Commands.push_back(std::move(command));
        UndoDiagnostics::Get().OnCommand();
    }

    [[nodiscard]] bool Undo() override {
        for (auto it = m_Commands.rbegin(); it != m_Commands.rend(); ++it) {
            if (*it && !(*it)->Undo()) {
                return false;
            }
        }
        m_State = TransactionState::Undone;
        return true;
    }

    [[nodiscard]] bool Redo() override {
        for (auto& cmd : m_Commands) {
            if (cmd && !cmd->Redo()) {
                return false;
            }
        }
        m_State = TransactionState::Redone;
        return true;
    }

    void Compress() override {
        for (auto& cmd : m_Commands) {
            if (cmd) {
                cmd->Compress();
            }
        }
    }

    void SetState(TransactionState state) noexcept { m_State = state; }
    [[nodiscard]] TransactionDescriptor& Desc() noexcept { return m_Desc; }

private:
    TransactionId m_Id{};
    TransactionDescriptor m_Desc{};
    TransactionState m_State = TransactionState::Open;
    std::uint64_t m_TimestampMs = 0;
    std::vector<TransactionCommandPtr> m_Commands;
};

class NeverMergePolicy final : public IMergePolicy {
public:
    [[nodiscard]] bool CanMergeTransactions(const ITransaction&, const ITransaction&) const override {
        return false;
    }
    [[nodiscard]] bool CanMergeCommands(const ITransactionCommand&, const ITransactionCommand&) const override {
        return false;
    }
};

class ConsecutiveSameTargetMergePolicy final : public IMergePolicy {
public:
    explicit ConsecutiveSameTargetMergePolicy(std::uint32_t windowMs) : m_WindowMs(windowMs) {}

    [[nodiscard]] bool CanMergeTransactions(const ITransaction& older, const ITransaction& newer) const override {
        if (!older.CanMerge() || !newer.CanMerge()) {
            return false;
        }
        if (older.Kind() != newer.Kind()) {
            return false;
        }
        if (older.WorldKey().value != newer.WorldKey().value) {
            return false;
        }
        if (m_WindowMs > 0 && newer.TimestampMs() >= older.TimestampMs()) {
            const auto dt = newer.TimestampMs() - older.TimestampMs();
            if (dt > m_WindowMs) {
                return false;
            }
        }
        if (older.CommandCount() != 1 || newer.CommandCount() != 1) {
            return false;
        }
        const auto& a = older.Commands()[0];
        const auto& b = newer.Commands()[0];
        if (!a || !b) {
            return false;
        }
        return CanMergeCommands(*a, *b);
    }

    [[nodiscard]] bool CanMergeCommands(
        const ITransactionCommand& older,
        const ITransactionCommand& newer) const override {
        if (older.Kind() != newer.Kind()) {
            return false;
        }
        if (older.MergeKey().empty() || newer.MergeKey().empty()) {
            return false;
        }
        return older.MergeKey() == newer.MergeKey();
    }

private:
    std::uint32_t m_WindowMs = 500;
};

[[nodiscard]] std::unique_ptr<ITransactionHistory> CreateTransactionHistory(HistoryConfig config);
[[nodiscard]] std::unique_ptr<ITransactionManager> CreateTransactionManager(
    reflection::ITypeRegistry& registry,
    serialization::ISerializer* serializer,
    HistoryConfig historyConfig,
    std::shared_ptr<IMergePolicy> mergePolicy);

} // namespace detail
} // namespace we::editor::undo
