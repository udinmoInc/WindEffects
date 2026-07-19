#include "PropertyEditor/PropertyEditorBenchmark.h"
#include "PropertyEditor/IPropertyEditorRuntime.h"
#include "PropertyEditor/MultiObjectEdit.h"

#include "Reflection/BuiltinTypes.h"
#include "Reflection/Registration.h"
#include "Reflection/Reflection.h"

#include <chrono>
#include <sstream>
#include <vector>

namespace we::editor::property {
namespace {

struct BenchItem {
    float a = 0.f;
    float b = 0.f;
    float c = 0.f;
    std::int32_t id = 0;
    bool flag = false;
};

reflection::TypeId RegisterBenchItem(reflection::ITypeRegistry& registry) {
    reflection::TypeBuilder builder("we::editor::property::bench::Item");
    builder.Kind(reflection::TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(BenchItem)))
        .Alignment(static_cast<std::uint32_t>(alignof(BenchItem)))
        .Ops(reflection::MakeTypeOpsFor<BenchItem>())
        .Property(reflection::MakeOffsetProperty(
            "a", reflection::BuiltinTypeId::Float(), offsetof(BenchItem, a), sizeof(float), alignof(float)))
        .Property(reflection::MakeOffsetProperty(
            "b", reflection::BuiltinTypeId::Float(), offsetof(BenchItem, b), sizeof(float), alignof(float)))
        .Property(reflection::MakeOffsetProperty(
            "c", reflection::BuiltinTypeId::Float(), offsetof(BenchItem, c), sizeof(float), alignof(float)))
        .Property(reflection::MakeOffsetProperty(
            "id",
            reflection::BuiltinTypeId::Int32(),
            offsetof(BenchItem, id),
            sizeof(std::int32_t),
            alignof(std::int32_t)))
        .Property(reflection::MakeOffsetProperty(
            "flag",
            reflection::BuiltinTypeId::Bool(),
            offsetof(BenchItem, flag),
            sizeof(bool),
            alignof(bool)));
    builder.Register(registry, reflection::RegisterMode::Replace);
    return reflection::MakeTypeId("we::editor::property::bench::Item");
}

template <typename Fn>
PropertyEditorBenchmarkSample TimeSample(std::string name, std::uint64_t iterations, Fn&& fn) {
    PropertyEditorBenchmarkSample sample;
    sample.name = std::move(name);
    sample.iterations = iterations;
    const auto start = std::chrono::steady_clock::now();
    for (std::uint64_t i = 0; i < iterations; ++i) {
        fn();
    }
    const auto end = std::chrono::steady_clock::now();
    sample.milliseconds =
        std::chrono::duration<double, std::milli>(end - start).count();
    sample.perIterationNs =
        iterations == 0 ? 0.0 : (sample.milliseconds * 1.0e6) / static_cast<double>(iterations);
    return sample;
}

} // namespace

PropertyEditorBenchmarkReport RunPropertyEditorBenchmarks(const PropertyEditorBenchmarkConfig& config) {
    PropertyEditorBenchmarkReport report{};

    auto registry = reflection::CreateTypeRegistry({});
    reflection::RegisterBuiltinTypes(*registry);
    const auto typeId = RegisterBenchItem(*registry);

    PropertyEditorDependencies deps;
    deps.typeRegistry = registry.get();
    auto runtime = CreatePropertyEditorRuntime(deps);
    if (!runtime) {
        report.success = false;
        report.summary = "Failed to create runtime";
        return report;
    }

    const std::uint32_t n = std::max(1u, config.propertyCount / 5u);
    std::vector<BenchItem> items(n);
    for (std::uint32_t i = 0; i < n; ++i) {
        items[i].id = static_cast<std::int32_t>(i);
        items[i].a = static_cast<float>(i);
    }

    auto details = runtime->MakeDetailsView();
    report.samples.push_back(TimeSample("BuildTree", 50, [&] {
        details->SetObject(typeId, &items.front());
    }));

    report.samples.push_back(TimeSample("FilterSearch", config.filterIterations, [&] {
        details->SetSearchText("a");
        details->SetSearchText("");
    }));

    const std::uint32_t multiCount = std::min<std::uint32_t>(
        config.multiObjectCount, static_cast<std::uint32_t>(items.size()));
    std::vector<void*> ptrs;
    ptrs.reserve(multiCount);
    for (std::uint32_t i = 0; i < multiCount; ++i) {
        ptrs.push_back(&items[i]);
    }

    report.samples.push_back(TimeSample("MultiObjectMerge", 50, [&] {
        details->SetObjects(typeId, ptrs);
        MultiObjectBinding binding;
        binding.typeId = typeId;
        binding.instances = ptrs;
        (void)MergeCommonProperties(*registry, binding, {});
    }));

    report.samples.push_back(TimeSample("EditNotify", 200, [&] {
        details->SetObject(typeId, &items.front());
        if (auto node = details->GetTree().FindByPath("id")) {
            if (auto handle = node->GetHandle()) {
                (void)handle->SetInt32(handle->GetPropertyInfo() ? items.front().id + 1 : 0);
            }
        }
    }));

    runtime->Shutdown();
    report.success = true;
    std::ostringstream oss;
    oss << "PropertyEditorBenchmarks: " << report.samples.size() << " samples";
    report.summary = oss.str();
    return report;
}

} // namespace we::editor::property
