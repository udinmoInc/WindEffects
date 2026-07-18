#include "Serialization/ISerializer.h"
#include "Serialization/IArchive.h"
#include "Serialization/Diagnostics.h"
#include "Serialization/Transforms.h"
#include "HashUtils.h"

#include "Reflection/IBinaryArchive.h"
#include "Reflection/IObjectFactory.h"
#include "Reflection/ReflectionMigration.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string_view>
#include <unordered_map>

namespace we::runtime::serialization {
namespace {

std::mutex& CustomMutex() {
    static std::mutex mutex;
    return mutex;
}

std::unordered_map<reflection::TypeId, CustomSerializerEntry, reflection::TypeIdHash>& CustomMap() {
    static std::unordered_map<reflection::TypeId, CustomSerializerEntry, reflection::TypeIdHash> map;
    return map;
}

SerializationTimingStats& Timing() {
    static SerializationTimingStats stats;
    return stats;
}

bool WriteReference(BinaryWriter& writer, const ObjectReference& ref) {
    const std::uint8_t kind = static_cast<std::uint8_t>(ref.kind);
    return writer.WriteBytes(&kind, 1) && writer.WriteObjectId(ref.objectId)
        && writer.WriteTypeId(ref.typeId) && writer.WriteGuid(ref.guid)
        && writer.WriteString(ref.softPath);
}

bool ReadReference(BinaryReader& reader, ObjectReference& ref) {
    std::uint8_t kind = 0;
    if (!reader.ReadBytes(&kind, 1)) {
        return false;
    }
    ref.kind = static_cast<ReferenceKind>(kind);
    return reader.ReadObjectId(ref.objectId) && reader.ReadTypeId(ref.typeId)
        && reader.ReadGuid(ref.guid) && reader.ReadString(ref.softPath);
}

std::vector<std::uint8_t> SerializePayload(
    reflection::ITypeRegistry& registry,
    reflection::TypeId typeId,
    void* instance,
    const SerializeOptions& options)
{
    if (const CustomSerializerEntry* custom = FindCustomSerializer(typeId)) {
        MemoryArchive archive(MemoryArchive::Mode::Saving);
        if (!custom->serialize(archive, instance, typeId, custom->userData)) {
            return {};
        }
        return archive.TakeBytes();
    }

    reflection::BinarySerializeOptions reflOptions;
    reflOptions.writeTypeIdHeader = options.writeTypeHeaders;
    reflOptions.skipTransient = options.skipTransient;
    reflOptions.includeBases = options.includeBases;
    reflOptions.requireSerializeFlag = true;
    return reflection::SerializeObjectToBytes(registry, typeId, instance, reflOptions);
}

bool DeserializePayload(
    reflection::ITypeRegistry& registry,
    reflection::TypeId typeId,
    void* instance,
    const std::uint8_t* bytes,
    std::size_t byteCount,
    const SerializeOptions& options)
{
    if (const CustomSerializerEntry* custom = FindCustomSerializer(typeId)) {
        MemoryArchive archive(bytes, byteCount);
        return custom->serialize(archive, instance, typeId, custom->userData);
    }

    reflection::BinarySerializeOptions reflOptions;
    reflOptions.writeTypeIdHeader = options.writeTypeHeaders;
    reflOptions.skipTransient = options.skipTransient;
    reflOptions.includeBases = options.includeBases;
    reflOptions.requireSerializeFlag = true;
    return reflection::DeserializeObjectFromBytes(
        registry, typeId, instance, bytes, byteCount, reflOptions);
}

class Serializer final : public ISerializer {
public:
    explicit Serializer(reflection::ITypeRegistry* registry) : m_Registry(registry) {}

    std::vector<std::uint8_t> SerializeObject(
        reflection::TypeId typeId,
        const void* instance,
        const SerializeOptions& options) override
    {
        if (!m_Registry || !instance) {
            return {};
        }
        ObjectGraph graph;
        (void)graph.AddObject(typeId, const_cast<void*>(instance), false);
        return SerializeGraph(graph, options);
    }

    bool DeserializeObject(
        reflection::TypeId typeId,
        void* instance,
        const std::uint8_t* bytes,
        std::size_t byteCount,
        const SerializeOptions& options) override
    {
        if (!m_Registry || !instance || !bytes || byteCount == 0) {
            return false;
        }
        SerializationDocument document;
        if (!DeserializeDocument(document, bytes, byteCount)) {
            return false;
        }
        if (document.objectTable.empty()) {
            return false;
        }
        const ObjectTableEntry& entry = document.objectTable.front();
        reflection::TypeId loadType = reflection::RemapTypeId(entry.typeId);
        if (loadType != typeId && typeId != reflection::kInvalidTypeId) {
            // Allow exact requested type only.
            if (loadType != typeId) {
                return false;
            }
        }
        if (entry.payloadOffset + entry.payloadSize > document.payload.size()) {
            return false;
        }
        const std::uint8_t* payload = document.payload.data()
            + static_cast<std::size_t>(entry.payloadOffset);
        if (!DeserializePayload(
                *m_Registry,
                loadType,
                instance,
                payload,
                static_cast<std::size_t>(entry.payloadSize),
                options)) {
            return false;
        }
        if (options.migrateOnVersionMismatch) {
            if (const reflection::TypeInfo* info = m_Registry->Find(loadType)) {
                if (entry.schemaVersion != info->versions.schemaVersion) {
                    if (!reflection::MigrateInstance(
                            loadType,
                            instance,
                            entry.schemaVersion,
                            info->versions.schemaVersion)) {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    std::vector<std::uint8_t> SerializeGraph(
        const ObjectGraph& graph,
        const SerializeOptions& options) override
    {
        SerializationDocument document;
        if (!SerializeDocument(document, graph, options)) {
            return {};
        }
        return document.rawBytes;
    }

    bool DeserializeGraph(
        ObjectGraph& outGraph,
        const std::uint8_t* bytes,
        std::size_t byteCount,
        const SerializeOptions& options) override
    {
        if (!m_Registry) {
            return false;
        }
        const auto start = std::chrono::steady_clock::now();

        SerializationDocument document;
        if (!DeserializeDocument(document, bytes, byteCount)) {
            return false;
        }

        outGraph.Clear(m_Registry);
        reflection::ObjectFactoryDependencies factoryDeps;
        factoryDeps.registry = m_Registry;
        auto factory = reflection::CreateObjectFactory(factoryDeps);

        // First pass: allocate / construct all objects.
        for (const ObjectTableEntry& entry : document.objectTable) {
            const reflection::TypeId typeId = reflection::RemapTypeId(entry.typeId);
            if (!m_Registry->Contains(typeId)) {
                if (!options.allowMissingTypesOnLoad) {
                    return false;
                }
                continue;
            }
            void* instance = factory->Create(typeId);
            if (!instance) {
                return false;
            }
            const ObjectId added =
                outGraph.AddObjectWithId(entry.objectId, typeId, instance, true);
            if (added == kInvalidObjectId) {
                factory->Destroy(typeId, instance);
                return false;
            }
        }

        // Second pass: deserialize payloads.
        for (const ObjectTableEntry& entry : document.objectTable) {
            GraphObject* obj = outGraph.Find(entry.objectId);
            if (!obj || !obj->instance) {
                continue;
            }
            if (entry.payloadOffset + entry.payloadSize > document.payload.size()) {
                return false;
            }
            const std::uint8_t* payload = document.payload.data()
                + static_cast<std::size_t>(entry.payloadOffset);
            if (!DeserializePayload(
                    *m_Registry,
                    obj->typeId,
                    obj->instance,
                    payload,
                    static_cast<std::size_t>(entry.payloadSize),
                    options)) {
                return false;
            }
            if (options.migrateOnVersionMismatch) {
                if (const reflection::TypeInfo* info = m_Registry->Find(obj->typeId)) {
                    if (entry.schemaVersion != info->versions.schemaVersion) {
                        if (!reflection::MigrateInstance(
                                obj->typeId,
                                obj->instance,
                                entry.schemaVersion,
                                info->versions.schemaVersion)) {
                            return false;
                        }
                    }
                }
            }
        }

        // Third pass: restore reference edges (from-id encoded in softPath prefix).
        for (ObjectReference ref : document.referenceTable) {
            ObjectId fromId = kInvalidObjectId;
            constexpr std::string_view kPrefix = "__from:";
            if (ref.softPath.rfind(kPrefix.data(), 0) == 0) {
                const std::string rest = ref.softPath.substr(kPrefix.size());
                const std::size_t bar = rest.find('|');
                const std::string idPart = bar == std::string::npos ? rest : rest.substr(0, bar);
                fromId = static_cast<ObjectId>(std::strtoull(idPart.c_str(), nullptr, 10));
                ref.softPath = bar == std::string::npos ? std::string{} : rest.substr(bar + 1);
            }
            if (fromId != kInvalidObjectId && outGraph.Find(fromId)) {
                outGraph.AddReference(fromId, std::move(ref));
            }
        }

        const auto end = std::chrono::steady_clock::now();
        const double ms = std::chrono::duration<double, std::milli>(end - start).count();
        Timing().lastDeserializeMs = ms;
        Timing().totalDeserializeMs += ms;
        ++Timing().deserializeCount;
        return true;
    }

    bool SerializeDocument(
        SerializationDocument& ioDocument,
        const ObjectGraph& graph,
        const SerializeOptions& options) override
    {
        if (!m_Registry) {
            return false;
        }
        const auto start = std::chrono::steady_clock::now();

        ioDocument = SerializationDocument{};
        std::vector<GraphObject> ordered = graph.Objects();
        if (options.deterministic) {
            std::sort(ordered.begin(), ordered.end(), [](const GraphObject& a, const GraphObject& b) {
                return a.objectId < b.objectId;
            });
        }

        BinaryWriter payloadWriter;
        for (const GraphObject& obj : ordered) {
            if (!obj.instance || obj.typeId == reflection::kInvalidTypeId) {
                return false;
            }
            auto payload = SerializePayload(*m_Registry, obj.typeId, obj.instance, options);
            if (payload.empty() && m_Registry->Find(obj.typeId)
                && !m_Registry->Find(obj.typeId)->properties.empty()) {
                // Empty payload only valid for empty types.
            }

            ObjectTableEntry entry;
            entry.objectId = obj.objectId;
            entry.typeId = obj.typeId;
            if (const reflection::TypeInfo* info = m_Registry->Find(obj.typeId)) {
                entry.schemaVersion = info->versions.schemaVersion;
            }
            entry.payloadOffset = payloadWriter.Size();
            entry.payloadSize = payload.size();
            if (!payload.empty() && !payloadWriter.WriteBytes(payload.data(), payload.size())) {
                return false;
            }
            ioDocument.objectTable.push_back(entry);
        }
        ioDocument.payload = payloadWriter.TakeBytes();

        // Deterministic reference export.
        std::vector<std::pair<ObjectId, ObjectReference>> edges;
        for (const auto& [from, refs] : graph.AllReferences()) {
            for (const ObjectReference& ref : refs) {
                edges.emplace_back(from, ref);
            }
        }
        if (options.deterministic) {
            std::sort(edges.begin(), edges.end(), [](const auto& a, const auto& b) {
                if (a.first != b.first) {
                    return a.first < b.first;
                }
                if (a.second.objectId != b.second.objectId) {
                    return a.second.objectId < b.second.objectId;
                }
                return static_cast<std::uint8_t>(a.second.kind) < static_cast<std::uint8_t>(b.second.kind);
            });
        }
        for (const auto& [from, ref] : edges) {
            ObjectReference packed = ref;
            // Encode from-id into reserved path prefix for round-trip without format change.
            packed.softPath = std::string("__from:") + std::to_string(from)
                + (ref.softPath.empty() ? std::string{} : (std::string("|") + ref.softPath));
            ioDocument.referenceTable.push_back(std::move(packed));
        }

        DocumentFlags flags = DocumentFlags::HasObjectTable | DocumentFlags::HasChecksum
            | DocumentFlags::HasReferenceTable;
        if (options.mode == SerializeMode::Delta) {
            flags = flags | DocumentFlags::DeltaDocument;
        } else if (options.mode == SerializeMode::Snapshot) {
            flags = flags | DocumentFlags::SnapshotDocument;
        } else if (options.mode == SerializeMode::Partial) {
            flags = flags | DocumentFlags::PartialDocument;
        }

        BinaryWriter body;
        // String table (reserved / empty for v1)
        body.WriteUInt64(0);
        const std::uint64_t objectTableOffset = sizeof(DocumentHeader) + body.Size();
        body.WriteUInt64(ioDocument.objectTable.size());
        for (const ObjectTableEntry& entry : ioDocument.objectTable) {
            body.WriteObjectId(entry.objectId);
            body.WriteTypeId(entry.typeId);
            body.WriteUInt32(entry.schemaVersion);
            body.WriteUInt32(entry.flags);
            body.WriteUInt64(entry.payloadOffset);
            body.WriteUInt64(entry.payloadSize);
        }
        const std::uint64_t referenceTableOffset = sizeof(DocumentHeader) + body.Size();
        body.WriteUInt64(ioDocument.referenceTable.size());
        for (const ObjectReference& ref : ioDocument.referenceTable) {
            if (!WriteReference(body, ref)) {
                return false;
            }
        }
        const std::uint64_t payloadOffset = sizeof(DocumentHeader) + body.Size();
        if (!ioDocument.payload.empty()) {
            body.WriteBytes(ioDocument.payload.data(), ioDocument.payload.size());
        }

        std::vector<std::uint8_t> bodyBytes = body.TakeBytes();

        // Optional compression / encryption of body.
        std::vector<std::uint8_t> transformed = bodyBytes;
        if (!CompressBytes(bodyBytes.data(), bodyBytes.size(), transformed)) {
            return false;
        }
        if (transformed.size() != bodyBytes.size()
            || std::memcmp(transformed.data(), bodyBytes.data(), bodyBytes.size()) != 0) {
            flags = flags | DocumentFlags::Compressed;
            bodyBytes = std::move(transformed);
        }
        transformed.clear();
        if (!EncryptBytes(bodyBytes.data(), bodyBytes.size(), transformed)) {
            return false;
        }
        if (transformed.size() != bodyBytes.size()
            || std::memcmp(transformed.data(), bodyBytes.data(), bodyBytes.size()) != 0) {
            flags = flags | DocumentFlags::Encrypted;
            bodyBytes = std::move(transformed);
        }

        DocumentHeader header{};
        header.formatVersion = kSerializationFormatVersion;
        header.abiVersion = kSerializationAbiVersion;
        header.flags = static_cast<std::uint32_t>(flags);
        header.objectCount = ioDocument.objectTable.size();
        header.stringTableOffset = sizeof(DocumentHeader);
        header.objectTableOffset = objectTableOffset;
        header.referenceTableOffset = referenceTableOffset;
        header.payloadOffset = payloadOffset;
        header.payloadSize = ioDocument.payload.size();
        header.contentChecksum = options.computeChecksum
            ? ComputeChecksum(bodyBytes.data(), bodyBytes.size())
            : 0;

        BinaryWriter documentWriter;
        documentWriter.WriteHeader(header);
        documentWriter.WriteBytes(bodyBytes.data(), bodyBytes.size());
        ioDocument.header = header;
        ioDocument.rawBytes = documentWriter.TakeBytes();

        const auto end = std::chrono::steady_clock::now();
        const double ms = std::chrono::duration<double, std::milli>(end - start).count();
        Timing().lastSerializeMs = ms;
        Timing().totalSerializeMs += ms;
        ++Timing().serializeCount;
        return documentWriter.Good();
    }

    bool DeserializeDocument(
        SerializationDocument& outDocument,
        const std::uint8_t* bytes,
        std::size_t byteCount) override
    {
        outDocument = SerializationDocument{};
        if (!bytes || byteCount < sizeof(DocumentHeader)) {
            return false;
        }

        std::vector<ValidationIssue> issues;
        ValidateDocumentBytes(bytes, byteCount, issues, true);
        if (CountValidationIssues(issues, ValidationSeverity::Error) > 0) {
            return false;
        }

        BinaryReader reader(bytes, byteCount);
        if (!reader.ReadHeader(outDocument.header)) {
            return false;
        }

        const std::size_t bodySize = byteCount - sizeof(DocumentHeader);
        std::vector<std::uint8_t> body(bytes + sizeof(DocumentHeader), bytes + byteCount);

        if (HasFlag(static_cast<DocumentFlags>(outDocument.header.flags), DocumentFlags::Encrypted)) {
            std::vector<std::uint8_t> decrypted;
            if (!DecryptBytes(body.data(), body.size(), decrypted)) {
                return false;
            }
            body = std::move(decrypted);
        }
        if (HasFlag(static_cast<DocumentFlags>(outDocument.header.flags), DocumentFlags::Compressed)) {
            // Expected uncompressed size unknown — decompress backends that need it
            // should store size in their own framing. Identity backend copies.
            std::vector<std::uint8_t> decompressed;
            if (!DecompressBytes(body.data(), body.size(), body.size(), decompressed)) {
                return false;
            }
            body = std::move(decompressed);
        }

        BinaryReader bodyReader(body.data(), body.size());
        std::uint64_t stringCount = 0;
        if (!bodyReader.ReadUInt64(stringCount)) {
            return false;
        }
        for (std::uint64_t i = 0; i < stringCount; ++i) {
            std::string s;
            if (!bodyReader.ReadString(s)) {
                return false;
            }
            outDocument.stringTable.push_back(std::move(s));
        }

        std::uint64_t objectCount = 0;
        if (!bodyReader.ReadUInt64(objectCount)) {
            return false;
        }
        outDocument.objectTable.reserve(static_cast<std::size_t>(objectCount));
        for (std::uint64_t i = 0; i < objectCount; ++i) {
            ObjectTableEntry entry;
            if (!bodyReader.ReadObjectId(entry.objectId) || !bodyReader.ReadTypeId(entry.typeId)
                || !bodyReader.ReadUInt32(entry.schemaVersion)
                || !bodyReader.ReadUInt32(entry.flags)
                || !bodyReader.ReadUInt64(entry.payloadOffset)
                || !bodyReader.ReadUInt64(entry.payloadSize)) {
                return false;
            }
            outDocument.objectTable.push_back(entry);
        }

        std::uint64_t refCount = 0;
        if (!bodyReader.ReadUInt64(refCount)) {
            return false;
        }
        outDocument.referenceTable.reserve(static_cast<std::size_t>(refCount));
        for (std::uint64_t i = 0; i < refCount; ++i) {
            ObjectReference ref;
            if (!ReadReference(bodyReader, ref)) {
                return false;
            }
            outDocument.referenceTable.push_back(std::move(ref));
        }

        const std::size_t remaining = bodyReader.Remaining();
        outDocument.payload.assign(body.data() + bodyReader.Tell(), body.data() + body.size());
        (void)remaining;
        outDocument.rawBytes.assign(bytes, bytes + byteCount);
        return true;
    }

private:
    reflection::ITypeRegistry* m_Registry = nullptr;
};

} // namespace

void RegisterCustomSerializer(CustomSerializerEntry entry) {
    if (entry.typeId == reflection::kInvalidTypeId || !entry.serialize) {
        return;
    }
    std::lock_guard lock(CustomMutex());
    CustomMap()[entry.typeId] = entry;
}

void UnregisterCustomSerializer(reflection::TypeId typeId) {
    std::lock_guard lock(CustomMutex());
    CustomMap().erase(typeId);
}

void ClearCustomSerializers() {
    std::lock_guard lock(CustomMutex());
    CustomMap().clear();
}

const CustomSerializerEntry* FindCustomSerializer(reflection::TypeId typeId) {
    std::lock_guard lock(CustomMutex());
    const auto it = CustomMap().find(typeId);
    return it == CustomMap().end() ? nullptr : &it->second;
}

ObjectGraph::~ObjectGraph() {
    Clear(nullptr);
}

ObjectGraph::ObjectGraph(ObjectGraph&& other) noexcept {
    *this = std::move(other);
}

ObjectGraph& ObjectGraph::operator=(ObjectGraph&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    Clear(nullptr);
    m_Objects = std::move(other.m_Objects);
    m_Index = std::move(other.m_Index);
    m_References = std::move(other.m_References);
    m_NextId = other.m_NextId;
    other.m_NextId = 1;
    return *this;
}

ObjectId ObjectGraph::AddObject(reflection::TypeId typeId, void* instance, bool takeOwnership) {
    return AddObjectWithId(m_NextId++, typeId, instance, takeOwnership);
}

ObjectId ObjectGraph::AddObjectWithId(
    ObjectId objectId,
    reflection::TypeId typeId,
    void* instance,
    bool takeOwnership)
{
    if (objectId == kInvalidObjectId || !instance) {
        return kInvalidObjectId;
    }
    if (m_Index.count(objectId) != 0) {
        return kInvalidObjectId;
    }
    GraphObject obj;
    obj.objectId = objectId;
    obj.typeId = typeId;
    obj.instance = instance;
    obj.owned = takeOwnership;
    m_Index[objectId] = m_Objects.size();
    m_Objects.push_back(obj);
    if (objectId >= m_NextId) {
        m_NextId = objectId + 1;
    }
    return objectId;
}

GraphObject* ObjectGraph::Find(ObjectId objectId) {
    const auto it = m_Index.find(objectId);
    if (it == m_Index.end()) {
        return nullptr;
    }
    return &m_Objects[it->second];
}

const GraphObject* ObjectGraph::Find(ObjectId objectId) const {
    const auto it = m_Index.find(objectId);
    if (it == m_Index.end()) {
        return nullptr;
    }
    return &m_Objects[it->second];
}

void ObjectGraph::AddReference(ObjectId from, ObjectReference reference) {
    m_References[from].push_back(std::move(reference));
}

const std::vector<ObjectReference>& ObjectGraph::ReferencesFrom(ObjectId from) const {
    static const std::vector<ObjectReference> kEmpty;
    const auto it = m_References.find(from);
    return it == m_References.end() ? kEmpty : it->second;
}

void ObjectGraph::Clear(const reflection::ITypeRegistry* registryForDestroy) {
    if (registryForDestroy) {
        reflection::ObjectFactoryDependencies deps;
        deps.registry = const_cast<reflection::ITypeRegistry*>(registryForDestroy);
        auto factory = reflection::CreateObjectFactory(deps);
        for (GraphObject& obj : m_Objects) {
            if (obj.owned && obj.instance) {
                factory->Destroy(obj.typeId, obj.instance);
                obj.instance = nullptr;
            }
        }
    }
    m_Objects.clear();
    m_Index.clear();
    m_References.clear();
    m_NextId = 1;
}

std::unique_ptr<ISerializer> CreateSerializer(SerializerDependencies deps) {
    return std::make_unique<Serializer>(deps.registry);
}

} // namespace we::runtime::serialization
