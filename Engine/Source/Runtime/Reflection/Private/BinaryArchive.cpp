#include "Reflection/IBinaryArchive.h"
#include "Reflection/ITypeRegistry.h"
#include "Reflection/ReflectionMigration.h"
#include "Reflection/SerializePlan.h"

#include <cstddef>
#include <cstring>

namespace we::runtime::reflection {
namespace {

bool SerializePrimitiveValue(
    IBinaryArchive& archive,
    PrimitiveKind kind,
    void* value,
    std::uint32_t size)
{
    if (!value) {
        return false;
    }
    switch (kind) {
    case PrimitiveKind::Bool:
        return archive.SerializeBool(*static_cast<bool*>(value));
    case PrimitiveKind::Int8:
        return archive.SerializeInt8(*static_cast<std::int8_t*>(value));
    case PrimitiveKind::UInt8:
        return archive.SerializeUInt8(*static_cast<std::uint8_t*>(value));
    case PrimitiveKind::Int16:
        return archive.SerializeInt16(*static_cast<std::int16_t*>(value));
    case PrimitiveKind::UInt16:
        return archive.SerializeUInt16(*static_cast<std::uint16_t*>(value));
    case PrimitiveKind::Int32:
        return archive.SerializeInt32(*static_cast<std::int32_t*>(value));
    case PrimitiveKind::UInt32:
        return archive.SerializeUInt32(*static_cast<std::uint32_t*>(value));
    case PrimitiveKind::Int64:
        return archive.SerializeInt64(*static_cast<std::int64_t*>(value));
    case PrimitiveKind::UInt64:
        return archive.SerializeUInt64(*static_cast<std::uint64_t*>(value));
    case PrimitiveKind::Float:
        return archive.SerializeFloat(*static_cast<float*>(value));
    case PrimitiveKind::Double:
        return archive.SerializeDouble(*static_cast<double*>(value));
    case PrimitiveKind::Char:
        return archive.SerializeBytes(value, sizeof(char));
    case PrimitiveKind::String:
        return archive.SerializeString(*static_cast<std::string*>(value));
    case PrimitiveKind::Vec2:
    case PrimitiveKind::Vec3:
    case PrimitiveKind::Vec4:
    case PrimitiveKind::Quat:
    case PrimitiveKind::Mat4:
    case PrimitiveKind::Guid:
    case PrimitiveKind::Opaque:
        return archive.SerializeBytes(value, size);
    case PrimitiveKind::None:
    case PrimitiveKind::Void:
    case PrimitiveKind::StringView:
    default:
        if (size == 0) {
            return false;
        }
        return archive.SerializeBytes(value, size);
    }
}

bool SerializeStepValue(
    IBinaryArchive& archive,
    const ITypeRegistry& registry,
    const SerializeStep& step,
    void* instance)
{
    if (!step.property) {
        return false;
    }
    const PropertyInfo& property = *step.property;
    void* field = nullptr;
    if (property.getter || property.setter) {
        // Accessor path — use temporary buffer for non-direct storage.
        if (step.size == 0) {
            return false;
        }
    }
    field = property.MutablePtr(instance);
    if (!field && step.size > 0) {
        return false;
    }

    if (step.isString) {
        return archive.SerializeString(*static_cast<std::string*>(field));
    }
    if (step.primitive != PrimitiveKind::None) {
        return SerializePrimitiveValue(archive, step.primitive, field, step.size);
    }
    if (step.isEnum) {
        return archive.SerializeBytes(field, step.size);
    }
    if (step.isNestedObject && field) {
        BinarySerializeOptions nested;
        nested.writeTypeIdHeader = false;
        nested.includeBases = true;
        return SerializeObjectBinary(archive, registry, step.propertyTypeId, field, nested);
    }
    if (step.size > 0 && field) {
        return archive.SerializeBytes(field, step.size);
    }
    return false;
}

} // namespace

MemoryBinaryArchive::MemoryBinaryArchive(Mode mode)
    : m_Mode(mode)
{
}

MemoryBinaryArchive::MemoryBinaryArchive(std::vector<std::uint8_t> existingBytes)
    : m_Mode(Mode::Loading)
    , m_Bytes(std::move(existingBytes))
{
}

MemoryBinaryArchive::MemoryBinaryArchive(const std::uint8_t* bytes, std::size_t size)
    : m_Mode(Mode::Loading)
{
    if (bytes && size > 0) {
        m_Bytes.assign(bytes, bytes + size);
    }
}

template <typename T>
bool MemoryBinaryArchive::SerializePod(T& value) {
    return SerializeBytes(&value, sizeof(T));
}

bool MemoryBinaryArchive::SerializeBytes(void* data, std::size_t size) {
    if (!m_Good || !data || size == 0) {
        if (size == 0) {
            return m_Good;
        }
        m_Good = false;
        return false;
    }
    if (m_Mode == Mode::Saving) {
        const auto* bytes = static_cast<const std::uint8_t*>(data);
        m_Bytes.insert(m_Bytes.end(), bytes, bytes + size);
        m_Offset = m_Bytes.size();
        return true;
    }
    if (m_Offset + size > m_Bytes.size()) {
        m_Good = false;
        return false;
    }
    std::memcpy(data, m_Bytes.data() + m_Offset, size);
    m_Offset += size;
    return true;
}

bool MemoryBinaryArchive::SerializeBool(bool& value) {
    std::uint8_t byte = value ? 1u : 0u;
    if (!SerializeUInt8(byte)) {
        return false;
    }
    if (IsLoading()) {
        value = byte != 0;
    }
    return true;
}

bool MemoryBinaryArchive::SerializeInt8(std::int8_t& value) { return SerializePod(value); }
bool MemoryBinaryArchive::SerializeUInt8(std::uint8_t& value) { return SerializePod(value); }
bool MemoryBinaryArchive::SerializeInt16(std::int16_t& value) { return SerializePod(value); }
bool MemoryBinaryArchive::SerializeUInt16(std::uint16_t& value) { return SerializePod(value); }
bool MemoryBinaryArchive::SerializeInt32(std::int32_t& value) { return SerializePod(value); }
bool MemoryBinaryArchive::SerializeUInt32(std::uint32_t& value) { return SerializePod(value); }
bool MemoryBinaryArchive::SerializeInt64(std::int64_t& value) { return SerializePod(value); }
bool MemoryBinaryArchive::SerializeUInt64(std::uint64_t& value) { return SerializePod(value); }
bool MemoryBinaryArchive::SerializeFloat(float& value) { return SerializePod(value); }
bool MemoryBinaryArchive::SerializeDouble(double& value) { return SerializePod(value); }

bool MemoryBinaryArchive::SerializeString(std::string& value) {
    if (IsSaving()) {
        if (value.size() > 0xFFFFFFFFull) {
            m_Good = false;
            return false;
        }
        std::uint32_t length = static_cast<std::uint32_t>(value.size());
        if (!SerializeUInt32(length)) {
            return false;
        }
        if (length == 0) {
            return true;
        }
        return SerializeBytes(value.data(), length);
    }

    std::uint32_t length = 0;
    if (!SerializeUInt32(length)) {
        return false;
    }
    value.resize(length);
    if (length == 0) {
        return true;
    }
    return SerializeBytes(value.data(), length);
}

bool MemoryBinaryArchive::SerializeTypeId(TypeId& value) {
    return SerializeUInt64(value);
}

bool SerializeObjectBinary(
    IBinaryArchive& archive,
    const ITypeRegistry& registry,
    TypeId typeId,
    void* instance,
    const BinarySerializeOptions& options)
{
    if (!instance) {
        return false;
    }
    const TypeInfo* info = registry.Resolve(typeId);
    if (!info) {
        return false;
    }

    std::uint32_t archivedSchemaVersion = info->versions.schemaVersion;
    if (options.writeTypeIdHeader) {
        TypeId headerType = info->typeId;
        std::uint32_t schemaVersion = info->versions.schemaVersion;
        if (!archive.SerializeTypeId(headerType)) {
            return false;
        }
        if (!archive.SerializeUInt32(schemaVersion)) {
            return false;
        }
        if (archive.IsLoading()) {
            headerType = RemapTypeId(headerType);
            if (headerType != info->typeId) {
                return false;
            }
            archivedSchemaVersion = schemaVersion;
            if (archivedSchemaVersion != info->versions.schemaVersion) {
                NotifySchemaMismatch(info->typeId, archivedSchemaVersion, info->versions.schemaVersion);
            }
        }
    }

    // Prefer cached serialize plan — zero discovery in the hot path.
    bool ok = false;
    if (options.requireSerializeFlag && options.skipTransient && options.includeBases) {
        if (const SerializePlan* plan = registry.GetSerializePlan(typeId)) {
            for (const SerializeStep& step : plan->steps) {
                if (!SerializeStepValue(archive, registry, step, instance)) {
                    return false;
                }
            }
            ok = true;
        }
    }

    if (!ok) {
        // Fallback for uncommon option combinations.
        const TypeInfo* resolved = info;
        for (const PropertyInfo& property : resolved->properties) {
            if (options.requireSerializeFlag && !property.IsSerialized()) {
                continue;
            }
            if (options.skipTransient && HasFlag(property.flags, PropertyFlags::Transient)) {
                continue;
            }
            SerializeStep step;
            step.property = &property;
            step.propertyTypeId = property.typeId;
            step.primitive = property.primitive;
            step.offset = property.offset;
            step.size = property.size;
            step.schemaTag = property.schemaTag;
            step.isString = property.primitive == PrimitiveKind::String;
            if (!SerializeStepValue(archive, registry, step, instance)) {
                return false;
            }
        }
        ok = true;
    }

    if (archive.IsLoading() && archivedSchemaVersion != info->versions.schemaVersion) {
        if (!MigrateInstance(
                info->typeId, instance, archivedSchemaVersion, info->versions.schemaVersion)) {
            return false;
        }
    }
    return ok;
}

std::vector<std::uint8_t> SerializeObjectToBytes(
    const ITypeRegistry& registry,
    TypeId typeId,
    const void* instance,
    const BinarySerializeOptions& options)
{
    MemoryBinaryArchive archive(MemoryBinaryArchive::Mode::Saving);
    if (!SerializeObjectBinary(archive, registry, typeId, const_cast<void*>(instance), options)) {
        return {};
    }
    return archive.TakeBytes();
}

bool DeserializeObjectFromBytes(
    const ITypeRegistry& registry,
    TypeId typeId,
    void* instance,
    const std::uint8_t* bytes,
    std::size_t byteCount,
    const BinarySerializeOptions& options)
{
    if (!bytes || byteCount == 0) {
        return false;
    }
    MemoryBinaryArchive archive(bytes, byteCount);
    return SerializeObjectBinary(archive, registry, typeId, instance, options);
}

} // namespace we::runtime::reflection
