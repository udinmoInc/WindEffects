#include "UndoInternal.h"

namespace we::editor::undo {

TransactionCommandPtr MakePropertyChangeCommand(
    reflection::ITypeRegistry& registry,
    reflection::TypeId typeId,
    std::vector<void*> instances,
    std::string path,
    std::vector<std::uint8_t> beforeBytes,
    std::vector<std::uint8_t> afterBytes,
    std::string label)
{
    return std::make_shared<detail::PropertyChangeCommand>(
        registry,
        typeId,
        std::move(instances),
        std::move(path),
        std::move(beforeBytes),
        std::move(afterBytes),
        std::move(label));
}

TransactionCommandPtr MakeSnapshotSwapCommand(
    serialization::ISerializer* serializer,
    reflection::ITypeRegistry& registry,
    reflection::TypeId typeId,
    void* instance,
    serialization::Snapshot before,
    serialization::Snapshot after,
    std::string label,
    TransactionKind kind)
{
    return std::make_shared<detail::SnapshotSwapCommand>(
        serializer,
        registry,
        typeId,
        instance,
        std::move(before),
        std::move(after),
        std::move(label),
        kind);
}

TransactionCommandPtr MakeCustomCommand(
    std::string label,
    TransactionKind kind,
    std::string mergeKey,
    std::function<bool()> undoFn,
    std::function<bool()> redoFn,
    std::uint64_t estimatedBytes)
{
    return std::make_shared<detail::CustomCommand>(
        std::move(label),
        kind,
        std::move(mergeKey),
        std::move(undoFn),
        std::move(redoFn),
        estimatedBytes);
}

TransactionCommandPtr MakeCompositeCommand(
    std::string label,
    TransactionKind kind,
    std::vector<TransactionCommandPtr> children)
{
    return std::make_shared<detail::CompositeCommand>(
        std::move(label), kind, std::move(children));
}

std::shared_ptr<IMergePolicy> CreateMergePolicy(MergePolicyKind kind, std::uint32_t windowMs) {
    switch (kind) {
    case MergePolicyKind::Never:
        return std::make_shared<detail::NeverMergePolicy>();
    case MergePolicyKind::ConsecutiveSameTarget:
    case MergePolicyKind::TimeWindowSameTarget:
        return std::make_shared<detail::ConsecutiveSameTargetMergePolicy>(windowMs);
    }
    return std::make_shared<detail::NeverMergePolicy>();
}

} // namespace we::editor::undo
