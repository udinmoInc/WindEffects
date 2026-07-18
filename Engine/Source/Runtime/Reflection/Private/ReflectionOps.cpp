#include "Reflection/ReflectionOps.h"
#include "Reflection/ITypeRegistry.h"
#include "Reflection/PropertyPath.h"
#include "Reflection/SerializePlan.h"
#include "HashUtils.h"

#include <algorithm>
#include <cstring>
#include <new>

namespace we::runtime::reflection {
namespace {

void MixBytes(std::uint64_t& hash, const void* data, std::size_t size) {
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    for (std::size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= 1099511628211ull;
    }
}

bool ReadPropertyBytes(
    const PropertyInfo& property,
    const void* instance,
    std::vector<std::uint8_t>& out)
{
    if (!instance || property.size == 0) {
        return false;
    }
    out.resize(property.size);
    if (property.getter) {
        return property.getter(instance, out.data(), out.size());
    }
    const void* src = property.ConstPtr(instance);
    if (!src) {
        return false;
    }
    std::memcpy(out.data(), src, property.size);
    return true;
}

bool WritePropertyBytes(
    const PropertyInfo& property,
    void* instance,
    const std::uint8_t* data,
    std::size_t size)
{
    if (!instance || !data || size == 0) {
        return false;
    }
    if (HasFlag(property.flags, PropertyFlags::ReadOnly) && !property.setter) {
        return false;
    }
    if (property.setter) {
        return property.setter(instance, data, size);
    }
    if (property.size == 0 || size < property.size) {
        return false;
    }
    void* dst = property.MutablePtr(instance);
    if (!dst) {
        return false;
    }
    std::memcpy(dst, data, property.size);
    return true;
}

void CollectDiffRecursive(
    const ITypeRegistry& registry,
    TypeId typeId,
    const void* left,
    const void* right,
    const std::string& prefix,
    bool serializedOnly,
    ReflectionDiff& diff)
{
    const TypeInfo* info = registry.Resolve(typeId);
    if (!info || !left || !right) {
        return;
    }

    // Walk bases first for stable path ordering.
    for (TypeId base : info->baseTypes) {
        CollectDiffRecursive(registry, base, left, right, prefix, serializedOnly, diff);
    }

    for (const PropertyInfo& property : info->properties) {
        if (serializedOnly && !property.IsSerialized()) {
            continue;
        }
        std::string path = prefix.empty() ? property.name : (prefix + "." + property.name);

        const TypeInfo* propType = registry.Find(property.typeId);
        if (propType && propType->IsClassOrStruct() && property.HasDirectStorage()) {
            const void* leftChild = property.ConstPtr(left);
            const void* rightChild = property.ConstPtr(right);
            CollectDiffRecursive(
                registry, property.typeId, leftChild, rightChild, path, serializedOnly, diff);
            continue;
        }

        std::vector<std::uint8_t> leftBytes;
        std::vector<std::uint8_t> rightBytes;
        if (!ReadPropertyBytes(property, left, leftBytes)
            || !ReadPropertyBytes(property, right, rightBytes)) {
            continue;
        }
        if (leftBytes == rightBytes) {
            continue;
        }
        ReflectionDiffEntry entry;
        entry.path = std::move(path);
        entry.propertyTypeId = property.typeId;
        entry.offset = property.offset;
        entry.size = property.size;
        diff.entries.push_back(std::move(entry));
        diff.identical = false;
    }
}

} // namespace

void* CloneObject(const ITypeRegistry& registry, TypeId typeId, const void* source) {
    const TypeInfo* info = registry.Resolve(typeId);
    if (!info || !source) {
        return nullptr;
    }
    void* instance = nullptr;
    if (info->ops.defaultConstruct) {
        instance = info->ops.defaultConstruct();
    } else if (info->ops.placementConstruct && info->size > 0) {
        void* memory = ::operator new(info->size, std::align_val_t{info->alignment});
        info->ops.placementConstruct(memory);
        instance = memory;
    }
    if (!instance) {
        return nullptr;
    }
    if (!CopyObject(registry, typeId, instance, source)) {
        DestroyClonedObject(registry, typeId, instance);
        return nullptr;
    }
    return instance;
}

void DestroyClonedObject(const ITypeRegistry& registry, TypeId typeId, void* instance) {
    if (!instance) {
        return;
    }
    const TypeInfo* info = registry.Resolve(typeId);
    if (!info) {
        return;
    }
    if (info->ops.destruct) {
        info->ops.destruct(instance);
        return;
    }
    if (info->ops.placementDestruct) {
        info->ops.placementDestruct(instance);
        ::operator delete(instance, std::align_val_t{info->alignment});
    }
}

bool CopyObject(
    const ITypeRegistry& registry,
    TypeId typeId,
    void* destination,
    const void* source)
{
    const TypeInfo* info = registry.Resolve(typeId);
    if (!info || !destination || !source) {
        return false;
    }
    if (info->ops.copyAssign) {
        info->ops.copyAssign(destination, source);
        return true;
    }
    if (info->ops.copyConstruct) {
        // Already constructed dest — fall through to property copy.
    }
    if (HasFlag(info->flags, TypeFlags::TrivialCopy) || HasFlag(info->flags, TypeFlags::Pod)) {
        if (info->size == 0) {
            return false;
        }
        std::memcpy(destination, source, info->size);
        return true;
    }

    // Property-wise POD copy of direct-storage fields (including bases via serialize plan).
    if (const SerializePlan* plan = registry.GetSerializePlan(typeId)) {
        for (const SerializeStep& step : plan->steps) {
            if (!step.property || !step.property->HasDirectStorage()) {
                continue;
            }
            const void* src = step.property->ConstPtr(source);
            void* dst = step.property->MutablePtr(destination);
            if (!src || !dst) {
                return false;
            }
            std::memcpy(dst, src, step.size);
        }
        return true;
    }
    return false;
}

int CompareObjects(
    const ITypeRegistry& registry,
    TypeId typeId,
    const void* left,
    const void* right)
{
    if (left == right) {
        return 0;
    }
    if (!left) {
        return -1;
    }
    if (!right) {
        return 1;
    }
    const ReflectionDiff diff = DiffObjects(registry, typeId, left, right, true);
    if (diff.identical) {
        return 0;
    }
    // Deterministic: compare first differing property bytes.
    for (const ReflectionDiffEntry& entry : diff.entries) {
        const ResolvedPropertyPath path = ResolvePropertyPath(registry, typeId, entry.path);
        if (!path.valid || !path.Leaf()) {
            continue;
        }
        std::vector<std::uint8_t> leftBytes;
        std::vector<std::uint8_t> rightBytes;
        const void* leftLeaf = left;
        const void* rightLeaf = right;
        for (std::size_t i = 0; i + 1 < path.properties.size(); ++i) {
            leftLeaf = path.properties[i]->ConstPtr(leftLeaf);
            rightLeaf = path.properties[i]->ConstPtr(rightLeaf);
            if (!leftLeaf || !rightLeaf) {
                return leftLeaf ? 1 : -1;
            }
        }
        if (!ReadPropertyBytes(*path.Leaf(), leftLeaf, leftBytes)
            || !ReadPropertyBytes(*path.Leaf(), rightLeaf, rightBytes)) {
            continue;
        }
        const int cmp = std::memcmp(
            leftBytes.data(),
            rightBytes.data(),
            std::min(leftBytes.size(), rightBytes.size()));
        if (cmp != 0) {
            return cmp;
        }
        if (leftBytes.size() != rightBytes.size()) {
            return leftBytes.size() < rightBytes.size() ? -1 : 1;
        }
    }
    return 0;
}

bool AreObjectsEqual(
    const ITypeRegistry& registry,
    TypeId typeId,
    const void* left,
    const void* right)
{
    return CompareObjects(registry, typeId, left, right) == 0;
}

ReflectionDiff DiffObjects(
    const ITypeRegistry& registry,
    TypeId typeId,
    const void* left,
    const void* right,
    bool serializedOnly)
{
    ReflectionDiff diff;
    diff.typeId = typeId;
    diff.identical = true;
    CollectDiffRecursive(registry, typeId, left, right, {}, serializedOnly, diff);
    return diff;
}

ReflectionPatch BuildPatch(
    const ITypeRegistry& registry,
    TypeId typeId,
    const void* from,
    const void* to,
    bool serializedOnly)
{
    ReflectionPatch patch;
    patch.typeId = typeId;
    if (const TypeInfo* info = registry.Find(typeId)) {
        patch.schemaVersion = info->versions.schemaVersion;
    }
    const ReflectionDiff diff = DiffObjects(registry, typeId, from, to, serializedOnly);
    for (const ReflectionDiffEntry& entry : diff.entries) {
        ReflectionPatchEntry patchEntry;
        patchEntry.path = entry.path;
        const ResolvedPropertyPath path = ResolvePropertyPath(registry, typeId, entry.path);
        if (!path.valid || !path.Leaf()) {
            continue;
        }
        const void* cursor = to;
        for (std::size_t i = 0; i + 1 < path.properties.size(); ++i) {
            cursor = path.properties[i]->ConstPtr(cursor);
            if (!cursor) {
                cursor = nullptr;
                break;
            }
        }
        if (!cursor || !ReadPropertyBytes(*path.Leaf(), cursor, patchEntry.bytes)) {
            continue;
        }
        patch.entries.push_back(std::move(patchEntry));
    }
    return patch;
}

bool ApplyPatch(const ITypeRegistry& registry, void* instance, const ReflectionPatch& patch) {
    if (!instance) {
        return false;
    }
    for (const ReflectionPatchEntry& entry : patch.entries) {
        if (!SetPropertyPathRaw(
                registry,
                patch.typeId,
                instance,
                entry.path,
                entry.bytes.data(),
                entry.bytes.size())) {
            return false;
        }
    }
    return true;
}

std::uint64_t HashObject(
    const ITypeRegistry& registry,
    TypeId typeId,
    const void* instance,
    bool serializedOnly)
{
    std::uint64_t hash = detail::Fnv1a64("we.reflection.object.v1");
    hash ^= typeId;
    hash *= 1099511628211ull;
    if (!instance) {
        return hash == 0 ? 1ull : hash;
    }

    if (serializedOnly) {
        if (const SerializePlan* plan = registry.GetSerializePlan(typeId)) {
            for (const SerializeStep& step : plan->steps) {
                if (!step.property) {
                    continue;
                }
                std::vector<std::uint8_t> bytes;
                if (!ReadPropertyBytes(*step.property, instance, bytes)) {
                    continue;
                }
                hash ^= step.property->nameId;
                hash *= 1099511628211ull;
                MixBytes(hash, bytes.data(), bytes.size());
            }
            return hash == 0 ? 1ull : hash;
        }
    }

    const TypeInfo* info = registry.Resolve(typeId);
    if (!info) {
        return hash == 0 ? 1ull : hash;
    }
    for (const PropertyInfo& property : info->properties) {
        if (serializedOnly && !property.IsSerialized()) {
            continue;
        }
        std::vector<std::uint8_t> bytes;
        if (!ReadPropertyBytes(property, instance, bytes)) {
            continue;
        }
        hash ^= property.nameId;
        hash *= 1099511628211ull;
        MixBytes(hash, bytes.data(), bytes.size());
    }
    return hash == 0 ? 1ull : hash;
}

std::uint64_t ComputeStableSerializationId(const ITypeRegistry& registry, TypeId typeId) {
    std::uint64_t hash = detail::Fnv1a64("we.reflection.stable_id.v1");
    hash ^= kReflectionSerializationEpoch;
    hash *= 1099511628211ull;
    hash ^= typeId;
    hash *= 1099511628211ull;
    if (const TypeInfo* info = registry.Find(typeId)) {
        hash ^= info->versions.schemaVersion;
        hash *= 1099511628211ull;
        hash ^= info->versions.binaryCompatibilityVersion;
        hash *= 1099511628211ull;
        hash ^= info->qualifiedNameId;
        hash *= 1099511628211ull;
    }
    return hash == 0 ? 1ull : hash;
}

} // namespace we::runtime::reflection
