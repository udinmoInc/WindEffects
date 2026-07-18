#include "Serialization/SerializationTests.h"
#include "Serialization/ISerializer.h"

#include "Reflection/BuiltinTypes.h"
#include "Reflection/Registration.h"

#include <chrono>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

namespace we::runtime::serialization {
namespace {

struct BenchObj {
    std::int32_t a = 0;
    float b = 0.0f;
    double c = 0.0;
    bool d = false;
};

SerializationBenchmarkSample MakeSample(std::string name, double ms, std::uint64_t iters) {
    SerializationBenchmarkSample sample;
    sample.name = std::move(name);
    sample.milliseconds = ms;
    sample.iterations = iters;
    sample.perIterationNs = iters > 0 ? (ms * 1.0e6) / static_cast<double>(iters) : 0.0;
    return sample;
}

template <typename Fn>
double TimeMs(Fn&& fn) {
    const auto start = std::chrono::steady_clock::now();
    fn();
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

} // namespace

SerializationDiagnosticsSnapshot CaptureSerializationDiagnostics() {
    SerializationDiagnosticsSnapshot snap;
    // Timing filled by serializer statics indirectly via last run benchmarks.
    return snap;
}

SerializationBenchmarkReport RunSerializationBenchmarks(const SerializationBenchmarkConfig& configIn) {
    SerializationBenchmarkConfig config = configIn;
    if (config.runFullScale) {
        config.objectCount = 1000000;
        config.graphSize = 100000;
    }

    SerializationBenchmarkReport report;
    reflection::TypeRegistryDependencies deps;
    deps.registerBuiltins = true;
    deps.applyStaticInitializers = false;
    auto registry = reflection::CreateTypeRegistry(deps);

    reflection::TypeBuilder("we::ser::BenchObj")
        .Kind(reflection::TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(BenchObj)))
        .Alignment(static_cast<std::uint32_t>(alignof(BenchObj)))
        .Ops(reflection::MakeTypeOpsFor<BenchObj>())
        .Property(reflection::MakeOffsetProperty(
            "a",
            reflection::BuiltinTypeId::Int32(),
            offsetof(BenchObj, a),
            sizeof(std::int32_t),
            alignof(std::int32_t)))
        .Property(reflection::MakeOffsetProperty(
            "b",
            reflection::BuiltinTypeId::Float(),
            offsetof(BenchObj, b),
            sizeof(float),
            alignof(float)))
        .Property(reflection::MakeOffsetProperty(
            "c",
            reflection::BuiltinTypeId::Double(),
            offsetof(BenchObj, c),
            sizeof(double),
            alignof(double)))
        .Property(reflection::MakeOffsetProperty(
            "d",
            reflection::BuiltinTypeId::Bool(),
            offsetof(BenchObj, d),
            sizeof(bool),
            alignof(bool)))
        .Register(*registry);
    registry->Seal();

    const reflection::TypeId typeId = reflection::MakeTypeId("we::ser::BenchObj");
    SerializerDependencies serDeps;
    serDeps.registry = registry.get();
    auto serializer = CreateSerializer(serDeps);

    BenchObj obj{};
    obj.a = 1;
    obj.b = 2.0f;
    obj.c = 3.0;
    obj.d = true;

    std::uint64_t ok = 0;
    const double serMs = TimeMs([&] {
        for (std::uint32_t i = 0; i < config.objectCount; ++i) {
            obj.a = static_cast<std::int32_t>(i);
            auto bytes = serializer->SerializeObject(typeId, &obj);
            BenchObj out{};
            if (!bytes.empty()
                && serializer->DeserializeObject(typeId, &out, bytes.data(), bytes.size())
                && out.a == obj.a) {
                ++ok;
            }
        }
    });
    report.samples.push_back(MakeSample("object_roundtrip", serMs, ok));

    // Graph stress
    ObjectGraph graph;
    std::vector<BenchObj> storage(config.graphSize);
    for (std::uint32_t i = 0; i < config.graphSize; ++i) {
        storage[i].a = static_cast<std::int32_t>(i);
        const ObjectId id = graph.AddObject(typeId, &storage[i], false);
        if (i > 0) {
            ObjectReference ref;
            ref.objectId = id - 1;
            ref.typeId = typeId;
            ref.kind = ReferenceKind::Weak;
            graph.AddReference(id, ref);
        }
    }

    std::vector<std::uint8_t> graphBytes;
    const double graphMs = TimeMs([&] { graphBytes = serializer->SerializeGraph(graph); });
    report.samples.push_back(MakeSample("graph_serialize", graphMs, config.graphSize));

    ObjectGraph loaded;
    const double loadMs = TimeMs([&] {
        (void)serializer->DeserializeGraph(loaded, graphBytes.data(), graphBytes.size());
    });
    report.samples.push_back(MakeSample("graph_deserialize", loadMs, loaded.Size()));
    loaded.Clear(registry.get());

    report.diagnostics.memory.documentBytes = graphBytes.size();
    report.diagnostics.memory.objectCount = config.graphSize;
    report.diagnostics.memory.payloadBytes = graphBytes.size();
    report.diagnostics.lastChecksum =
        graphBytes.empty() ? 0 : ComputeChecksum(graphBytes.data(), graphBytes.size());
    report.diagnostics.checksumValid =
        graphBytes.empty() ? false : IsDocumentValid(graphBytes.data(), graphBytes.size(), true);

    report.success = ok == config.objectCount && !graphBytes.empty();
    std::ostringstream oss;
    oss << "Serialization benchmarks: objects=" << config.objectCount
        << " graph=" << config.graphSize << " ok=" << ok
        << " graph_bytes=" << graphBytes.size();
    report.summary = oss.str();
    return report;
}

} // namespace we::runtime::serialization
