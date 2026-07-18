#pragma once

#include "Reflection/AttributeInfo.h"
#include "Reflection/EnumInfo.h"
#include "Reflection/Export.h"
#include "Reflection/FunctionInfo.h"
#include "Reflection/PropertyInfo.h"
#include "Reflection/TypeId.h"
#include "Reflection/TypeKinds.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::reflection {

/// Construction / destruction hooks for reflected types (DLL-safe C function pointers).
using DefaultConstructFn = void* (*)();                          // heap allocate + default construct
using DestructFn = void (*)(void* instance);                     // destroy + free if heap
using PlacementConstructFn = void (*)(void* memory);             // placement default construct
using PlacementDestructFn = void (*)(void* memory);              // placement destroy
using CopyConstructFn = void (*)(void* dst, const void* src);    // placement copy
using MoveConstructFn = void (*)(void* dst, void* src);          // placement move
using CopyAssignFn = void (*)(void* dst, const void* src);
using MoveAssignFn = void (*)(void* dst, void* src);

struct REFLECTION_API TypeOps {
    DefaultConstructFn defaultConstruct = nullptr;
    DestructFn destruct = nullptr;
    PlacementConstructFn placementConstruct = nullptr;
    PlacementDestructFn placementDestruct = nullptr;
    CopyConstructFn copyConstruct = nullptr;
    MoveConstructFn moveConstruct = nullptr;
    CopyAssignFn copyAssign = nullptr;
    MoveAssignFn moveAssign = nullptr;

    [[nodiscard]] bool CanDefaultConstruct() const noexcept {
        return defaultConstruct != nullptr || placementConstruct != nullptr;
    }
};

/// Complete runtime type metadata — the single source of truth for a reflected type.
struct REFLECTION_API TypeInfo {
    TypeId typeId = kInvalidTypeId;
    std::string name;                 // short name (e.g. "Transform")
    std::string qualifiedName;        // fully qualified (e.g. "we::runtime::ecs::Transform")
    TypeKind kind = TypeKind::Unknown;
    PrimitiveKind primitive = PrimitiveKind::None;
    TypeFlags flags = TypeFlags::None;

    std::uint32_t size = 0;
    std::uint32_t alignment = 1;
    std::uint32_t schemaVersion = 1;  // per-type binary schema version
    TypeId aliasOf = kInvalidTypeId;  // when kind == Alias

    /// Single inheritance chain (most-derived base first). Multiple bases allowed.
    std::vector<TypeId> baseTypes;
    /// Implemented interfaces (TypeKind::Interface).
    std::vector<TypeId> interfaces;

    std::vector<PropertyInfo> properties;
    std::vector<FunctionInfo> functions;
    EnumInfo enumInfo;                // valid when kind == Enum
    TypeOps ops;
    AttributeBag attributes;

    /// Optional element / key / value types for containers.
    TypeId elementTypeId = kInvalidTypeId;
    TypeId keyTypeId = kInvalidTypeId;
    TypeId valueTypeId = kInvalidTypeId;

    [[nodiscard]] bool IsValid() const noexcept { return typeId != kInvalidTypeId; }
    [[nodiscard]] bool IsPrimitive() const noexcept { return kind == TypeKind::Primitive; }
    [[nodiscard]] bool IsEnum() const noexcept { return kind == TypeKind::Enum; }
    [[nodiscard]] bool IsClassOrStruct() const noexcept {
        return kind == TypeKind::Class || kind == TypeKind::Struct;
    }
    [[nodiscard]] bool IsInstantiable() const noexcept {
        return HasFlag(flags, TypeFlags::Instantiable) && ops.CanDefaultConstruct();
    }

    [[nodiscard]] const PropertyInfo* FindProperty(std::string_view propertyName) const noexcept;
    [[nodiscard]] const FunctionInfo* FindFunction(std::string_view functionName) const noexcept;
    [[nodiscard]] bool IsA(TypeId candidateBaseOrSelf) const noexcept;
};

} // namespace we::runtime::reflection
