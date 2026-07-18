#pragma once

#include "Reflection/AttributeInfo.h"
#include "Reflection/EnumInfo.h"
#include "Reflection/Export.h"
#include "Reflection/FunctionInfo.h"
#include "Reflection/NameId.h"
#include "Reflection/PropertyInfo.h"
#include "Reflection/TypeId.h"
#include "Reflection/TypeKinds.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::reflection {

/// Construction / destruction hooks for reflected types (DLL-safe C function pointers).
using DefaultConstructFn = void* (*)();
using DestructFn = void (*)(void* instance);
using PlacementConstructFn = void (*)(void* memory);
using PlacementDestructFn = void (*)(void* memory);
using CopyConstructFn = void (*)(void* dst, const void* src);
using MoveConstructFn = void (*)(void* dst, void* src);
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

/// Per-type versioning for binary packages, networking, and schema migration.
struct REFLECTION_API TypeVersionInfo {
    std::uint32_t typeVersion = 1;                  // logical type revision
    std::uint32_t schemaVersion = 1;                // reflected field layout revision
    std::uint32_t migrationVersion = 1;             // migration pipeline revision
    std::uint32_t binaryCompatibilityVersion = 1;   // wire/package compatibility epoch
};

/// Complete runtime type metadata — the single source of truth for a reflected type.
struct REFLECTION_API TypeInfo {
    TypeId typeId = kInvalidTypeId;
    std::string name;
    std::string qualifiedName;
    NameId nameId = kInvalidNameId;
    NameId qualifiedNameId = kInvalidNameId;
    TypeKind kind = TypeKind::Unknown;
    PrimitiveKind primitive = PrimitiveKind::None;
    TypeFlags flags = TypeFlags::None;

    std::uint32_t size = 0;
    std::uint32_t alignment = 1;

    /// @deprecated Prefer versions.schemaVersion — kept for binary/API compatibility.
    std::uint32_t schemaVersion = 1;
    TypeVersionInfo versions{};

    TypeId aliasOf = kInvalidTypeId;

    std::vector<TypeId> baseTypes;
    std::vector<TypeId> interfaces;

    std::vector<PropertyInfo> properties;
    std::vector<FunctionInfo> functions;
    EnumInfo enumInfo;
    TypeOps ops;
    AttributeBag attributes;

    TypeId elementTypeId = kInvalidTypeId;
    TypeId keyTypeId = kInvalidTypeId;
    TypeId valueTypeId = kInvalidTypeId;

    /// Registration order index (deterministic within a registry lifetime).
    std::uint32_t registrationIndex = 0;

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
    [[nodiscard]] const PropertyInfo* FindProperty(NameId propertyNameId) const noexcept;
    [[nodiscard]] const FunctionInfo* FindFunction(std::string_view functionName) const noexcept;
    [[nodiscard]] const FunctionInfo* FindFunction(NameId functionNameId) const noexcept;
    [[nodiscard]] bool IsA(TypeId candidateBaseOrSelf) const noexcept;
};

} // namespace we::runtime::reflection
