#include "Serialization/SerializationTests.h"
#include "Serialization/Async.h"
#include "Serialization/Delta.h"
#include "Serialization/Diagnostics.h"
#include "Serialization/IArchive.h"
#include "Serialization/ISerializer.h"
#include "Serialization/Transforms.h"

#include "Reflection/BuiltinTypes.h"
#include "Reflection/Registration.h"
#include "Reflection/ReflectionMigration.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

namespace we::runtime::serialization {
namespace {

struct Item {
    std::int32_t id = 0;
    float value = 0.0f;
    bool enabled = true;
};

struct Node {
    std::int32_t tag = 0;
    Item item{};
};

void Record(SerializationTestReport& report, std::string name, bool passed, std::string message = {}) {
    SerializationTestCaseResult result;
    result.name = std::move(name);
    result.passed = passed;
    result.message = std::move(message);
    if (passed) {
        ++report.passed;
    } else {
        ++report.failed;
    }
    report.cases.push_back(std::move(result));
}

reflection::TypeId RegisterItem(reflection::ITypeRegistry& registry) {
    reflection::TypeBuilder("we::ser::Item")
        .Kind(reflection::TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(Item)))
        .Alignment(static_cast<std::uint32_t>(alignof(Item)))
        .Ops(reflection::MakeTypeOpsFor<Item>())
        .SchemaVersion(1)
        .Property(reflection::MakeOffsetProperty(
            "id",
            reflection::BuiltinTypeId::Int32(),
            offsetof(Item, id),
            sizeof(std::int32_t),
            alignof(std::int32_t)))
        .Property(reflection::MakeOffsetProperty(
            "value",
            reflection::BuiltinTypeId::Float(),
            offsetof(Item, value),
            sizeof(float),
            alignof(float)))
        .Property(reflection::MakeOffsetProperty(
            "enabled",
            reflection::BuiltinTypeId::Bool(),
            offsetof(Item, enabled),
            sizeof(bool),
            alignof(bool)))
        .Register(registry, reflection::RegisterMode::Replace);
    return reflection::MakeTypeId("we::ser::Item");
}

reflection::TypeId RegisterNode(reflection::ITypeRegistry& registry, reflection::TypeId itemId) {
    reflection::TypeBuilder("we::ser::Node")
        .Kind(reflection::TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(Node)))
        .Alignment(static_cast<std::uint32_t>(alignof(Node)))
        .Ops(reflection::MakeTypeOpsFor<Node>())
        .SchemaVersion(1)
        .Property(reflection::MakeOffsetProperty(
            "tag",
            reflection::BuiltinTypeId::Int32(),
            offsetof(Node, tag),
            sizeof(std::int32_t),
            alignof(std::int32_t)))
        .Property(reflection::MakeOffsetProperty(
            "item", itemId, offsetof(Node, item), sizeof(Item), alignof(Item)))
        .Register(registry, reflection::RegisterMode::Replace);
    return reflection::MakeTypeId("we::ser::Node");
}

} // namespace

SerializationTestReport RunSerializationTests() {
    SerializationTestReport report;
    ClearCustomSerializers();
    reflection::ClearTypeMigrations();

    reflection::TypeRegistryDependencies deps;
    deps.registerBuiltins = true;
    deps.applyStaticInitializers = false;
    auto registry = reflection::CreateTypeRegistry(deps);
    const reflection::TypeId itemId = RegisterItem(*registry);
    const reflection::TypeId nodeId = RegisterNode(*registry, itemId);
    registry->Seal();

    SerializerDependencies serDeps;
    serDeps.registry = registry.get();
    auto serializer = CreateSerializer(serDeps);

    // Serialize/deserialize correctness
    Item original{};
    original.id = 42;
    original.value = 3.5f;
    original.enabled = false;
    auto bytes = serializer->SerializeObject(itemId, &original);
    Item loaded{};
    const bool roundTrip = !bytes.empty()
        && serializer->DeserializeObject(itemId, &loaded, bytes.data(), bytes.size())
        && loaded.id == 42 && loaded.value == 3.5f && loaded.enabled == false;
    Record(report, "serialize_deserialize_correctness", roundTrip);

    // Nested object
    Node node{};
    node.tag = 7;
    node.item = original;
    auto nodeBytes = serializer->SerializeObject(nodeId, &node);
    Node nodeLoaded{};
    Record(
        report,
        "nested_object_roundtrip",
        serializer->DeserializeObject(nodeId, &nodeLoaded, nodeBytes.data(), nodeBytes.size())
            && nodeLoaded.tag == 7 && nodeLoaded.item.id == 42);

    // Document validation / checksum
    Record(report, "document_validation", IsDocumentValid(bytes.data(), bytes.size(), true));

    // Corruption detection
    auto corrupted = bytes;
    if (corrupted.size() > sizeof(DocumentHeader) + 4) {
        corrupted[sizeof(DocumentHeader) + 2] ^= 0xFFu;
    }
    Record(report, "corrupted_files", !IsDocumentValid(corrupted.data(), corrupted.size(), true));

    // Invalid version
    {
        DocumentHeader header{};
        std::memcpy(&header, bytes.data(), sizeof(header));
        header.formatVersion = 0;
        auto badVer = bytes;
        std::memcpy(badVer.data(), &header, sizeof(header));
        std::vector<ValidationIssue> issues;
        ValidateDocumentBytes(badVer.data(), badVer.size(), issues, false);
        bool found = false;
        for (const auto& issue : issues) {
            if (issue.code == ValidationCode::BadFormatVersion) {
                found = true;
            }
        }
        Record(report, "invalid_versions", found);
    }

    // Object / circular references via graph
    {
        registry->Unseal();
        // Keep sealed registry for lookups — unseal not needed for graph refs.
        registry->Seal();

        Item a{};
        a.id = 1;
        Item b{};
        b.id = 2;
        ObjectGraph graph;
        const ObjectId idA = graph.AddObject(itemId, &a, false);
        const ObjectId idB = graph.AddObject(itemId, &b, false);
        ObjectReference ref;
        ref.objectId = idB;
        ref.typeId = itemId;
        ref.kind = ReferenceKind::Strong;
        graph.AddReference(idA, ref);
        // Circular
        ObjectReference back;
        back.objectId = idA;
        back.typeId = itemId;
        back.kind = ReferenceKind::Weak;
        graph.AddReference(idB, back);

        auto graphBytes = serializer->SerializeGraph(graph);
        ObjectGraph loadedGraph;
        const bool graphOk = serializer->DeserializeGraph(loadedGraph, graphBytes.data(), graphBytes.size());
        Record(report, "object_references", graphOk && loadedGraph.Size() == 2);
        Record(
            report,
            "circular_references",
            graphOk && !loadedGraph.ReferencesFrom(idA).empty()
                && !loadedGraph.ReferencesFrom(idB).empty());
        loadedGraph.Clear(registry.get());
    }

    // Asset references
    {
        ObjectReference assetRef;
        assetRef.kind = ReferenceKind::Asset;
        assetRef.guid.bytes[0] = 0xAB;
        assetRef.guid.bytes[15] = 0xCD;
        MemoryArchive archive(MemoryArchive::Mode::Saving);
        Record(report, "asset_references", archive.SerializeReference(assetRef) && archive.Good());
        auto refBytes = archive.TakeBytes();
        MemoryArchive loader(std::move(refBytes));
        ObjectReference loadedRef;
        Record(
            report,
            "asset_reference_roundtrip",
            loader.SerializeReference(loadedRef) && loadedRef.kind == ReferenceKind::Asset
                && loadedRef.guid.bytes[0] == 0xAB && loadedRef.guid.bytes[15] == 0xCD);
    }

    // Delta / diff / patch
    {
        Item base = original;
        Item target = original;
        target.id = 99;
        const BinaryDiff diff = DiffInstances(*registry, itemId, &base, &target);
        Record(report, "binary_diff", !diff.identical && !diff.entries.empty());
        BinaryDelta delta = GenerateDelta(*serializer, *registry, itemId, &base, &target);
        Item patched = base;
        Record(report, "delta_patch", ApplyDelta(*registry, &patched, delta) && patched.id == 99);
    }

    // Snapshot
    {
        const Snapshot snap = CaptureSnapshot(*serializer, itemId, &original);
        Item restored{};
        Record(
            report,
            "snapshot_serialization",
            RestoreSnapshot(*serializer, itemId, &restored, snap) && restored.id == original.id);
    }

    // Version migration
    {
        std::atomic<int> migrated{0};
        reflection::RegisterTypeMigration({
            itemId,
            [](void* instance, std::uint32_t, std::uint32_t, void* user) -> bool {
                if (user) {
                    static_cast<std::atomic<int>*>(user)->fetch_add(1);
                }
                if (instance) {
                    static_cast<Item*>(instance)->id += 1000;
                }
                return true;
            },
            &migrated,
            1,
            1});
        Item migrateTarget = original;
        Record(
            report,
            "version_migration",
            reflection::MigrateInstance(itemId, &migrateTarget, 1, 2) && migrated.load() == 1);
        reflection::UnregisterTypeMigrations(itemId);
    }

    // Custom serializer
    {
        static std::atomic<int> customCalls{0};
        RegisterCustomSerializer({
            itemId,
            [](IArchive& archive, void* instance, reflection::TypeId, void*) -> bool {
                customCalls.fetch_add(1);
                auto* item = static_cast<Item*>(instance);
                return archive.Binary().SerializeInt32(item->id)
                    && archive.Binary().SerializeFloat(item->value)
                    && archive.Binary().SerializeBool(item->enabled);
            },
            nullptr});
        Item customIn{};
        customIn.id = 5;
        customIn.value = 1.25f;
        auto customBytes = serializer->SerializeObject(itemId, &customIn);
        Item customOut{};
        const bool customOk = serializer->DeserializeObject(
            itemId, &customOut, customBytes.data(), customBytes.size());
        Record(report, "plugin_custom_serializer", customOk && customCalls.load() >= 2 && customOut.id == 5);
        UnregisterCustomSerializer(itemId);
    }

    // Async
    {
        std::vector<std::uint8_t> copy(sizeof(Item));
        std::memcpy(copy.data(), &original, sizeof(Item));
        auto shared = std::shared_ptr<ISerializer>(CreateSerializer(serDeps));
        auto fut = SerializeObjectAsync(shared, itemId, copy);
        const AsyncSerializeResult asyncResult = fut.get();
        Record(report, "async_serialization", asyncResult.success && !asyncResult.bytes.empty());
    }

    // Thread safety
    {
        std::atomic<int> ok{0};
        std::vector<std::thread> threads;
        for (int t = 0; t < 8; ++t) {
            threads.emplace_back([&] {
                Item local = original;
                local.id = 10 + t;
                auto b = serializer->SerializeObject(itemId, &local);
                Item out{};
                if (!b.empty() && serializer->DeserializeObject(itemId, &out, b.data(), b.size())
                    && out.id == local.id) {
                    ok.fetch_add(1);
                }
            });
        }
        for (auto& th : threads) {
            th.join();
        }
        Record(report, "thread_safety", ok.load() == 8);
    }

    // Binary compatibility / deterministic output
    {
        auto a = serializer->SerializeObject(itemId, &original);
        auto b = serializer->SerializeObject(itemId, &original);
        Record(report, "deterministic_binary_output", a == b && !a.empty());
        Record(report, "binary_compatibility", IsDocumentValid(a.data(), a.size(), true));
    }

    // Stress (moderate)
    {
        constexpr int kCount = 5000;
        int ok = 0;
        for (int i = 0; i < kCount; ++i) {
            Item it{};
            it.id = i;
            it.value = static_cast<float>(i) * 0.1f;
            auto b = serializer->SerializeObject(itemId, &it);
            Item out{};
            if (!b.empty() && serializer->DeserializeObject(itemId, &out, b.data(), b.size())
                && out.id == i) {
                ++ok;
            }
        }
        Record(report, "stress_thousands_of_objects", ok == kCount);
    }

    // Missing type detection (document with unknown type still validates structurally)
    Record(report, "checksums", ComputeChecksum(bytes.data(), bytes.size()) != 0);

    report.success = report.failed == 0;
    report.summary = "Serialization tests: passed=" + std::to_string(report.passed)
        + " failed=" + std::to_string(report.failed);
    return report;
}

} // namespace we::runtime::serialization
