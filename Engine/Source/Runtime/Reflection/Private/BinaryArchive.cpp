#include "Reflection/IBinaryArchive.h"
#include "Reflection/ITypeRegistry.h"
#include "Reflection/PropertyIterator.h"
#include "Reflection/BuiltinTypes.h"

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

bool SerializePropertyValue(
    IBinaryArchive& archive,
    const ITypeRegistry& registry,
    const PropertyInfo& property,
    void* instance)
{
    void* field = property.MutablePtr(instance);
    if (!field && property.size > 0) {
        return false;
    }

    if (property.primitive != PrimitiveKind::None) {
        return SerializePrimitiveValue(archive, property.primitive, field, property.size);
    }

    const TypeInfo* propertyType = registry.Resolve(property.typeId);
    if (propertyType && propertyType->IsPrimitive()) {
        return SerializePrimitiveValue(archive, propertyType->primitive, field, property.size);
    }

    if (propertyType && propertyType->IsEnum()) {
        // Enums serialize as their underlying integer width.
        return archive.SerializeBytes(field, property.size);
    }

    // Nested reflected object: recurse using property's type.
    if (propertyType && propertyType->IsClassOrStruct() && field) {
        BinarySerializeOptions nestedOptions;
        nestedOptions.writeTypeIdHeader = false;
        nestedOptions.includeBases = true;
        return SerializeObjectBinary(archive, registry, property.typeId, field, nestedOptions);
    }

    if (property.size > 0 && field) {
        return archive.SerializeBytes(field, property.size);
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

    if (options.writeTypeIdHeader) {
        TypeId headerType = info->typeId;
        std::uint32_t schemaVersion = info->schemaVersion;
        if (!archive.SerializeTypeId(headerType)) {
            return false;
        }
        if (!archive.SerializeUInt32(schemaVersion)) {
            return false;
        }
        if (archive.IsLoading() && headerType != info->typeId) {
            // Strict match for now; future adapters may remap schema versions.
            return false;
        }
    }

    PropertyIterateOptions iterateOptions;
    iterateOptions.includeBases = options.includeBases;
    iterateOptions.serializedOnly = options.requireSerializeFlag;
    iterateOptions.skipTransient = options.skipTransient;
    iterateOptions.skipHidden = false;

    const std::vector<PropertyVisit> properties = CollectProperties(registry, typeId, iterateOptions);
    for (const PropertyVisit& visit : properties) {
        if (!visit.property) {
            return false;
        }
        if (!SerializePropertyValue(archive, registry, *visit.property, instance)) {
            return false;
        }
    }
    return true;
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
