#pragma once

#include "Reflection/Export.h"

#include <cstdint>
#include <functional>
#include <string_view>

namespace we::runtime::reflection {

/// Stable, DLL-boundary-safe type identifier.
/// Derived from a deterministic hash of the fully-qualified type name.
/// Never use C++ RTTI / typeid across module boundaries.
using TypeId = std::uint64_t;

inline constexpr TypeId kInvalidTypeId = 0;

/// Schema / metadata format version for binary consumers.
inline constexpr std::uint32_t kReflectionSchemaVersion = 1;

/// Deterministic FNV-1a 64-bit hash used for TypeId generation.
[[nodiscard]] REFLECTION_API TypeId HashTypeName(std::string_view qualifiedName) noexcept;

/// Produce a TypeId from a fully-qualified name (e.g. "we::runtime::ecs::Transform").
[[nodiscard]] inline TypeId MakeTypeId(std::string_view qualifiedName) noexcept {
    return HashTypeName(qualifiedName);
}

struct TypeIdHash {
    [[nodiscard]] std::size_t operator()(TypeId id) const noexcept {
        return static_cast<std::size_t>(id ^ (id >> 32));
    }
};

} // namespace we::runtime::reflection
