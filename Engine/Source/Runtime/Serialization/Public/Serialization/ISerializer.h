#pragma once

#include "Serialization/Export.h"
#include "Serialization/ObjectGraph.h"
#include "Serialization/Types.h"
#include "Reflection/ITypeRegistry.h"
#include "Reflection/TypeId.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace we::runtime::serialization {

class IArchive;

/// Custom serializer override for special-case types (registered explicitly).
using CustomSerializeFn = bool (*)(
    IArchive& archive,
    void* instance,
    reflection::TypeId typeId,
    void* userData);

struct SERIALIZATION_API CustomSerializerEntry {
    reflection::TypeId typeId = reflection::kInvalidTypeId;
    CustomSerializeFn serialize = nullptr;
    void* userData = nullptr;
};

SERIALIZATION_API void RegisterCustomSerializer(CustomSerializerEntry entry);
SERIALIZATION_API void UnregisterCustomSerializer(reflection::TypeId typeId);
SERIALIZATION_API void ClearCustomSerializers();
[[nodiscard]] SERIALIZATION_API const CustomSerializerEntry* FindCustomSerializer(
    reflection::TypeId typeId);

/// Live object registered in a serialization graph.
struct SERIALIZATION_API GraphObject {
    ObjectId objectId = kInvalidObjectId;
    reflection::TypeId typeId = reflection::kInvalidTypeId;
    void* instance = nullptr;
    bool owned = false; // if true, deserializer owns allocation via TypeOps
};

/// Object graph used for multi-object serialize / deserialize with reference fixups.
class SERIALIZATION_API ObjectGraph {
public:
    ObjectGraph() = default;
    ~ObjectGraph();

    ObjectGraph(const ObjectGraph&) = delete;
    ObjectGraph& operator=(const ObjectGraph&) = delete;
    ObjectGraph(ObjectGraph&&) noexcept;
    ObjectGraph& operator=(ObjectGraph&&) noexcept;

    [[nodiscard]] ObjectId AddObject(reflection::TypeId typeId, void* instance, bool takeOwnership = false);
    [[nodiscard]] ObjectId AddObjectWithId(
        ObjectId objectId,
        reflection::TypeId typeId,
        void* instance,
        bool takeOwnership = false);

    [[nodiscard]] GraphObject* Find(ObjectId objectId);
    [[nodiscard]] const GraphObject* Find(ObjectId objectId) const;
    [[nodiscard]] std::size_t Size() const noexcept { return m_Objects.size(); }
    [[nodiscard]] const std::vector<GraphObject>& Objects() const noexcept { return m_Objects; }
    [[nodiscard]] ObjectId NextObjectId() const noexcept { return m_NextId; }

    void AddReference(ObjectId from, ObjectReference reference);
    [[nodiscard]] const std::vector<ObjectReference>& ReferencesFrom(ObjectId from) const;
    [[nodiscard]] const std::unordered_map<ObjectId, std::vector<ObjectReference>>& AllReferences() const {
        return m_References;
    }

    void Clear(const reflection::ITypeRegistry* registryForDestroy = nullptr);

private:
    std::vector<GraphObject> m_Objects;
    std::unordered_map<ObjectId, std::size_t> m_Index;
    std::unordered_map<ObjectId, std::vector<ObjectReference>> m_References;
    ObjectId m_NextId = 1;
};

struct SERIALIZATION_API SerializationDocument {
    DocumentHeader header{};
    std::vector<std::string> stringTable;
    std::vector<ObjectTableEntry> objectTable;
    std::vector<ObjectReference> referenceTable;
    std::vector<std::uint8_t> payload;
    std::vector<std::uint8_t> rawBytes; // full document bytes when loaded/saved
};

/// Primary serialization service — Reflection-driven by default.
class SERIALIZATION_API ISerializer {
public:
    virtual ~ISerializer() = default;

    /// Serialize a single reflected instance to bytes (document with one object).
    [[nodiscard]] virtual std::vector<std::uint8_t> SerializeObject(
        reflection::TypeId typeId,
        const void* instance,
        const SerializeOptions& options = {}) = 0;

    [[nodiscard]] virtual bool DeserializeObject(
        reflection::TypeId typeId,
        void* instance,
        const std::uint8_t* bytes,
        std::size_t byteCount,
        const SerializeOptions& options = {}) = 0;

    /// Multi-object graph serialize / deserialize with circular reference handling.
    [[nodiscard]] virtual std::vector<std::uint8_t> SerializeGraph(
        const ObjectGraph& graph,
        const SerializeOptions& options = {}) = 0;

    [[nodiscard]] virtual bool DeserializeGraph(
        ObjectGraph& outGraph,
        const std::uint8_t* bytes,
        std::size_t byteCount,
        const SerializeOptions& options = {}) = 0;

    [[nodiscard]] virtual bool SerializeDocument(
        SerializationDocument& ioDocument,
        const ObjectGraph& graph,
        const SerializeOptions& options = {}) = 0;

    [[nodiscard]] virtual bool DeserializeDocument(
        SerializationDocument& outDocument,
        const std::uint8_t* bytes,
        std::size_t byteCount) = 0;
};

struct SERIALIZATION_API SerializerDependencies {
    reflection::ITypeRegistry* registry = nullptr; // required; not owned
};

[[nodiscard]] SERIALIZATION_API std::unique_ptr<ISerializer> CreateSerializer(
    SerializerDependencies deps);

} // namespace we::runtime::serialization
