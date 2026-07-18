#include "Reflection/ReflectionBenchmark.h"
#include "Reflection/BuiltinTypes.h"
#include "Reflection/IBinaryArchive.h"
#include "Reflection/IObjectFactory.h"
#include "Reflection/IReflectionDiagnostics.h"
#include "Reflection/NameId.h"
#include "Reflection/Registration.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace we::runtime::reflection {
namespace {

struct BenchObject {
    float a = 0.0f;
    float b = 0.0f;
    std::int32_t c = 0;
    std::uint32_t d = 0;
    double e = 0.0;
    bool f = false;
    float g = 0.0f;
    float h = 0.0f;
};

BenchmarkSample MakeSample(std::string name, double ms, std::uint64_t iterations, std::size_t bytes = 0) {
    BenchmarkSample sample;
    sample.name = std::move(name);
    sample.milliseconds = ms;
    sample.iterations = iterations;
    sample.perIterationNs = iterations > 0 ? (ms * 1.0e6) / static_cast<double>(iterations) : 0.0;
    sample.bytesObserved = bytes;
    return sample;
}

template <typename Fn>
double TimeMs(Fn&& fn) {
    const auto start = std::chrono::steady_clock::now();
    fn();
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

void RegisterSyntheticType(ITypeRegistry& registry, std::uint32_t index, TypeId baseId) {
    char qualified[64];
    std::snprintf(qualified, sizeof(qualified), "we::bench::Type_%u", index);

    TypeBuilder builder(qualified);
    builder.Kind(TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(BenchObject)))
        .Alignment(static_cast<std::uint32_t>(alignof(BenchObject)))
        .Ops(MakeTypeOpsFor<BenchObject>())
        .SchemaVersion(1)
        .TypeVersion(1)
        .MigrationVersion(1)
        .BinaryCompatibilityVersion(1);

    if (baseId != kInvalidTypeId) {
        builder.Base(baseId);
    }

    builder.Property(MakeOffsetProperty(
        "a", BuiltinTypeId::Float(), offsetof(BenchObject, a), sizeof(float), alignof(float),
        PropertyFlags::Serialize | PropertyFlags::Editable, PrimitiveKind::Float));
    builder.Property(MakeOffsetProperty(
        "b", BuiltinTypeId::Float(), offsetof(BenchObject, b), sizeof(float), alignof(float),
        PropertyFlags::Serialize | PropertyFlags::Editable, PrimitiveKind::Float));
    builder.Property(MakeOffsetProperty(
        "c", BuiltinTypeId::Int32(), offsetof(BenchObject, c), sizeof(std::int32_t), alignof(std::int32_t),
        PropertyFlags::Serialize | PropertyFlags::Editable, PrimitiveKind::Int32));
    builder.Property(MakeOffsetProperty(
        "d", BuiltinTypeId::UInt32(), offsetof(BenchObject, d), sizeof(std::uint32_t), alignof(std::uint32_t),
        PropertyFlags::Serialize | PropertyFlags::Editable, PrimitiveKind::UInt32));
    builder.Property(MakeOffsetProperty(
        "e", BuiltinTypeId::Double(), offsetof(BenchObject, e), sizeof(double), alignof(double),
        PropertyFlags::Serialize | PropertyFlags::Editable, PrimitiveKind::Double));
    builder.Property(MakeOffsetProperty(
        "f", BuiltinTypeId::Bool(), offsetof(BenchObject, f), sizeof(bool), alignof(bool),
        PropertyFlags::Serialize | PropertyFlags::Editable, PrimitiveKind::Bool));
    builder.Property(MakeOffsetProperty(
        "g", BuiltinTypeId::Float(), offsetof(BenchObject, g), sizeof(float), alignof(float),
        PropertyFlags::Serialize | PropertyFlags::Editable, PrimitiveKind::Float));
    builder.Property(MakeOffsetProperty(
        "h", BuiltinTypeId::Float(), offsetof(BenchObject, h), sizeof(float), alignof(float),
        PropertyFlags::Serialize | PropertyFlags::Editable, PrimitiveKind::Float));

    builder.Register(registry, RegisterMode::Replace);
}

} // namespace

BenchmarkReport RunReflectionBenchmarks(const BenchmarkConfig& configIn) {
    BenchmarkConfig config = configIn;
    if (config.runFullScale) {
        config.typeCount = 100000;
        config.objectsPerType = 10; // 1,000,000 objects
        config.pluginCount = 100;
        config.inheritanceDepth = 16;
        config.propertiesPerType = 8;
    }

    BenchmarkReport report;
    TypeRegistryDependencies deps;
    deps.registerBuiltins = true;
    deps.applyStaticInitializers = false;
    deps.sealAfterBuiltins = false;
    auto registry = CreateTypeRegistry(deps);

    std::vector<TypeId> typeIds;
    typeIds.reserve(config.typeCount);

    // --- Registration ---
    const double regMs = TimeMs([&] {
        TypeId prevBase = kInvalidTypeId;
        for (std::uint32_t i = 0; i < config.typeCount; ++i) {
            if (config.pluginCount > 0 && (i % (config.typeCount / config.pluginCount + 1)) == 0) {
                registry->EndPluginRegistration();
                (void)registry->BeginPluginRegistration("bench_plugin");
            }
            const TypeId base = (config.inheritanceDepth > 0 && (i % config.inheritanceDepth) != 0)
                ? prevBase
                : kInvalidTypeId;
            RegisterSyntheticType(*registry, i, base);
            char qualified[64];
            std::snprintf(qualified, sizeof(qualified), "we::bench::Type_%u", i);
            const TypeId id = MakeTypeId(qualified);
            typeIds.push_back(id);
            prevBase = id;
            if ((i % config.inheritanceDepth) == config.inheritanceDepth - 1) {
                prevBase = kInvalidTypeId;
            }
        }
        registry->EndPluginRegistration();
    });
    report.samples.push_back(MakeSample("registration", regMs, config.typeCount));

    const double sealMs = TimeMs([&] { registry->Seal(); });
    report.samples.push_back(MakeSample("seal", sealMs, 1));

    // --- Type lookup ---
    volatile std::uintptr_t sink = 0;
    const double lookupMs = TimeMs([&] {
        for (std::uint32_t round = 0; round < 10; ++round) {
            for (TypeId id : typeIds) {
                const TypeInfo* info = registry->Find(id);
                sink += info ? info->size : 0;
            }
        }
    });
    report.samples.push_back(MakeSample("type_lookup", lookupMs, static_cast<std::uint64_t>(typeIds.size()) * 10ull));

    // --- Property lookup ---
    const NameId nameA = InternName("a");
    const double propMs = TimeMs([&] {
        for (std::uint32_t round = 0; round < 5; ++round) {
            for (TypeId id : typeIds) {
                const PropertyInfo* prop = registry->FindProperty(id, nameA);
                sink += prop ? prop->offset : 0;
            }
        }
    });
    report.samples.push_back(
        MakeSample("property_lookup", propMs, static_cast<std::uint64_t>(typeIds.size()) * 5ull));

    // --- Function lookup (mostly miss / empty) ---
    const NameId nameFn = InternName("DoWork");
    const double fnMs = TimeMs([&] {
        for (TypeId id : typeIds) {
            const FunctionInfo* fn = registry->FindFunction(id, nameFn);
            sink += fn ? 1u : 0u;
        }
    });
    report.samples.push_back(MakeSample("function_lookup", fnMs, typeIds.size()));

    // --- Object construction (streaming destroy to support 1M target without huge residency) ---
    ObjectFactoryDependencies factoryDeps;
    factoryDeps.registry = registry.get();
    auto factory = CreateObjectFactory(factoryDeps);
    const TypeId firstType = typeIds.empty() ? kInvalidTypeId : typeIds.front();
    const std::uint64_t objectCount =
        static_cast<std::uint64_t>(config.typeCount) * static_cast<std::uint64_t>(config.objectsPerType);

    std::uint64_t constructed = 0;
    const double ctorMs = TimeMs([&] {
        for (std::uint64_t i = 0; i < objectCount; ++i) {
            const TypeId id = typeIds[static_cast<std::size_t>(i % typeIds.size())];
            void* obj = factory->Create(id);
            if (obj) {
                ++constructed;
                factory->Destroy(id, obj);
            }
        }
    });
    report.samples.push_back(MakeSample("object_construction", ctorMs, constructed));

    // --- Concurrent sealed lookups ---
    std::atomic<std::uint64_t> concurrentHits{0};
    const double concurrentMs = TimeMs([&] {
        constexpr int kThreads = 8;
        std::vector<std::thread> threads;
        threads.reserve(kThreads);
        for (int t = 0; t < kThreads; ++t) {
            threads.emplace_back([&, t] {
                std::uint64_t local = 0;
                for (std::uint32_t round = 0; round < 3; ++round) {
                    for (std::size_t i = 0; i < typeIds.size(); ++i) {
                        const TypeId id = typeIds[(i + static_cast<std::size_t>(t)) % typeIds.size()];
                        if (registry->Find(id) && registry->FindProperty(id, nameA)) {
                            ++local;
                        }
                    }
                }
                concurrentHits.fetch_add(local, std::memory_order_relaxed);
            });
        }
        for (auto& thread : threads) {
            thread.join();
        }
    });
    report.samples.push_back(MakeSample(
        "concurrent_lookup",
        concurrentMs,
        concurrentHits.load(std::memory_order_relaxed)));

    // --- Serialization traversal ---
    BenchObject local{};
    local.a = 1.0f;
    local.c = 42;
    const double serMs = TimeMs([&] {
        for (std::uint32_t i = 0; i < 10000; ++i) {
            auto bytes = SerializeObjectToBytes(*registry, firstType, &local);
            sink += bytes.size();
        }
    });
    report.samples.push_back(MakeSample("serialization_traversal", serMs, 10000));

    report.diagnostics = CaptureDiagnostics(*registry);
    report.success = constructed == objectCount && concurrentHits.load() > 0;

    std::ostringstream oss;
    oss << "Reflection benchmarks: types=" << config.typeCount
        << " objects=" << objectCount
        << " plugins=" << config.pluginCount
        << " constructed=" << constructed
        << " concurrent_hits=" << concurrentHits.load()
        << " sink=" << sink;
    report.summary = oss.str();
    return report;
}

} // namespace we::runtime::reflection
