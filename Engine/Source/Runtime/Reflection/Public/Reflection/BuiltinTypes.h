#pragma once

#include "Reflection/Export.h"
#include "Reflection/ITypeRegistry.h"
#include "Reflection/TypeId.h"

namespace we::runtime::reflection {

/// Well-known TypeIds for built-in primitives (deterministic hashes of qualified names).
namespace BuiltinTypeId {
    [[nodiscard]] REFLECTION_API TypeId Void();
    [[nodiscard]] REFLECTION_API TypeId Bool();
    [[nodiscard]] REFLECTION_API TypeId Int8();
    [[nodiscard]] REFLECTION_API TypeId UInt8();
    [[nodiscard]] REFLECTION_API TypeId Int16();
    [[nodiscard]] REFLECTION_API TypeId UInt16();
    [[nodiscard]] REFLECTION_API TypeId Int32();
    [[nodiscard]] REFLECTION_API TypeId UInt32();
    [[nodiscard]] REFLECTION_API TypeId Int64();
    [[nodiscard]] REFLECTION_API TypeId UInt64();
    [[nodiscard]] REFLECTION_API TypeId Float();
    [[nodiscard]] REFLECTION_API TypeId Double();
    [[nodiscard]] REFLECTION_API TypeId Char();
    [[nodiscard]] REFLECTION_API TypeId String();
    [[nodiscard]] REFLECTION_API TypeId Vec2();
    [[nodiscard]] REFLECTION_API TypeId Vec3();
    [[nodiscard]] REFLECTION_API TypeId Vec4();
    [[nodiscard]] REFLECTION_API TypeId Quat();
    [[nodiscard]] REFLECTION_API TypeId Mat4();
    [[nodiscard]] REFLECTION_API TypeId Guid();
}

/// Registers all built-in primitive / math types into the registry.
/// Idempotent: re-registration replaces existing entries with the same TypeId.
REFLECTION_API void RegisterBuiltinTypes(ITypeRegistry& registry);

} // namespace we::runtime::reflection
