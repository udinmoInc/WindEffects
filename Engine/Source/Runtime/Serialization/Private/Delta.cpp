#include "Serialization/Delta.h"
#include "Serialization/ISerializer.h"

#include "Reflection/PropertyPath.h"
#include "Reflection/ReflectionOps.h"

#include <cstring>

namespace we::runtime::serialization {

BinaryDiff DiffInstances(
    const reflection::ITypeRegistry& registry,
    reflection::TypeId typeId,
    const void* left,
    const void* right,
    bool serializedOnly)
{
    BinaryDiff diff;
    diff.typeId = typeId;
    const reflection::ReflectionDiff refl = reflection::DiffObjects(
        registry, typeId, left, right, serializedOnly);
    diff.identical = refl.identical;
    for (const reflection::ReflectionDiffEntry& entry : refl.entries) {
        BinaryDiffEntry out;
        out.path = entry.path;
        out.offset = entry.offset;
        out.size = entry.size;

        const reflection::ResolvedPropertyPath path =
            reflection::ResolvePropertyPath(registry, typeId, entry.path);
        if (path.valid && path.Leaf() && left && right) {
            const void* leftCursor = left;
            const void* rightCursor = right;
            for (std::size_t i = 0; i + 1 < path.properties.size(); ++i) {
                leftCursor = path.properties[i]->ConstPtr(leftCursor);
                rightCursor = path.properties[i]->ConstPtr(rightCursor);
            }
            if (leftCursor && rightCursor && path.Leaf()->HasDirectStorage()) {
                out.before.resize(path.Leaf()->size);
                out.after.resize(path.Leaf()->size);
                std::memcpy(out.before.data(), path.Leaf()->ConstPtr(leftCursor), path.Leaf()->size);
                std::memcpy(out.after.data(), path.Leaf()->ConstPtr(rightCursor), path.Leaf()->size);
            }
        }
        diff.entries.push_back(std::move(out));
    }
    return diff;
}

BinaryDelta GenerateDelta(
    ISerializer& serializer,
    const reflection::ITypeRegistry& registry,
    reflection::TypeId typeId,
    const void* baseInstance,
    const void* targetInstance,
    const SerializeOptions& options)
{
    BinaryDelta delta;
    delta.typeId = typeId;
    delta.baseFingerprint = reflection::HashObject(registry, typeId, baseInstance, true);
    delta.targetFingerprint = reflection::HashObject(registry, typeId, targetInstance, true);
    if (const reflection::TypeInfo* info = registry.Find(typeId)) {
        delta.schemaVersion = info->versions.schemaVersion;
    }
    delta.patch = reflection::BuildPatch(registry, typeId, baseInstance, targetInstance, true);

    SerializeOptions deltaOptions = options;
    deltaOptions.mode = SerializeMode::Delta;
    // Encode patch as a document containing the base snapshot + patch metadata via target bytes.
    delta.encoded = serializer.SerializeObject(typeId, targetInstance, deltaOptions);
    return delta;
}

bool ApplyDelta(
    const reflection::ITypeRegistry& registry,
    void* instance,
    const BinaryDelta& delta)
{
    if (!instance) {
        return false;
    }
    return reflection::ApplyPatch(registry, instance, delta.patch);
}

Snapshot CaptureSnapshot(
    ISerializer& serializer,
    reflection::TypeId typeId,
    const void* instance,
    const SerializeOptions& options)
{
    Snapshot snap;
    snap.typeId = typeId;
    SerializeOptions snapOptions = options;
    snapOptions.mode = SerializeMode::Snapshot;
    snap.bytes = serializer.SerializeObject(typeId, instance, snapOptions);
    return snap;
}

bool RestoreSnapshot(
    ISerializer& serializer,
    reflection::TypeId typeId,
    void* instance,
    const Snapshot& snapshot,
    const SerializeOptions& options)
{
    if (!instance || snapshot.bytes.empty()) {
        return false;
    }
    return serializer.DeserializeObject(
        typeId, instance, snapshot.bytes.data(), snapshot.bytes.size(), options);
}

std::vector<std::uint8_t> SerializePartial(
    ISerializer& serializer,
    const reflection::ITypeRegistry& registry,
    reflection::TypeId typeId,
    const void* instance,
    const std::vector<std::string>& propertyPaths,
    const SerializeOptions& options)
{
    (void)registry;
    (void)propertyPaths;
    // v1: partial mode flag on full serialize — property filtering reserved for schema v2.
    SerializeOptions partial = options;
    partial.mode = SerializeMode::Partial;
    return serializer.SerializeObject(typeId, instance, partial);
}

} // namespace we::runtime::serialization
