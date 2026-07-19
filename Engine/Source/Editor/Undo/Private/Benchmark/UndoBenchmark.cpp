#include "Undo/UndoBenchmark.h"
#include "Undo/Undo.h"

#include "Reflection/BuiltinTypes.h"
#include "Reflection/Registration.h"
#include "Reflection/Reflection.h"

#include <chrono>
#include <cstring>
#include <sstream>
#include <vector>

namespace we::editor::undo {
namespace {

struct BenchItem {
    float a = 0.f;
    float b = 0.f;
    std::int32_t id = 0;
};

reflection::TypeId RegisterBenchItem(reflection::ITypeRegistry& registry) {
    reflection::TypeBuilder builder("we::editor::undo::bench::Item");
    builder.Kind(reflection::TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(BenchItem)))
        .Alignment(static_cast<std::uint32_t>(alignof(BenchItem)))
        .Ops(reflection::MakeTypeOpsFor<BenchItem>())
        .Property(reflection::MakeOffsetProperty(
            "a", reflection::BuiltinTypeId::Float(), offsetof(BenchItem, a), sizeof(float), alignof(float)))
        .Property(reflection::MakeOffsetProperty(
            "b", reflection::BuiltinTypeId::Float(), offsetof(BenchItem, b), sizeof(float), alignof(float)))
        .Property(reflection::MakeOffsetProperty(
            "id",
            reflection::BuiltinTypeId::Int32(),
            offsetof(BenchItem, id),
            sizeof(std::int32_t),
            alignof(std::int32_t)));
    builder.Register(registry, reflection::RegisterMode::Replace);
    return reflection::MakeTypeId("we::editor::undo::bench::Item");
}

template <typename Fn>
UndoBenchmarkSample TimeSample(const char* name, std::uint32_t iterations, Fn&& fn) {
    const auto start = std::chrono::steady_clock::now();
    for (std::uint32_t i = 0; i < iterations; ++i) {
        fn();
    }
    const auto end = std::chrono::steady_clock::now();
    UndoBenchmarkSample sample;
    sample.name = name;
    sample.iterations = iterations;
    sample.milliseconds =
        std::chrono::duration<double, std::milli>(end - start).count();
    return sample;
}

} // namespace

UndoBenchmarkReport RunUndoRuntimeBenchmarks(const UndoBenchmarkConfig& config) {
    UndoBenchmarkReport report{};

    auto registry = reflection::CreateTypeRegistry({});
    reflection::RegisterBuiltinTypes(*registry);
    const auto typeId = RegisterBenchItem(*registry);

    UndoDependencies deps;
    deps.typeRegistry = registry.get();
    deps.history.maxTransactions = config.historyDepth + 100;
    deps.history.mergePolicy = MergePolicyKind::Never;

    auto runtime = CreateUndoRuntime(deps);
    if (!runtime) {
        report.success = false;
        report.summary = "Failed to create runtime";
        return report;
    }

    auto& manager = runtime->Manager();
    BenchItem item{};

    report.samples.push_back(TimeSample("PropertyEdits", config.propertyEdits, [&] {
        float before = item.a;
        item.a += 1.f;
        std::vector<std::uint8_t> b(sizeof(float)), a(sizeof(float));
        std::memcpy(b.data(), &before, sizeof(before));
        std::memcpy(a.data(), &item.a, sizeof(item.a));
        void* ptr = &item;
        (void)manager.RecordPropertyChange(
            typeId, std::span<void* const>(&ptr, 1), "a", std::move(b), std::move(a));
    }));

    report.samples.push_back(TimeSample("UndoRedoPairs", std::min(config.propertyEdits, 1000u), [&] {
        if (manager.CanUndo()) {
            (void)manager.Undo();
            (void)manager.Redo();
        }
    }));

    report.samples.push_back(TimeSample("NestedTransactions", config.nestedDepth, [&] {
        for (std::uint32_t d = 0; d < config.nestedDepth; ++d) {
            TransactionDescriptor desc;
            desc.label = "N";
            (void)manager.BeginTransaction(desc);
        }
        for (std::uint32_t d = 0; d < config.nestedDepth; ++d) {
            (void)manager.EndTransaction();
        }
    }));

    // Multi-object
    std::vector<BenchItem> items(config.multiObjectCount);
    std::vector<void*> ptrs;
    ptrs.reserve(items.size());
    for (auto& it : items) {
        ptrs.push_back(&it);
    }
    report.samples.push_back(TimeSample("MultiObjectEdit", 200, [&] {
        float before = 0.f;
        float after = 1.f;
        std::vector<std::uint8_t> b(sizeof(float)), a(sizeof(float));
        std::memcpy(b.data(), &before, sizeof(before));
        std::memcpy(a.data(), &after, sizeof(after));
        for (auto* p : ptrs) {
            static_cast<BenchItem*>(p)->a = after;
        }
        (void)manager.RecordPropertyChange(typeId, ptrs, "a", b, a, "multi");
    }));

    runtime->Shutdown();
    report.success = true;
    std::ostringstream oss;
    oss << "UndoBenchmarks: " << report.samples.size() << " samples";
    report.summary = oss.str();
    return report;
}

} // namespace we::editor::undo
