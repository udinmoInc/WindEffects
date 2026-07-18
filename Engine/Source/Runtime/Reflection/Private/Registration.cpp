#include "Reflection/Registration.h"

#include <utility>

namespace we::runtime::reflection {

TypeBuilder::TypeBuilder(std::string_view qualifiedName) {
    QualifiedName(qualifiedName);
}

TypeBuilder& TypeBuilder::QualifiedName(std::string_view name) {
    m_Info.qualifiedName = std::string(name);
    m_Info.typeId = MakeTypeId(m_Info.qualifiedName);
    if (m_Info.name.empty()) {
        const std::size_t pos = name.find_last_of(':');
        m_Info.name = std::string(pos == std::string_view::npos ? name : name.substr(pos + 1));
    }
    return *this;
}

TypeBuilder& TypeBuilder::Name(std::string_view shortName) {
    m_Info.name = std::string(shortName);
    return *this;
}

TypeBuilder& TypeBuilder::Kind(TypeKind kind) {
    m_Info.kind = kind;
    return *this;
}

TypeBuilder& TypeBuilder::Primitive(PrimitiveKind kind) {
    m_Info.primitive = kind;
    m_Info.kind = TypeKind::Primitive;
    return *this;
}

TypeBuilder& TypeBuilder::Size(std::uint32_t size) {
    m_Info.size = size;
    return *this;
}

TypeBuilder& TypeBuilder::Alignment(std::uint32_t alignment) {
    m_Info.alignment = alignment == 0 ? 1u : alignment;
    return *this;
}

TypeBuilder& TypeBuilder::SchemaVersion(std::uint32_t version) {
    const std::uint32_t v = version == 0 ? 1u : version;
    m_Info.schemaVersion = v;
    m_Info.versions.schemaVersion = v;
    return *this;
}

TypeBuilder& TypeBuilder::TypeVersion(std::uint32_t version) {
    m_Info.versions.typeVersion = version == 0 ? 1u : version;
    return *this;
}

TypeBuilder& TypeBuilder::MigrationVersion(std::uint32_t version) {
    m_Info.versions.migrationVersion = version == 0 ? 1u : version;
    return *this;
}

TypeBuilder& TypeBuilder::BinaryCompatibilityVersion(std::uint32_t version) {
    m_Info.versions.binaryCompatibilityVersion = version == 0 ? 1u : version;
    return *this;
}

TypeBuilder& TypeBuilder::Versions(TypeVersionInfo versions) {
    m_Info.versions = versions;
    if (m_Info.versions.schemaVersion == 0) {
        m_Info.versions.schemaVersion = 1;
    }
    m_Info.schemaVersion = m_Info.versions.schemaVersion;
    return *this;
}

TypeBuilder& TypeBuilder::Flags(TypeFlags flags) {
    m_Info.flags = flags;
    return *this;
}

TypeBuilder& TypeBuilder::AddFlags(TypeFlags flags) {
    m_Info.flags = m_Info.flags | flags;
    return *this;
}

TypeBuilder& TypeBuilder::AliasOf(TypeId other) {
    m_Info.kind = TypeKind::Alias;
    m_Info.aliasOf = other;
    return *this;
}

TypeBuilder& TypeBuilder::Base(TypeId baseTypeId) {
    if (baseTypeId != kInvalidTypeId) {
        m_Info.baseTypes.push_back(baseTypeId);
    }
    return *this;
}

TypeBuilder& TypeBuilder::Interface(TypeId interfaceTypeId) {
    if (interfaceTypeId != kInvalidTypeId) {
        m_Info.interfaces.push_back(interfaceTypeId);
    }
    return *this;
}

TypeBuilder& TypeBuilder::ElementType(TypeId elementTypeId) {
    m_Info.elementTypeId = elementTypeId;
    return *this;
}

TypeBuilder& TypeBuilder::KeyValueTypes(TypeId keyTypeId, TypeId valueTypeId) {
    m_Info.keyTypeId = keyTypeId;
    m_Info.valueTypeId = valueTypeId;
    return *this;
}

TypeBuilder& TypeBuilder::Ops(TypeOps ops) {
    m_Info.ops = ops;
    return *this;
}

TypeBuilder& TypeBuilder::Attribute(AttributeInfo attr) {
    m_Info.attributes.Add(std::move(attr));
    return *this;
}

TypeBuilder& TypeBuilder::Property(PropertyInfo property) {
    m_Info.properties.push_back(std::move(property));
    return *this;
}

TypeBuilder& TypeBuilder::Property(
    std::string_view name,
    TypeId propertyTypeId,
    std::uint32_t offset,
    std::uint32_t size,
    PropertyFlags flags,
    PrimitiveKind primitive)
{
    m_Info.properties.push_back(
        MakeOffsetProperty(name, propertyTypeId, offset, size, 1, flags, primitive));
    return *this;
}

TypeBuilder& TypeBuilder::Function(FunctionInfo function) {
    m_Info.functions.push_back(std::move(function));
    return *this;
}

TypeBuilder& TypeBuilder::EnumValue(std::string_view name, std::int64_t value) {
    EnumValueInfo entry;
    entry.name = std::string(name);
    entry.value = value;
    m_Info.enumInfo.values.push_back(std::move(entry));
    m_Info.kind = TypeKind::Enum;
    return *this;
}

TypeBuilder& TypeBuilder::EnumFlags(bool isFlags) {
    m_Info.enumInfo.isFlags = isFlags;
    m_Info.kind = TypeKind::Enum;
    return *this;
}

TypeInfo TypeBuilder::Build() const {
    return m_Info;
}

bool TypeBuilder::Register(ITypeRegistry& registry) const {
    return registry.RegisterType(m_Info);
}

bool TypeBuilder::Register(ITypeRegistry& registry, RegisterMode mode) const {
    return registry.RegisterType(m_Info, mode);
}

} // namespace we::runtime::reflection
