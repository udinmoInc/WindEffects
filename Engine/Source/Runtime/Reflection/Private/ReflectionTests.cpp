#include "Reflection/ReflectionProductionReport.h"
#include "Reflection/BuiltinTypes.h"
#include "Reflection/FunctionInvoke.h"
#include "Reflection/IBinaryArchive.h"
#include "Reflection/IObjectFactory.h"
#include "Reflection/IPropertyAccessor.h"
#include "Reflection/IReflectionVisitor.h"
#include "Reflection/PluginOwnership.h"
#include "Reflection/PropertyPath.h"
#include "Reflection/Registration.h"
#include "Reflection/ReflectionMigration.h"
#include "Reflection/ReflectionOps.h"
#include "Reflection/ReflectionValidation.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>

namespace we::runtime::reflection {
namespace {

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Transform {
    Vec3 position{};
    float scale = 1.0f;
};

struct Actor {
    Transform transform{};
    std::int32_t health = 100;
    bool active = true;
};

bool InvokeAddHealth(void* instance, const void* args, std::size_t argsSize, void* returnBuffer, std::size_t returnBufferSize) {
    if (!instance || !args || argsSize < sizeof(std::int32_t)) {
        return false;
    }
    auto* actor = static_cast<Actor*>(instance);
    actor->health += *static_cast<const std::int32_t*>(args);
    if (returnBuffer && returnBufferSize >= sizeof(std::int32_t)) {
        *static_cast<std::int32_t*>(returnBuffer) = actor->health;
    }
    return true;
}

void Record(
    ReflectionTestReport& report,
    std::string name,
    bool passed,
    std::string message = {})
{
    ReflectionTestCaseResult result;
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

TypeId RegisterVec3(ITypeRegistry& registry) {
    TypeBuilder builder("we::test::Vec3");
    builder.Kind(TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(Vec3)))
        .Alignment(static_cast<std::uint32_t>(alignof(Vec3)))
        .Ops(MakeTypeOpsFor<Vec3>())
        .Property(MakeOffsetProperty(
            "x", BuiltinTypeId::Float(), offsetof(Vec3, x), sizeof(float), alignof(float)))
        .Property(MakeOffsetProperty(
            "y", BuiltinTypeId::Float(), offsetof(Vec3, y), sizeof(float), alignof(float)))
        .Property(MakeOffsetProperty(
            "z", BuiltinTypeId::Float(), offsetof(Vec3, z), sizeof(float), alignof(float)));
    builder.Register(registry, RegisterMode::FailIfExists);
    return MakeTypeId("we::test::Vec3");
}

TypeId RegisterTransform(ITypeRegistry& registry, TypeId vec3Id) {
    TypeBuilder builder("we::test::Transform");
    builder.Kind(TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(Transform)))
        .Alignment(static_cast<std::uint32_t>(alignof(Transform)))
        .Ops(MakeTypeOpsFor<Transform>())
        .Property(MakeOffsetProperty(
            "position", vec3Id, offsetof(Transform, position), sizeof(Vec3), alignof(Vec3)))
        .Property(MakeOffsetProperty(
            "scale", BuiltinTypeId::Float(), offsetof(Transform, scale), sizeof(float), alignof(float)));
    builder.Register(registry, RegisterMode::FailIfExists);
    return MakeTypeId("we::test::Transform");
}

TypeId RegisterActor(ITypeRegistry& registry, TypeId transformId) {
    FunctionInfo addHealth;
    addHealth.name = "AddHealth";
    addHealth.returnTypeId = BuiltinTypeId::Int32();
    addHealth.invoke = &InvokeAddHealth;
    ParameterInfo amount;
    amount.name = "amount";
    amount.typeId = BuiltinTypeId::Int32();
    addHealth.parameters.push_back(amount);

    TypeBuilder builder("we::test::Actor");
    builder.Kind(TypeKind::Class)
        .Size(static_cast<std::uint32_t>(sizeof(Actor)))
        .Alignment(static_cast<std::uint32_t>(alignof(Actor)))
        .Ops(MakeTypeOpsFor<Actor>())
        .SchemaVersion(2)
        .Property(MakeOffsetProperty(
            "transform",
            transformId,
            offsetof(Actor, transform),
            sizeof(Transform),
            alignof(Transform)))
        .Property(MakeOffsetProperty(
            "health",
            BuiltinTypeId::Int32(),
            offsetof(Actor, health),
            sizeof(std::int32_t),
            alignof(std::int32_t)))
        .Property(MakeOffsetProperty(
            "active",
            BuiltinTypeId::Bool(),
            offsetof(Actor, active),
            sizeof(bool),
            alignof(bool)))
        .Function(addHealth);
    builder.Register(registry, RegisterMode::FailIfExists);
    return MakeTypeId("we::test::Actor");
}

class CountingVisitor final : public IReflectionVisitor {
public:
    std::uint32_t types = 0;
    std::uint32_t properties = 0;
    bool VisitType(const TypeInfo&) override {
        ++types;
        return true;
    }
    bool VisitProperty(const TypeInfo&, const PropertyInfo&) override {
        ++properties;
        return true;
    }
};

} // namespace

ReflectionTestReport RunReflectionTests() {
    ReflectionTestReport report;
    ClearTypeMigrations();
    ClearTypeIdRemaps();
    ClearPropertyPathCache();

    TypeRegistryDependencies deps;
    deps.registerBuiltins = true;
    deps.applyStaticInitializers = false;
    auto registry = CreateTypeRegistry(deps);

    // --- Type registration ---
    const TypeId vec3Id = RegisterVec3(*registry);
    const TypeId transformId = RegisterTransform(*registry, vec3Id);
    const TypeId actorId = RegisterActor(*registry, transformId);
    Record(report, "type_registration", registry->Contains(actorId) && registry->Contains(vec3Id));

    // Duplicate registration detection
    const bool dupRejected = !TypeBuilder("we::test::Vec3")
                                  .Kind(TypeKind::Struct)
                                  .Size(1)
                                  .Register(*registry, RegisterMode::FailIfExists);
    Record(report, "duplicate_registration_detection", dupRejected);

    // Plugin registration / unload
    const std::uint64_t pluginToken = registry->BeginPluginRegistration("test_plugin");
    TypeBuilder("we::test::PluginType")
        .Kind(TypeKind::Struct)
        .Size(4)
        .Alignment(4)
        .Ops(MakeTypeOpsFor<std::int32_t>())
        .Property(MakeOffsetProperty(
            "value", BuiltinTypeId::Int32(), 0, sizeof(std::int32_t), alignof(std::int32_t)))
        .Register(*registry);
    registry->EndPluginRegistration();
    const TypeId pluginType = MakeTypeId("we::test::PluginType");
    Record(
        report,
        "plugin_registration",
        registry->Contains(pluginType) && registry->GetPluginId(pluginToken) == "test_plugin");

    RetainType(pluginType);
    Record(report, "plugin_unload_blocked_while_retained", !IsPluginUnloadSafe(*registry, pluginToken));
    ReleaseType(pluginType);
    Record(report, "plugin_unload", UnloadPluginReflection(*registry, pluginToken, false));
    Record(report, "plugin_unload_removes_types", !registry->Contains(pluginType));

    // Queries
    registry->Seal();
    Record(report, "seal", registry->IsSealed());
    Record(report, "type_lookup", registry->Find(actorId) != nullptr);
    Record(report, "property_lookup", registry->FindProperty(actorId, "health") != nullptr);
    Record(report, "function_lookup", registry->FindFunction(actorId, "AddHealth") != nullptr);
    Record(report, "serialize_plan", registry->GetSerializePlan(actorId) != nullptr);

    // Metadata
    const TypeInfo* actorInfo = registry->Find(actorId);
    Record(
        report,
        "metadata_queries",
        actorInfo && actorInfo->versions.schemaVersion == 2 && actorInfo->properties.size() == 3);

    // Property path
    Actor actor{};
    actor.transform.position.x = 3.5f;
    actor.health = 50;
    float x = 0.0f;
    const bool pathGet = GetPropertyPathRaw(
        *registry, actorId, &actor, "transform.position.x", &x, sizeof(x));
    Record(report, "property_path_get", pathGet && x == 3.5f);
    float newX = 9.0f;
    const bool pathSet = SetPropertyPathRaw(
        *registry, actorId, &actor, "transform.position.x", &newX, sizeof(newX));
    Record(report, "property_path_set", pathSet && actor.transform.position.x == 9.0f);
    const auto pathAgain = ResolvePropertyPath(*registry, actorId, "transform.position.x");
    Record(report, "property_path_cache", pathAgain.valid && GetPropertyPathCacheStats().hits >= 1);

    // Generic accessor
    PropertyAccessorDependencies accessorDeps;
    accessorDeps.registry = registry.get();
    auto accessor = CreatePropertyAccessor(accessorDeps);
    std::int32_t health = 0;
    Record(
        report,
        "generic_property_access",
        accessor->GetInt32(actorId, &actor, "health", health) && health == 50);

    // Function invoke
    std::int32_t delta = 7;
    std::int32_t newHealth = 0;
    Record(
        report,
        "generic_function_invocation",
        InvokeFunctionByName(
            *registry, actorId, "AddHealth", &actor, &delta, sizeof(delta), &newHealth, sizeof(newHealth))
            && newHealth == 57 && actor.health == 57);

    // Visitors
    CountingVisitor visitor;
    const std::size_t visitCount = VisitTypeHierarchy(*registry, actorId, visitor, {});
    Record(report, "reflection_visitors", visitCount > 0 && visitor.types > 0 && visitor.properties > 0);

    // Serialization plans + binary roundtrip
    Actor copy = actor;
    auto bytes = SerializeObjectToBytes(*registry, actorId, &actor);
    Actor loaded{};
    const bool deser = DeserializeObjectFromBytes(
        *registry, actorId, &loaded, bytes.data(), bytes.size());
    Record(report, "serialization_plans", deser && AreObjectsEqual(*registry, actorId, &actor, &loaded));

    // Ops: clone / diff / patch / hash / compare
    void* cloned = CloneObject(*registry, actorId, &actor);
    Record(report, "reflection_cloning", cloned != nullptr && AreObjectsEqual(*registry, actorId, &actor, cloned));
    Actor other = actor;
    other.health = 1;
    const ReflectionDiff diff = DiffObjects(*registry, actorId, &actor, &other);
    Record(report, "reflection_diff", !diff.identical && !diff.entries.empty());
    const ReflectionPatch patch = BuildPatch(*registry, actorId, &other, &actor);
    Record(report, "reflection_patching", ApplyPatch(*registry, &other, patch) && other.health == actor.health);
    const std::uint64_t hashA = HashObject(*registry, actorId, &actor);
    const std::uint64_t hashB = HashObject(*registry, actorId, &copy);
    Record(report, "reflection_hashing", hashA != 0 && hashA == HashObject(*registry, actorId, cloned));
    Record(report, "reflection_comparison", CompareObjects(*registry, actorId, &actor, &copy) == 0);
    (void)hashB;
    DestroyClonedObject(*registry, actorId, cloned);

    // Validation
    std::vector<DiagnosticIssue> issues;
    ValidationOptions options;
    options.reportUnusedTypes = false;
    ValidateRegistryEx(*registry, issues, options);
    Record(
        report,
        "reflection_validation",
        CountDiagnostics(issues, DiagnosticSeverity::Error) == 0);

    // Invalid inheritance / offset detection on a scratch registry
    {
        auto bad = CreateTypeRegistry({false, false, false});
        TypeBuilder("we::test::BadSelf").Kind(TypeKind::Struct).Size(4).Base(MakeTypeId("we::test::BadSelf")).Register(*bad);
        std::vector<DiagnosticIssue> badIssues;
        ValidateRegistryEx(*bad, badIssues, {});
        bool foundCycleOrSelf = false;
        for (const auto& issue : badIssues) {
            if (issue.code == DiagnosticCode::SelfBase || issue.code == DiagnosticCode::InheritanceCycle) {
                foundCycleOrSelf = true;
            }
        }
        Record(report, "invalid_inheritance_detection", foundCycleOrSelf);

        auto badOffset = CreateTypeRegistry({false, false, false});
        TypeBuilder("we::test::BadOffset")
            .Kind(TypeKind::Struct)
            .Size(4)
            .Property(MakeOffsetProperty("x", BuiltinTypeId::Int32(), 8, 4, 4))
            .Register(*badOffset);
        std::vector<DiagnosticIssue> offsetIssues;
        ValidateRegistryEx(*badOffset, offsetIssues, {});
        bool foundOffset = false;
        for (const auto& issue : offsetIssues) {
            if (issue.code == DiagnosticCode::InvalidPropertyOffset) {
                foundOffset = true;
            }
        }
        Record(report, "invalid_property_offset_validation", foundOffset);
    }

    // Binary compatibility
    const std::uint64_t fp = registry->ComputeFingerprint();
    const auto bin = ValidateBinaryCompatibility(*registry, fp, kReflectionSchemaVersion);
    Record(report, "binary_compatibility", bin.compatible);

    // Version migration
    {
        std::atomic<int> migrated{0};
        RegisterTypeMigration({
            actorId,
            [](void* instance, std::uint32_t from, std::uint32_t to, void* user) -> bool {
                (void)from;
                (void)to;
                if (user) {
                    static_cast<std::atomic<int>*>(user)->fetch_add(1);
                }
                if (instance) {
                    static_cast<Actor*>(instance)->health += 1;
                }
                return true;
            },
            &migrated,
            1,
            1});
        Actor migrateTarget = actor;
        Record(
            report,
            "version_migration",
            MigrateInstance(actorId, &migrateTarget, 1, 2) && migrated.load() == 1
                && migrateTarget.health == actor.health + 1);
        UnregisterTypeMigrations(actorId);
    }

    // Cache verification
    const CacheVerificationReport caches = VerifyReflectionCaches(*registry);
    Record(report, "reflection_cache_verification", caches.valid);

    // Thread safety / concurrent lookup
    {
        std::atomic<std::uint64_t> hits{0};
        std::vector<std::thread> threads;
        for (int t = 0; t < 8; ++t) {
            threads.emplace_back([&] {
                for (int i = 0; i < 2000; ++i) {
                    if (registry->Find(actorId) && registry->FindProperty(actorId, "health")) {
                        hits.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            });
        }
        for (auto& thread : threads) {
            thread.join();
        }
        Record(report, "thread_safety_concurrent_lookup", hits.load() == 8ull * 2000ull);
    }

    // Large-scale stress (moderate in-unit size for CI)
    {
        registry->Unseal();
        constexpr std::uint32_t kStressTypes = 2000;
        for (std::uint32_t i = 0; i < kStressTypes; ++i) {
            char name[64];
            std::snprintf(name, sizeof(name), "we::test::Stress_%u", i);
            TypeBuilder(name)
                .Kind(TypeKind::Struct)
                .Size(static_cast<std::uint32_t>(sizeof(Vec3)))
                .Alignment(static_cast<std::uint32_t>(alignof(Vec3)))
                .Ops(MakeTypeOpsFor<Vec3>())
                .Property(MakeOffsetProperty(
                    "x", BuiltinTypeId::Float(), 0, sizeof(float), alignof(float)))
                .Register(*registry, RegisterMode::Replace);
        }
        registry->Seal();
        std::uint32_t found = 0;
        for (std::uint32_t i = 0; i < kStressTypes; ++i) {
            char name[64];
            std::snprintf(name, sizeof(name), "we::test::Stress_%u", i);
            if (registry->FindByName(name)) {
                ++found;
            }
        }
        Record(report, "large_scale_stress", found == kStressTypes);
    }

    // Memory correctness — clone/destroy cycle
    {
        ObjectFactoryDependencies factoryDeps;
        factoryDeps.registry = registry.get();
        auto factory = CreateObjectFactory(factoryDeps);
        std::uint32_t created = 0;
        for (int i = 0; i < 1000; ++i) {
            void* obj = factory->Create(actorId);
            if (!obj) {
                break;
            }
            factory->Destroy(actorId, obj);
            ++created;
        }
        Record(report, "memory_correctness", created == 1000);
    }

    report.success = report.failed == 0;
    report.summary = "Reflection tests: passed=" + std::to_string(report.passed)
        + " failed=" + std::to_string(report.failed);
    return report;
}

} // namespace we::runtime::reflection
