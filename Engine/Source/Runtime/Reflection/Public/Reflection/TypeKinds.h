#pragma once

#include <cstdint>

namespace we::runtime::reflection {

/// High-level classification of a reflected type.
enum class TypeKind : std::uint8_t {
    Unknown = 0,
    Primitive,
    Enum,
    Struct,
    Class,
    Interface,
    Array,
    Map,
    Optional,
    Alias,   // typedef / using alias to another TypeId
};

/// Built-in primitive kinds (stored when TypeKind == Primitive).
enum class PrimitiveKind : std::uint8_t {
    None = 0,
    Bool,
    Int8,
    UInt8,
    Int16,
    UInt16,
    Int32,
    UInt32,
    Int64,
    UInt64,
    Float,
    Double,
    Char,       // UTF-8 code unit (char)
    String,     // std::string (or engine string type)
    StringView, // non-owning view — not typically serialized as owned data
    Void,
    Vec2,
    Vec3,
    Vec4,
    Quat,
    Mat4,
    Guid,       // 128-bit identity blob
    Opaque,     // raw bytes with known size
};

/// Property / field usage flags (bitmask). Designed for editors, serialization, networking.
enum class PropertyFlags : std::uint32_t {
    None            = 0,
    Serialize       = 1u << 0,  // included in binary persistence
    Network         = 1u << 1,  // replicated / networked
    Editable        = 1u << 2,  // shown in property editors
    ReadOnly        = 1u << 3,  // editable UI but not writable at runtime via accessor
    Transient       = 1u << 4,  // never persisted
    Hidden          = 1u << 5,  // hide from default UI
    AssetReference  = 1u << 6,  // treats value as asset identity
    EntityReference = 1u << 7,  // treats value as entity identity
    Deprecated      = 1u << 8,
    Advanced        = 1u << 9,  // collapsed under "advanced" in editors
};

[[nodiscard]] constexpr PropertyFlags operator|(PropertyFlags a, PropertyFlags b) noexcept {
    return static_cast<PropertyFlags>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

[[nodiscard]] constexpr PropertyFlags operator&(PropertyFlags a, PropertyFlags b) noexcept {
    return static_cast<PropertyFlags>(static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b));
}

[[nodiscard]] constexpr bool HasFlag(PropertyFlags flags, PropertyFlags test) noexcept {
    return (static_cast<std::uint32_t>(flags) & static_cast<std::uint32_t>(test)) != 0;
}

/// Type-level flags.
enum class TypeFlags : std::uint32_t {
    None           = 0,
    Abstract       = 1u << 0,
    TrivialCopy    = 1u << 1,  // memcpy-safe value
    TrivialDestroy = 1u << 2,
    Pod            = 1u << 3,  // plain-old-data blob
    Versioned      = 1u << 4,  // participates in schema migration
    EditorOnly     = 1u << 5,
    Instantiable   = 1u << 6,  // has a registered factory
};

[[nodiscard]] constexpr TypeFlags operator|(TypeFlags a, TypeFlags b) noexcept {
    return static_cast<TypeFlags>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

[[nodiscard]] constexpr TypeFlags operator&(TypeFlags a, TypeFlags b) noexcept {
    return static_cast<TypeFlags>(static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b));
}

[[nodiscard]] constexpr bool HasFlag(TypeFlags flags, TypeFlags test) noexcept {
    return (static_cast<std::uint32_t>(flags) & static_cast<std::uint32_t>(test)) != 0;
}

/// Function / method flags.
enum class FunctionFlags : std::uint32_t {
    None       = 0,
    Const      = 1u << 0,
    Static     = 1u << 1,
    Virtual    = 1u << 2,
    Blueprint  = 1u << 3,  // exposed to visual scripting
    EditorOnly = 1u << 4,
    Deprecated = 1u << 5,
};

[[nodiscard]] constexpr FunctionFlags operator|(FunctionFlags a, FunctionFlags b) noexcept {
    return static_cast<FunctionFlags>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

[[nodiscard]] constexpr FunctionFlags operator&(FunctionFlags a, FunctionFlags b) noexcept {
    return static_cast<FunctionFlags>(static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b));
}

[[nodiscard]] constexpr bool HasFlag(FunctionFlags flags, FunctionFlags test) noexcept {
    return (static_cast<std::uint32_t>(flags) & static_cast<std::uint32_t>(test)) != 0;
}

} // namespace we::runtime::reflection
