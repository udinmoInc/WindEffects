#pragma once

#include "Reflection/AttributeInfo.h"
#include "Reflection/Export.h"
#include "Reflection/ITypeRegistry.h"
#include "Reflection/NameId.h"
#include "Reflection/PropertyInfo.h"
#include "Reflection/TypeId.h"
#include "Reflection/TypeInfo.h"
#include "Reflection/TypeKinds.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace we::runtime::reflection {

/// Fluent builder for constructing TypeInfo prior to registry insertion.
class REFLECTION_API TypeBuilder {
public:
    TypeBuilder() = default;
    explicit TypeBuilder(std::string_view qualifiedName);

    TypeBuilder& QualifiedName(std::string_view name);
    TypeBuilder& Name(std::string_view shortName);
    TypeBuilder& Kind(TypeKind kind);
    TypeBuilder& Primitive(PrimitiveKind kind);
    TypeBuilder& Size(std::uint32_t size);
    TypeBuilder& Alignment(std::uint32_t alignment);
    TypeBuilder& SchemaVersion(std::uint32_t version);
    TypeBuilder& TypeVersion(std::uint32_t version);
    TypeBuilder& MigrationVersion(std::uint32_t version);
    TypeBuilder& BinaryCompatibilityVersion(std::uint32_t version);
    TypeBuilder& Versions(TypeVersionInfo versions);
    TypeBuilder& Flags(TypeFlags flags);
    TypeBuilder& AddFlags(TypeFlags flags);
    TypeBuilder& AliasOf(TypeId other);
    TypeBuilder& Base(TypeId baseTypeId);
    TypeBuilder& Interface(TypeId interfaceTypeId);
    TypeBuilder& ElementType(TypeId elementTypeId);
    TypeBuilder& KeyValueTypes(TypeId keyTypeId, TypeId valueTypeId);
    TypeBuilder& Ops(TypeOps ops);
    TypeBuilder& Attribute(AttributeInfo attr);

    TypeBuilder& Property(PropertyInfo property);
    TypeBuilder& Property(
        std::string_view name,
        TypeId propertyTypeId,
        std::uint32_t offset,
        std::uint32_t size,
        PropertyFlags flags = PropertyFlags::Serialize | PropertyFlags::Editable,
        PrimitiveKind primitive = PrimitiveKind::None);

    TypeBuilder& Function(FunctionInfo function);

    TypeBuilder& EnumValue(std::string_view name, std::int64_t value);
    TypeBuilder& EnumFlags(bool isFlags);

    [[nodiscard]] TypeInfo Build() const;
    bool Register(ITypeRegistry& registry) const;
    bool Register(ITypeRegistry& registry, RegisterMode mode) const;

private:
    TypeInfo m_Info;
};

[[nodiscard]] inline PropertyInfo MakeOffsetProperty(
    std::string_view name,
    TypeId propertyTypeId,
    std::uint32_t offset,
    std::uint32_t size,
    std::uint16_t alignment,
    PropertyFlags flags = PropertyFlags::Serialize | PropertyFlags::Editable,
    PrimitiveKind primitive = PrimitiveKind::None)
{
    PropertyInfo property;
    property.name = std::string(name);
    property.typeId = propertyTypeId;
    property.offset = offset;
    property.size = size;
    property.alignment = alignment == 0 ? std::uint16_t{1} : alignment;
    property.flags = flags;
    property.primitive = primitive;
    return property;
}

template <typename T>
[[nodiscard]] TypeOps MakeTypeOpsFor() {
    TypeOps ops{};
    ops.placementConstruct = [](void* memory) { new (memory) T(); };
    ops.placementDestruct = [](void* memory) { static_cast<T*>(memory)->~T(); };
    ops.defaultConstruct = []() -> void* { return new T(); };
    ops.destruct = [](void* instance) { delete static_cast<T*>(instance); };
    if constexpr (std::is_copy_constructible_v<T>) {
        ops.copyConstruct = [](void* dst, const void* src) {
            new (dst) T(*static_cast<const T*>(src));
        };
    }
    if constexpr (std::is_move_constructible_v<T>) {
        ops.moveConstruct = [](void* dst, void* src) {
            new (dst) T(std::move(*static_cast<T*>(src)));
        };
    }
    if constexpr (std::is_copy_assignable_v<T>) {
        ops.copyAssign = [](void* dst, const void* src) {
            *static_cast<T*>(dst) = *static_cast<const T*>(src);
        };
    }
    if constexpr (std::is_move_assignable_v<T>) {
        ops.moveAssign = [](void* dst, void* src) {
            *static_cast<T*>(dst) = std::move(*static_cast<T*>(src));
        };
    }
    return ops;
}

template <typename T>
[[nodiscard]] constexpr PrimitiveKind PrimitiveKindOf() noexcept {
    if constexpr (std::is_same_v<T, bool>) return PrimitiveKind::Bool;
    else if constexpr (std::is_same_v<T, std::int8_t>) return PrimitiveKind::Int8;
    else if constexpr (std::is_same_v<T, std::uint8_t>) return PrimitiveKind::UInt8;
    else if constexpr (std::is_same_v<T, std::int16_t>) return PrimitiveKind::Int16;
    else if constexpr (std::is_same_v<T, std::uint16_t>) return PrimitiveKind::UInt16;
    else if constexpr (std::is_same_v<T, std::int32_t>) return PrimitiveKind::Int32;
    else if constexpr (std::is_same_v<T, std::uint32_t>) return PrimitiveKind::UInt32;
    else if constexpr (std::is_same_v<T, std::int64_t>) return PrimitiveKind::Int64;
    else if constexpr (std::is_same_v<T, std::uint64_t>) return PrimitiveKind::UInt64;
    else if constexpr (std::is_same_v<T, float>) return PrimitiveKind::Float;
    else if constexpr (std::is_same_v<T, double>) return PrimitiveKind::Double;
    else if constexpr (std::is_same_v<T, char>) return PrimitiveKind::Char;
    else if constexpr (std::is_same_v<T, std::string>) return PrimitiveKind::String;
    else return PrimitiveKind::None;
}

} // namespace we::runtime::reflection

#define WE_REFLECT_TYPE_ID(QualifiedNameLiteral) \
    (::we::runtime::reflection::MakeTypeId(QualifiedNameLiteral))

#define WE_REFLECT_PROPERTY(OwnerType, FieldName, PropertyTypeId) \
    (::we::runtime::reflection::MakeOffsetProperty( \
        #FieldName, \
        (PropertyTypeId), \
        static_cast<std::uint32_t>(offsetof(OwnerType, FieldName)), \
        static_cast<std::uint32_t>(sizeof(((OwnerType*)nullptr)->FieldName)), \
        static_cast<std::uint16_t>(alignof(decltype(((OwnerType*)nullptr)->FieldName)))))

#define WE_REFLECT_PROPERTY_FLAGS(OwnerType, FieldName, PropertyTypeId, Flags) \
    (::we::runtime::reflection::MakeOffsetProperty( \
        #FieldName, \
        (PropertyTypeId), \
        static_cast<std::uint32_t>(offsetof(OwnerType, FieldName)), \
        static_cast<std::uint32_t>(sizeof(((OwnerType*)nullptr)->FieldName)), \
        static_cast<std::uint16_t>(alignof(decltype(((OwnerType*)nullptr)->FieldName))), \
        (Flags)))

#define WE_REFLECT_ENUM_VALUE(Name) \
    { #Name, static_cast<std::int64_t>(Name), {} }

/// Static auto-registration — runs when ApplyStaticTypeInitializers / CreateTypeRegistry is invoked.
#define WE_REFLECT_AUTO_REGISTER(UniqueName, ...) \
    static void WE_Reflect_Init_##UniqueName(::we::runtime::reflection::ITypeRegistry& registry) { \
        __VA_ARGS__ \
    } \
    static const bool WE_Reflect_Auto_##UniqueName = []() { \
        ::we::runtime::reflection::RegisterStaticTypeInitializer(&WE_Reflect_Init_##UniqueName); \
        return true; \
    }()
