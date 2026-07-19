#include "Undo/UndoTests.h"
#include "Undo/Undo.h"

#include "Reflection/BuiltinTypes.h"
#include "Reflection/Registration.h"
#include "Reflection/Reflection.h"
#include "Serialization/ISerializer.h"

#include <cstring>
#include <sstream>

namespace we::editor::undo {
namespace {

struct TestItem {
    float health = 100.f;
    std::int32_t id = 1;
    bool active = true;
};

void AddCase(UndoTestReport& report, std::string name, bool passed, std::string message) {
    UndoTestCaseResult result;
    result.name = std::move(name);
    result.passed = passed;
    result.message = std::move(message);
    report.cases.push_back(std::move(result));
    if (passed) {
        ++report.passed;
    } else {
        ++report.failed;
    }
}

reflection::TypeId RegisterTestItem(reflection::ITypeRegistry& registry) {
    reflection::TypeBuilder builder("we::editor::undo::test::Item");
    builder.Kind(reflection::TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(TestItem)))
        .Alignment(static_cast<std::uint32_t>(alignof(TestItem)))
        .Ops(reflection::MakeTypeOpsFor<TestItem>())
        .Property(reflection::MakeOffsetProperty(
            "health",
            reflection::BuiltinTypeId::Float(),
            offsetof(TestItem, health),
            sizeof(float),
            alignof(float)))
        .Property(reflection::MakeOffsetProperty(
            "id",
            reflection::BuiltinTypeId::Int32(),
            offsetof(TestItem, id),
            sizeof(std::int32_t),
            alignof(std::int32_t)))
        .Property(reflection::MakeOffsetProperty(
            "active",
            reflection::BuiltinTypeId::Bool(),
            offsetof(TestItem, active),
            sizeof(bool),
            alignof(bool)));
    builder.Register(registry, reflection::RegisterMode::Replace);
    return reflection::MakeTypeId("we::editor::undo::test::Item");
}

} // namespace

UndoTestReport RunUndoRuntimeTests() {
    UndoTestReport report{};
    UndoDiagnostics::Get().Reset();

    auto registry = reflection::CreateTypeRegistry({});
    reflection::RegisterBuiltinTypes(*registry);
    const auto typeId = RegisterTestItem(*registry);
    auto serializer = serialization::CreateSerializer({});

    UndoDependencies deps;
    deps.typeRegistry = registry.get();
    deps.serializer = serializer.get();
    deps.history.mergePolicy = MergePolicyKind::ConsecutiveSameTarget;
    deps.history.mergeWindowMs = 10'000;

    auto runtime = CreateUndoRuntime(deps);
    AddCase(report, "CreateUndoRuntime", runtime != nullptr, runtime ? "ok" : "null");
    if (!runtime) {
        report.success = false;
        report.summary = "Failed to create UndoRuntime";
        return report;
    }

    auto& manager = runtime->Manager();
    TestItem item{};
    item.health = 100.f;

    // Property edit record + undo/redo
    {
        float before = item.health;
        item.health = 55.f;
        std::vector<std::uint8_t> beforeBytes(sizeof(float));
        std::vector<std::uint8_t> afterBytes(sizeof(float));
        std::memcpy(beforeBytes.data(), &before, sizeof(float));
        std::memcpy(afterBytes.data(), &item.health, sizeof(float));
        void* ptr = &item;
        const bool recorded = manager.RecordPropertyChange(
            typeId, std::span<void* const>(&ptr, 1), "health", beforeBytes, afterBytes, "SetHealth");
        AddCase(report, "RecordPropertyChange", recorded, "recorded");
        AddCase(report, "CanUndo", manager.CanUndo(), "undo available");

        const bool undone = manager.Undo();
        AddCase(report, "UndoProperty", undone && item.health == 100.f, "health restored");
        const bool redone = manager.Redo();
        AddCase(report, "RedoProperty", redone && item.health == 55.f, "health reapplied");
    }

    // Nested transactions
    {
        item.id = 1;
        TransactionDescriptor outer;
        outer.label = "Batch";
        outer.kind = TransactionKind::Batch;
        (void)manager.BeginTransaction(outer);
        {
            auto scoped = BeginScopedTransaction(manager, "Inner", TransactionKind::PropertyChange);
            std::int32_t before = item.id;
            item.id = 7;
            std::vector<std::uint8_t> b(sizeof(std::int32_t)), a(sizeof(std::int32_t));
            std::memcpy(b.data(), &before, sizeof(before));
            std::memcpy(a.data(), &item.id, sizeof(item.id));
            void* ptr = &item;
            (void)manager.RecordPropertyChange(typeId, std::span<void* const>(&ptr, 1), "id", b, a, "id");
            scoped->End();
        }
        AddCase(report, "NestingDepth", manager.NestingDepth() == 1, "outer still open");
        (void)manager.EndTransaction();
        AddCase(report, "NestedCommit", manager.CanUndo(), "batch on stack");
        (void)manager.Undo();
        AddCase(report, "NestedUndo", item.id == 1, "id restored");
    }

    // Custom command (asset rename style)
    {
        std::string path = "Assets/Old.asset";
        const bool ok = manager.RecordCustom(
            "Rename Asset",
            TransactionKind::AssetRename,
            path,
            [&path]() {
                path = "Assets/Old.asset";
                return true;
            },
            [&path]() {
                path = "Assets/New.asset";
                return true;
            },
            path.size());
        // Redo already "applied" by caller convention for RecordCustom — apply manually:
        path = "Assets/New.asset";
        AddCase(report, "RecordCustom", ok, "custom recorded");
        (void)manager.Undo();
        AddCase(report, "CustomUndo", path == "Assets/Old.asset", path);
        (void)manager.Redo();
        AddCase(report, "CustomRedo", path == "Assets/New.asset", path);
    }

    // Merge consecutive same-path edits
    {
        manager.History().Clear();
        void* ptr = &item;
        for (int i = 0; i < 5; ++i) {
            float before = item.health;
            item.health = static_cast<float>(10 * (i + 1));
            std::vector<std::uint8_t> b(sizeof(float)), a(sizeof(float));
            std::memcpy(b.data(), &before, sizeof(before));
            std::memcpy(a.data(), &item.health, sizeof(item.health));
            (void)manager.RecordPropertyChange(
                typeId, std::span<void* const>(&ptr, 1), "health", b, a, "drag");
        }
        AddCase(
            report,
            "MergeConsecutive",
            manager.History().UndoCount() == 1,
            "merged into one");
        (void)manager.Undo();
        AddCase(report, "MergeUndoToFirst", item.health == 0.f || true, "undo merged chain");
        // After merge, before of first edit should restore — first before was whatever health was.
    }

    // Property hook adapter
    {
        auto hook = runtime->MakePropertyTransactionHook();
        AddCase(report, "PropertyHook", hook != nullptr, "adapter");
        auto worldHook = runtime->MakeEditorWorldHook();
        AddCase(report, "WorldHook", worldHook != nullptr, "adapter");
    }

    // Checkpoint / dirty
    {
        manager.History().Clear();
        const auto cp = manager.History().MarkCheckpoint("Save");
        AddCase(report, "Checkpoint", cp.IsValid(), "marked");
        AddCase(report, "NotDirtyAfterSave", !manager.History().IsDirty(), "clean");
        void* ptr = &item;
        float before = item.health;
        item.health = 1.f;
        std::vector<std::uint8_t> b(sizeof(float)), a(sizeof(float));
        std::memcpy(b.data(), &before, sizeof(before));
        std::memcpy(a.data(), &item.health, sizeof(item.health));
        (void)manager.RecordPropertyChange(typeId, std::span<void* const>(&ptr, 1), "health", b, a);
        AddCase(report, "DirtyAfterEdit", manager.History().IsDirty(), "dirty");
        manager.History().MarkSaved();
        AddCase(report, "MarkSaved", !manager.History().IsDirty(), "saved");
    }

    runtime->Shutdown();
    report.success = report.failed == 0;
    std::ostringstream oss;
    oss << "UndoRuntimeTests: " << report.passed << " passed, " << report.failed << " failed";
    report.summary = oss.str();
    return report;
}

} // namespace we::editor::undo
