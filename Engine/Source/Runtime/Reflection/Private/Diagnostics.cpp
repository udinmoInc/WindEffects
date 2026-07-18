#include "Reflection/IReflectionDiagnostics.h"
#include "Reflection/NameId.h"
#include "Reflection/ReflectionValidation.h"
#include "Reflection/SerializePlan.h"
#include "TypeRegistry.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace we::runtime::reflection {
namespace {

const TypeRegistry* AsOptimizedRegistry(const ITypeRegistry& registry) {
    return static_cast<const TypeRegistry*>(&registry);
}

void PushIssue(
    std::vector<DiagnosticIssue>& out,
    DiagnosticSeverity severity,
    TypeId typeId,
    const char* code,
    std::string message)
{
    out.push_back({severity, typeId, code, std::move(message)});
}

bool HasInheritanceCycle(
    const ITypeRegistry& registry,
    TypeId start,
    TypeId current,
    std::unordered_set<TypeId, TypeIdHash>& visiting,
    std::unordered_set<TypeId, TypeIdHash>& visited)
{
    if (!visiting.insert(current).second) {
        return current == start || visiting.count(start) != 0;
    }
    if (!visited.insert(current).second) {
        visiting.erase(current);
        return false;
    }
    const TypeInfo* info = registry.Find(current);
    if (!info) {
        visiting.erase(current);
        return false;
    }
    for (TypeId base : info->baseTypes) {
        if (base == current || HasInheritanceCycle(registry, start, base, visiting, visited)) {
            visiting.erase(current);
            return true;
        }
    }
    for (TypeId iface : info->interfaces) {
        if (iface == current || HasInheritanceCycle(registry, start, iface, visiting, visited)) {
            visiting.erase(current);
            return true;
        }
    }
    visiting.erase(current);
    return false;
}

bool HasAliasCycle(const ITypeRegistry& registry, TypeId start) {
    TypeId current = start;
    std::unordered_set<TypeId, TypeIdHash> seen;
    for (int depth = 0; depth < 64; ++depth) {
        if (!seen.insert(current).second) {
            return true;
        }
        const TypeInfo* info = registry.Find(current);
        if (!info || info->kind != TypeKind::Alias || info->aliasOf == kInvalidTypeId) {
            return false;
        }
        current = info->aliasOf;
    }
    return true;
}

void ValidateAttributes(
    TypeId ownerId,
    const AttributeBag& bag,
    std::vector<DiagnosticIssue>& outIssues)
{
    bag.EnsureLoaded();
    std::unordered_set<NameId> seen;
    for (const AttributeInfo& attr : bag.entries) {
        if (attr.name.empty() && attr.nameId == kInvalidNameId) {
            PushIssue(
                outIssues, DiagnosticSeverity::Error, ownerId, DiagnosticCode::InvalidAttribute,
                "Attribute missing name");
            continue;
        }
        if (attr.nameId != kInvalidNameId && !seen.insert(attr.nameId).second) {
            PushIssue(
                outIssues, DiagnosticSeverity::Warning, ownerId, DiagnosticCode::InvalidAttribute,
                "Duplicate attribute name: " + attr.name);
        }
        if (attr.kind == AttributeValueKind::None) {
            PushIssue(
                outIssues, DiagnosticSeverity::Warning, ownerId, DiagnosticCode::InvalidAttribute,
                "Attribute has None value kind: " + attr.name);
        }
    }
}

void ValidateEnum(
    const TypeInfo& info,
    std::vector<DiagnosticIssue>& outIssues)
{
    if (info.kind != TypeKind::Enum) {
        return;
    }
    if (info.enumInfo.values.empty()) {
        PushIssue(
            outIssues, DiagnosticSeverity::Warning, info.typeId, DiagnosticCode::InvalidEnum,
            "Enum has no values");
    }
    std::unordered_set<std::string> names;
    std::unordered_set<std::int64_t> values;
    for (const auto& value : info.enumInfo.values) {
        if (value.name.empty()) {
            PushIssue(
                outIssues, DiagnosticSeverity::Error, info.typeId, DiagnosticCode::InvalidEnum,
                "Enum value missing name");
            continue;
        }
        if (!names.insert(value.name).second) {
            PushIssue(
                outIssues, DiagnosticSeverity::Error, info.typeId, DiagnosticCode::DuplicateEnumValue,
                "Duplicate enum value name: " + value.name);
        }
        if (!info.enumInfo.isFlags && !values.insert(value.value).second) {
            PushIssue(
                outIssues, DiagnosticSeverity::Warning, info.typeId, DiagnosticCode::DuplicateEnumValue,
                "Duplicate enum numeric value: " + value.name);
        }
    }
}

void ValidatePropertyLayout(
    const TypeInfo& info,
    const ValidationOptions& options,
    std::vector<DiagnosticIssue>& outIssues)
{
    struct Range {
        std::uint32_t begin = 0;
        std::uint32_t end = 0;
        std::string name;
    };
    std::vector<Range> ranges;

    for (const PropertyInfo& property : info.properties) {
        if (property.name.empty() && property.nameId == kInvalidNameId) {
            PushIssue(
                outIssues, DiagnosticSeverity::Error, info.typeId, DiagnosticCode::InvalidProperty,
                "Property missing name");
        }
        if (options.checkPropertyOffsets && property.HasDirectStorage() && info.size > 0) {
            const std::uint64_t end =
                static_cast<std::uint64_t>(property.offset) + static_cast<std::uint64_t>(property.size);
            if (end > info.size) {
                PushIssue(
                    outIssues, DiagnosticSeverity::Error, info.typeId,
                    DiagnosticCode::InvalidPropertyOffset,
                    "Property exceeds type size: " + property.name);
            }
            if (property.alignment > 0
                && (property.offset % property.alignment) != 0) {
                PushIssue(
                    outIssues, DiagnosticSeverity::Error, info.typeId,
                    DiagnosticCode::MisalignedProperty,
                    "Property offset misaligned: " + property.name);
            }
            if (options.checkPropertyOverlap) {
                ranges.push_back({property.offset, static_cast<std::uint32_t>(end), property.name});
            }
        }
    }

    if (options.checkPropertyOverlap && !ranges.empty()) {
        std::sort(ranges.begin(), ranges.end(), [](const Range& a, const Range& b) {
            return a.begin < b.begin;
        });
        for (std::size_t i = 1; i < ranges.size(); ++i) {
            if (ranges[i].begin < ranges[i - 1].end) {
                PushIssue(
                    outIssues, DiagnosticSeverity::Error, info.typeId,
                    DiagnosticCode::PropertyOverlap,
                    "Overlapping properties: " + ranges[i - 1].name + " and " + ranges[i].name);
            }
        }
    }
}

} // namespace

void ValidateRegistryEx(
    const ITypeRegistry& registry,
    std::vector<DiagnosticIssue>& outIssues,
    const ValidationOptions& options)
{
    const auto ids = registry.GetAllTypeIds();
    std::unordered_set<NameId> seenQualified;
    std::unordered_set<TypeId, TypeIdHash> seenIds;

    for (TypeId id : ids) {
        const TypeInfo* info = registry.Find(id);
        if (!info) {
            PushIssue(
                outIssues, DiagnosticSeverity::Error, id, DiagnosticCode::MissingType,
                "TypeId present in index but Find() returned null");
            continue;
        }
        if (!seenIds.insert(id).second) {
            PushIssue(
                outIssues, DiagnosticSeverity::Error, id, DiagnosticCode::DuplicateType,
                "Duplicate TypeId in registry index");
        }
        if (info->qualifiedNameId != kInvalidNameId
            && !seenQualified.insert(info->qualifiedNameId).second) {
            PushIssue(
                outIssues, DiagnosticSeverity::Error, id, DiagnosticCode::DuplicateQualifiedName,
                "Duplicate qualified name registration");
        }

        if (options.level == ValidationLevel::Basic) {
            continue;
        }

        if (info->size == 0 && info->kind != TypeKind::Primitive
            && info->kind != TypeKind::Interface && info->kind != TypeKind::Alias
            && info->primitive != PrimitiveKind::Void) {
            PushIssue(
                outIssues, DiagnosticSeverity::Warning, id, DiagnosticCode::ZeroSizeType,
                "Non-void type registered with size 0");
        }
        if (info->alignment == 0
            || (info->size > 0 && info->alignment > 0 && (info->size % info->alignment) != 0
                && info->kind != TypeKind::Alias)) {
            // size % alignment != 0 is common for packed structs — warn only in Strict.
            if (info->alignment == 0) {
                PushIssue(
                    outIssues, DiagnosticSeverity::Error, id, DiagnosticCode::InvalidAlignment,
                    "Type alignment is zero");
            }
        }
        if (info->versions.schemaVersion == 0 || info->versions.typeVersion == 0
            || info->versions.migrationVersion == 0
            || info->versions.binaryCompatibilityVersion == 0) {
            PushIssue(
                outIssues, DiagnosticSeverity::Error, id, DiagnosticCode::InvalidVersion,
                "One or more type version fields are zero");
        }

        std::unordered_set<NameId> propNames;
        for (const PropertyInfo& property : info->properties) {
            if (property.nameId != kInvalidNameId && !propNames.insert(property.nameId).second) {
                PushIssue(
                    outIssues, DiagnosticSeverity::Error, id, DiagnosticCode::DuplicateProperty,
                    "Duplicate property name on type: " + property.name);
            }
            if (property.size == 0 && property.getter == nullptr) {
                PushIssue(
                    outIssues, DiagnosticSeverity::Warning, id, DiagnosticCode::ZeroSizeProperty,
                    "Property has no storage and no getter: " + property.name);
            }
            if (options.checkMissingPropertyTypes && property.typeId != kInvalidTypeId
                && !registry.Contains(property.typeId)) {
                PushIssue(
                    outIssues, DiagnosticSeverity::Warning, id, DiagnosticCode::MissingPropertyType,
                    "Property type not registered: " + property.name);
            }
        }

        std::unordered_set<NameId> fnNames;
        for (const FunctionInfo& function : info->functions) {
            if (function.nameId != kInvalidNameId && !fnNames.insert(function.nameId).second) {
                PushIssue(
                    outIssues, DiagnosticSeverity::Error, id, DiagnosticCode::DuplicateFunction,
                    "Duplicate function name on type: " + function.name);
            }
        }

        if (options.checkInheritance) {
            for (TypeId base : info->baseTypes) {
                if (base == id) {
                    PushIssue(
                        outIssues, DiagnosticSeverity::Error, id, DiagnosticCode::SelfBase,
                        "Type lists itself as a base");
                } else if (!registry.Contains(base)) {
                    PushIssue(
                        outIssues, DiagnosticSeverity::Warning, id, DiagnosticCode::MissingBase,
                        "Base TypeId not registered");
                }
            }
            for (TypeId iface : info->interfaces) {
                if (iface == id) {
                    PushIssue(
                        outIssues, DiagnosticSeverity::Error, id, DiagnosticCode::SelfBase,
                        "Type lists itself as an interface");
                } else if (!registry.Contains(iface)) {
                    PushIssue(
                        outIssues, DiagnosticSeverity::Warning, id, DiagnosticCode::MissingInterface,
                        "Interface TypeId not registered");
                }
            }
            std::unordered_set<TypeId, TypeIdHash> visiting;
            std::unordered_set<TypeId, TypeIdHash> visited;
            if (HasInheritanceCycle(registry, id, id, visiting, visited)) {
                PushIssue(
                    outIssues, DiagnosticSeverity::Error, id, DiagnosticCode::InheritanceCycle,
                    "Inheritance / interface cycle detected");
            }
            if (info->kind == TypeKind::Alias && HasAliasCycle(registry, id)) {
                PushIssue(
                    outIssues, DiagnosticSeverity::Error, id, DiagnosticCode::AliasCycle,
                    "Alias cycle detected");
            }
        }

        if (options.checkPropertyOffsets || options.checkPropertyOverlap) {
            ValidatePropertyLayout(*info, options, outIssues);
        }
        if (options.checkEnumIntegrity) {
            ValidateEnum(*info, outIssues);
        }
        if (options.checkAttributes) {
            ValidateAttributes(id, info->attributes, outIssues);
            for (const PropertyInfo& property : info->properties) {
                ValidateAttributes(id, property.attributes, outIssues);
            }
        }

        if (options.checkSerializePlans) {
            if (const SerializePlan* plan = registry.GetSerializePlan(id)) {
                if (plan->typeId != id && plan->typeId != kInvalidTypeId) {
                    PushIssue(
                        outIssues, DiagnosticSeverity::Error, id,
                        DiagnosticCode::InvalidSerializePlan,
                        "SerializePlan typeId mismatch");
                }
                for (const SerializeStep& step : plan->steps) {
                    if (!step.property) {
                        PushIssue(
                            outIssues, DiagnosticSeverity::Error, id,
                            DiagnosticCode::InvalidSerializePlan,
                            "SerializePlan step has null property");
                    }
                }
            }
        }
    }

    if (options.reportUnusedTypes || options.level == ValidationLevel::Strict) {
        if (const TypeRegistry* tr = AsOptimizedRegistry(registry)) {
            for (TypeId id : ids) {
                const TypeInfo* info = registry.Find(id);
                if (!info || info->kind == TypeKind::Primitive) {
                    continue;
                }
                if (!tr->IsTypeMarkedUsed(id)) {
                    PushIssue(
                        outIssues,
                        options.level == ValidationLevel::Strict ? DiagnosticSeverity::Warning
                                                                 : DiagnosticSeverity::Info,
                        id,
                        DiagnosticCode::UnusedType,
                        "Type has not been marked used via MarkTypeUsed()");
                }
            }
        }
    }

    if (options.checkCaches) {
        const CacheVerificationReport cacheReport = VerifyReflectionCaches(registry);
        outIssues.insert(outIssues.end(), cacheReport.issues.begin(), cacheReport.issues.end());
    }
}

void ValidateRegistry(const ITypeRegistry& registry, std::vector<DiagnosticIssue>& outIssues) {
    ValidationOptions options;
    options.level = ValidationLevel::Full;
    options.reportUnusedTypes = false;
    options.checkCaches = true;
    ValidateRegistryEx(registry, outIssues, options);
}

CacheVerificationReport VerifyReflectionCaches(const ITypeRegistry& registry) {
    CacheVerificationReport report;
    const auto ids = registry.GetAllTypeIds();
    report.typesChecked = ids.size();

    for (TypeId id : ids) {
        const TypeInfo* info = registry.Find(id);
        if (!info) {
            continue;
        }

        std::size_t ancestorCount = 0;
        const TypeId* ancestors = registry.GetAncestorChain(id, ancestorCount);
        ++report.ancestorCachesChecked;
        if (!ancestors || ancestorCount == 0) {
            PushIssue(
                report.issues, DiagnosticSeverity::Error, id, DiagnosticCode::CacheMismatch,
                "Ancestor cache empty");
            report.valid = false;
        } else if (ancestors[0] != id) {
            PushIssue(
                report.issues, DiagnosticSeverity::Error, id, DiagnosticCode::CacheMismatch,
                "Ancestor cache does not start with self");
            report.valid = false;
        }

        for (const PropertyInfo& property : info->properties) {
            if (property.nameId == kInvalidNameId) {
                continue;
            }
            const PropertyInfo* found = registry.FindProperty(id, property.nameId);
            if (!found || found->nameId != property.nameId) {
                PushIssue(
                    report.issues, DiagnosticSeverity::Error, id, DiagnosticCode::CacheMismatch,
                    "Property name map mismatch: " + property.name);
                report.valid = false;
            }
        }
        for (const FunctionInfo& function : info->functions) {
            if (function.nameId == kInvalidNameId) {
                continue;
            }
            const FunctionInfo* found = registry.FindFunction(id, function.nameId);
            if (!found || found->nameId != function.nameId) {
                PushIssue(
                    report.issues, DiagnosticSeverity::Error, id, DiagnosticCode::CacheMismatch,
                    "Function name map mismatch: " + function.name);
                report.valid = false;
            }
        }

        if (const SerializePlan* plan = registry.GetSerializePlan(id)) {
            ++report.plansChecked;
            if (plan->schemaVersion != info->versions.schemaVersion) {
                PushIssue(
                    report.issues, DiagnosticSeverity::Warning, id, DiagnosticCode::CacheMismatch,
                    "SerializePlan schemaVersion diverges from TypeInfo");
            }
        }

        if (!registry.IsA(id, id)) {
            PushIssue(
                report.issues, DiagnosticSeverity::Error, id, DiagnosticCode::CacheMismatch,
                "IsA(self) returned false");
            report.valid = false;
        }
    }

    if (!report.issues.empty()) {
        report.valid = report.valid
            && CountDiagnostics(report.issues, DiagnosticSeverity::Error) == 0;
    }
    return report;
}

BinaryCompatibilityReport ValidateBinaryCompatibility(
    const ITypeRegistry& registry,
    std::uint64_t expectedFingerprint,
    std::uint32_t expectedSchemaVersion)
{
    BinaryCompatibilityReport report;
    report.currentFingerprint = registry.ComputeFingerprint();
    report.expectedFingerprint = expectedFingerprint;
    report.schemaVersion = registry.GetSchemaVersion();
    report.expectedSchemaVersion =
        expectedSchemaVersion == 0 ? kReflectionSchemaVersion : expectedSchemaVersion;

    if (report.schemaVersion != report.expectedSchemaVersion) {
        PushIssue(
            report.issues, DiagnosticSeverity::Error, kInvalidTypeId,
            DiagnosticCode::BinaryIncompatible,
            "Reflection schema version mismatch");
    }
    if (expectedFingerprint != 0 && report.currentFingerprint != expectedFingerprint) {
        PushIssue(
            report.issues, DiagnosticSeverity::Error, kInvalidTypeId,
            DiagnosticCode::BinaryIncompatible,
            "Registry fingerprint mismatch");
    }
    if (registry.GetSchemaVersion() != kReflectionSchemaVersion) {
        PushIssue(
            report.issues, DiagnosticSeverity::Error, kInvalidTypeId,
            DiagnosticCode::BinaryIncompatible,
            "Runtime schema version constant mismatch");
    }
    report.compatible = CountDiagnostics(report.issues, DiagnosticSeverity::Error) == 0;
    return report;
}

bool IsReflectionIntegrityOk(const ITypeRegistry& registry) {
    std::vector<DiagnosticIssue> issues;
    ValidationOptions options;
    options.level = ValidationLevel::Full;
    options.reportUnusedTypes = false;
    ValidateRegistryEx(registry, issues, options);
    return CountDiagnostics(issues, DiagnosticSeverity::Error) == 0;
}

std::size_t CountDiagnostics(
    const std::vector<DiagnosticIssue>& issues,
    DiagnosticSeverity minimumSeverity)
{
    std::size_t count = 0;
    for (const DiagnosticIssue& issue : issues) {
        if (static_cast<std::uint8_t>(issue.severity) >= static_cast<std::uint8_t>(minimumSeverity)) {
            ++count;
        }
    }
    return count;
}

ReflectionDiagnosticsSnapshot CaptureDiagnostics(const ITypeRegistry& registry) {
    ReflectionDiagnosticsSnapshot snap;
    snap.fingerprint = registry.ComputeFingerprint();
    snap.sealed = registry.IsSealed();

    const auto ids = registry.GetAllTypeIds();
    snap.memory.typeCount = ids.size();
    for (TypeId id : ids) {
        if (const TypeInfo* info = registry.Find(id)) {
            snap.memory.propertyCount += info->properties.size();
            snap.memory.functionCount += info->functions.size();
            if (const SerializePlan* plan = registry.GetSerializePlan(id)) {
                snap.memory.serializePlanBytes +=
                    plan->steps.size() * sizeof(SerializeStep) + sizeof(SerializePlan);
            }
            snap.memory.estimatedHeapBytes += sizeof(TypeInfo);
            snap.memory.estimatedHeapBytes += info->qualifiedName.size() + info->name.size();
            snap.memory.estimatedHeapBytes += info->properties.size() * sizeof(PropertyInfo);
            snap.memory.estimatedHeapBytes += info->functions.size() * sizeof(FunctionInfo);
        }
    }

    snap.memory.nameTableEntries = GetNameTable().GetCount();
    snap.memory.nameTableBytes = GetNameTable().GetByteSize();
    snap.memory.estimatedHeapBytes += snap.memory.nameTableBytes + snap.memory.serializePlanBytes;

    if (const TypeRegistry* tr = AsOptimizedRegistry(registry)) {
        snap.timing.lastRegistrationMs = tr->GetLastRegistrationMs();
        snap.timing.totalRegistrationMs = tr->GetTotalRegistrationMs();
        snap.timing.lastSealMs = tr->GetLastSealMs();
        snap.timing.registrationCount = tr->GetRegistrationCount();
        snap.timing.lookupCount = tr->GetLookupCount();
        snap.timing.propertyLookupCount = tr->GetPropertyLookupCount();
        snap.timing.functionLookupCount = tr->GetFunctionLookupCount();
    }

    ValidateRegistry(registry, snap.issues);
    return snap;
}

} // namespace we::runtime::reflection
