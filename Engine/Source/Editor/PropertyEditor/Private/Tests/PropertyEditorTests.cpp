#include "PropertyEditor/PropertyEditorTests.h"
#include "PropertyEditor/Compat/LegacyPropertyEditor.h"
#include "PropertyEditor/IPropertyEditorRuntime.h"
#include "PropertyEditor/IPropertyTransactionHook.h"
#include "PropertyEditor/MultiObjectEdit.h"

#include "Reflection/BuiltinTypes.h"
#include "Reflection/Registration.h"
#include "Reflection/Reflection.h"

#include <cstring>
#include <sstream>

namespace we::editor::property {
namespace {

void AddCase(PropertyEditorTestReport& report, std::string name, bool passed, std::string message) {
    PropertyEditorTestCaseResult result;
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

struct Vec3 {
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
};

struct Transform {
    Vec3 position{};
    float scale = 1.f;
};

struct TestActor {
    Transform transform{};
    std::int32_t health = 100;
    bool active = true;
};

enum class TestMode : std::int32_t {
    Idle = 0,
    Walk = 1,
    Run = 2,
};

struct TestEnumHolder {
    TestMode mode = TestMode::Idle;
};

class CountingTransactionHook final : public IPropertyTransactionHook {
public:
    int preCount = 0;
    int commitCount = 0;
    int postCount = 0;

    void OnPreEdit(PropertyChangeEvent&) override { ++preCount; }
    void OnCommit(
        const PropertyChangeEvent&,
        const serialization::BinaryDiff*,
        const serialization::Snapshot*) override
    {
        ++commitCount;
    }
    void OnPostEdit(const PropertyChangeEvent&) override { ++postCount; }
};

reflection::TypeId RegisterVec3(reflection::ITypeRegistry& registry) {
    reflection::TypeBuilder builder("we::editor::property::test::Vec3");
    builder.Kind(reflection::TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(Vec3)))
        .Alignment(static_cast<std::uint32_t>(alignof(Vec3)))
        .Ops(reflection::MakeTypeOpsFor<Vec3>())
        .Property(reflection::MakeOffsetProperty(
            "x",
            reflection::BuiltinTypeId::Float(),
            offsetof(Vec3, x),
            sizeof(float),
            alignof(float)))
        .Property(reflection::MakeOffsetProperty(
            "y",
            reflection::BuiltinTypeId::Float(),
            offsetof(Vec3, y),
            sizeof(float),
            alignof(float)))
        .Property(reflection::MakeOffsetProperty(
            "z",
            reflection::BuiltinTypeId::Float(),
            offsetof(Vec3, z),
            sizeof(float),
            alignof(float)));
    builder.Register(registry, reflection::RegisterMode::Replace);
    return reflection::MakeTypeId("we::editor::property::test::Vec3");
}

reflection::TypeId RegisterTransform(reflection::ITypeRegistry& registry, reflection::TypeId vec3Id) {
    reflection::TypeBuilder builder("we::editor::property::test::Transform");
    builder.Kind(reflection::TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(Transform)))
        .Alignment(static_cast<std::uint32_t>(alignof(Transform)))
        .Ops(reflection::MakeTypeOpsFor<Transform>())
        .Property(reflection::MakeOffsetProperty(
            "position", vec3Id, offsetof(Transform, position), sizeof(Vec3), alignof(Vec3)))
        .Property(reflection::MakeOffsetProperty(
            "scale",
            reflection::BuiltinTypeId::Float(),
            offsetof(Transform, scale),
            sizeof(float),
            alignof(float)));
    builder.Register(registry, reflection::RegisterMode::Replace);
    return reflection::MakeTypeId("we::editor::property::test::Transform");
}

reflection::TypeId RegisterActor(reflection::ITypeRegistry& registry, reflection::TypeId transformId) {
    reflection::TypeBuilder builder("we::editor::property::test::Actor");
    builder.Kind(reflection::TypeKind::Class)
        .Size(static_cast<std::uint32_t>(sizeof(TestActor)))
        .Alignment(static_cast<std::uint32_t>(alignof(TestActor)))
        .Ops(reflection::MakeTypeOpsFor<TestActor>())
        .Property(reflection::MakeOffsetProperty(
            "transform",
            transformId,
            offsetof(TestActor, transform),
            sizeof(Transform),
            alignof(Transform)))
        .Property(reflection::MakeOffsetProperty(
            "health",
            reflection::BuiltinTypeId::Int32(),
            offsetof(TestActor, health),
            sizeof(std::int32_t),
            alignof(std::int32_t)))
        .Property(reflection::MakeOffsetProperty(
            "active",
            reflection::BuiltinTypeId::Bool(),
            offsetof(TestActor, active),
            sizeof(bool),
            alignof(bool)));
    builder.Register(registry, reflection::RegisterMode::Replace);
    return reflection::MakeTypeId("we::editor::property::test::Actor");
}

reflection::TypeId RegisterEnumHolder(reflection::ITypeRegistry& registry) {
    reflection::TypeBuilder enumBuilder("we::editor::property::test::TestMode");
    enumBuilder.Kind(reflection::TypeKind::Enum)
        .Size(static_cast<std::uint32_t>(sizeof(TestMode)))
        .Alignment(static_cast<std::uint32_t>(alignof(TestMode)))
        .EnumValue("Idle", 0)
        .EnumValue("Walk", 1)
        .EnumValue("Run", 2);
    enumBuilder.Register(registry, reflection::RegisterMode::Replace);
    const reflection::TypeId enumId = reflection::MakeTypeId("we::editor::property::test::TestMode");

    reflection::TypeBuilder holder("we::editor::property::test::EnumHolder");
    holder.Kind(reflection::TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(TestEnumHolder)))
        .Alignment(static_cast<std::uint32_t>(alignof(TestEnumHolder)))
        .Ops(reflection::MakeTypeOpsFor<TestEnumHolder>())
        .Property(reflection::MakeOffsetProperty(
            "mode", enumId, offsetof(TestEnumHolder, mode), sizeof(TestMode), alignof(TestMode)));
    holder.Register(registry, reflection::RegisterMode::Replace);
    return reflection::MakeTypeId("we::editor::property::test::EnumHolder");
}

} // namespace

PropertyEditorTestReport RunPropertyEditorTests() {
    PropertyEditorTestReport report{};

    auto registry = reflection::CreateTypeRegistry({});
    reflection::RegisterBuiltinTypes(*registry);
    const auto vec3Id = RegisterVec3(*registry);
    const auto transformId = RegisterTransform(*registry, vec3Id);
    const auto actorId = RegisterActor(*registry, transformId);
    const auto enumHolderId = RegisterEnumHolder(*registry);

    auto hook = std::make_shared<CountingTransactionHook>();
    PropertyEditorDependencies deps;
    deps.typeRegistry = registry.get();
    deps.transactionHook = hook;
    auto runtime = CreatePropertyEditorRuntime(deps);
    AddCase(report, "CreatePropertyEditorRuntime", runtime != nullptr, runtime ? "ok" : "null");
    if (!runtime) {
        report.success = false;
        report.summary = "Failed to create PropertyEditorRuntime";
        return report;
    }

    TestActor actor{};
    actor.health = 42;
    actor.transform.position.x = 1.f;

    auto details = runtime->MakeDetailsView();
    AddCase(report, "MakeDetailsView", details != nullptr, "view");
    details->SetObject(actorId, &actor);

    auto& tree = details->GetTree();
    AddCase(report, "TreeHasRoots", !tree.GetRootNodes().empty(), "categories");

    auto healthNode = tree.FindByPath("health");
    AddCase(report, "FindHealth", healthNode != nullptr, healthNode ? "found" : "missing");
    if (healthNode && healthNode->GetHandle()) {
        std::int32_t value = 0;
        const bool got = healthNode->GetHandle()->GetInt32(value);
        AddCase(report, "GetHealth", got && value == 42, "roundtrip get");
        const bool set = healthNode->GetHandle()->SetInt32(77);
        AddCase(report, "SetHealth", set && actor.health == 77, "roundtrip set");
        AddCase(
            report,
            "TransactionHook",
            hook->preCount > 0 && hook->commitCount > 0 && hook->postCount > 0,
            "pre/commit/post");
    } else {
        AddCase(report, "GetHealth", false, "no handle");
        AddCase(report, "SetHealth", false, "no handle");
        AddCase(report, "TransactionHook", false, "no edit");
    }

    auto nested = tree.FindByPath("transform.position.x");
    AddCase(report, "NestedPath", nested != nullptr, nested ? "ok" : "missing");
    if (nested && nested->GetHandle()) {
        float x = 0.f;
        (void)nested->GetHandle()->GetFloat(x);
        (void)nested->GetHandle()->SetFloat(9.5f);
        AddCase(report, "NestedSet", actor.transform.position.x == 9.5f, "x=9.5");
    } else {
        AddCase(report, "NestedSet", false, "missing node");
    }

    // Flags filter: hide advanced
    PropertyFilterOptions filter{};
    filter.showAdvanced = false;
    filter.searchText = "health";
    details->SetFilter(filter);
    AddCase(report, "SearchFilter", !details->GetTree().GetFilteredRootNodes().empty(), "health search");

    // Multi-object
    TestActor a{};
    TestActor b{};
    a.health = 10;
    b.health = 20;
    a.active = true;
    b.active = true;
    details->SetObjects(actorId, {&a, &b});
    auto multiHealth = details->GetTree().FindByPath("health");
    AddCase(
        report,
        "MultiMixed",
        multiHealth && multiHealth->GetValueState() == PropertyValueState::Mixed,
        "mixed health");
    auto multiActive = details->GetTree().FindByPath("active");
    AddCase(
        report,
        "MultiIdentical",
        multiActive && multiActive->GetValueState() == PropertyValueState::Identical,
        "identical active");
    if (multiHealth && multiHealth->GetHandle()) {
        (void)multiHealth->GetHandle()->SetInt32(55);
        AddCase(report, "MultiWriteAll", a.health == 55 && b.health == 55, "both written");
    } else {
        AddCase(report, "MultiWriteAll", false, "no handle");
    }

    MultiObjectBinding binding;
    binding.typeId = actorId;
    binding.instances = {&a, &b};
    const auto merged = MergeCommonProperties(*registry, binding, {});
    AddCase(report, "MergeCommonProperties", !merged.empty(), "merged");

    // Enum
    TestEnumHolder holder{};
    details->SetObject(enumHolderId, &holder);
    auto modeNode = details->GetTree().FindByPath("mode");
    AddCase(report, "EnumNode", modeNode != nullptr, "mode");

    // Customization registration
    bool customized = false;
    runtime->RegisterDetailCustomization(actorId, [&]() {
        struct Custom final : IDetailCustomization {
            bool* flag = nullptr;
            void CustomizeDetails(IDetailsView&) override {
                if (flag) {
                    *flag = true;
                }
            }
        };
        auto c = std::make_unique<Custom>();
        c->flag = &customized;
        return c;
    });
    details->SetObject(actorId, &actor);
    AddCase(report, "DetailCustomization", customized, "invoked");

    // Shim AddProperty still works
    PropertyEditor shim;
    Property prop;
    prop.name = "ShimFloat";
    prop.category = "Test";
    prop.type = PropertyType::Float;
    prop.value = "1.25";
    bool shimChanged = false;
    prop.onValueChanged = [&](const std::string&) { shimChanged = true; };
    shim.AddProperty(prop);
    shim.SetPropertyValue("ShimFloat", "2.5");
    AddCase(
        report,
        "ShimAddProperty",
        shim.GetPropertyValue("ShimFloat") == "2.5" && shimChanged,
        "compat");

    runtime->Shutdown();
    AddCase(report, "Shutdown", true, "ok");

    report.success = report.failed == 0;
    std::ostringstream oss;
    oss << "PropertyEditorTests: " << report.passed << " passed, " << report.failed << " failed";
    report.summary = oss.str();
    return report;
}

} // namespace we::editor::property
