#include "Reflection/IReflectionDiagnostics.h"
#include "Reflection/NameId.h"
#include "Reflection/SerializePlan.h"
#include "TypeRegistry.h"

#include <unordered_set>

namespace we::runtime::reflection {
namespace {

const TypeRegistry* AsOptimizedRegistry(const ITypeRegistry& registry) {
    // Sole production implementation is TypeRegistry.
    return static_cast<const TypeRegistry*>(&registry);
}

} // namespace

void ValidateRegistry(const ITypeRegistry& registry, std::vector<DiagnosticIssue>& outIssues) {
    const auto ids = registry.GetAllTypeIds();
    std::unordered_set<NameId> seenQualified;
    std::unordered_set<TypeId, TypeIdHash> seenIds;

    for (TypeId id : ids) {
        const TypeInfo* info = registry.Find(id);
        if (!info) {
            outIssues.push_back({
                DiagnosticSeverity::Error,
                id,
                "missing_type",
                "TypeId present in index but Find() returned null"});
            continue;
        }
        if (!seenIds.insert(id).second) {
            outIssues.push_back({
                DiagnosticSeverity::Error,
                id,
                "duplicate_type",
                "Duplicate TypeId in registry index"});
        }
        if (info->qualifiedNameId != kInvalidNameId
            && !seenQualified.insert(info->qualifiedNameId).second) {
            outIssues.push_back({
                DiagnosticSeverity::Error,
                id,
                "duplicate_qualified_name",
                "Duplicate qualified name registration"});
        }
        if (info->size == 0 && info->kind != TypeKind::Primitive
            && info->kind != TypeKind::Interface && info->kind != TypeKind::Alias
            && info->primitive != PrimitiveKind::Void) {
            outIssues.push_back({
                DiagnosticSeverity::Warning,
                id,
                "zero_size_type",
                "Non-void type registered with size 0"});
        }
        if (info->versions.schemaVersion == 0 || info->versions.typeVersion == 0
            || info->versions.migrationVersion == 0
            || info->versions.binaryCompatibilityVersion == 0) {
            outIssues.push_back({
                DiagnosticSeverity::Error,
                id,
                "invalid_version",
                "One or more type version fields are zero"});
        }

        std::unordered_set<NameId> propNames;
        for (const PropertyInfo& property : info->properties) {
            if (property.name.empty() && property.nameId == kInvalidNameId) {
                outIssues.push_back({
                    DiagnosticSeverity::Error,
                    id,
                    "invalid_property",
                    "Property missing name"});
            }
            if (property.nameId != kInvalidNameId && !propNames.insert(property.nameId).second) {
                outIssues.push_back({
                    DiagnosticSeverity::Error,
                    id,
                    "duplicate_property",
                    "Duplicate property name on type: " + property.name});
            }
            if (property.size == 0 && property.getter == nullptr) {
                outIssues.push_back({
                    DiagnosticSeverity::Warning,
                    id,
                    "zero_size_property",
                    "Property has no storage and no getter: " + property.name});
            }
        }

        for (TypeId base : info->baseTypes) {
            if (!registry.Contains(base)) {
                outIssues.push_back({
                    DiagnosticSeverity::Warning,
                    id,
                    "missing_base",
                    "Base TypeId not registered"});
            }
        }
    }

    if (const TypeRegistry* tr = AsOptimizedRegistry(registry)) {
        if (auto snap = tr->GetSnapshot()) {
            for (const auto& [id, record] : snap->byId) {
                if (!record || record->used || record->info.kind == TypeKind::Primitive) {
                    continue;
                }
                outIssues.push_back({
                    DiagnosticSeverity::Info,
                    id,
                    "unused_type",
                    "Type has not been marked used via MarkTypeUsed()"});
            }
        }
    }
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
