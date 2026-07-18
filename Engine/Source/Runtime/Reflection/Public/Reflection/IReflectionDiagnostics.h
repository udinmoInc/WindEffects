#pragma once

#include "Reflection/Export.h"
#include "Reflection/TypeId.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::reflection {

enum class DiagnosticSeverity : std::uint8_t {
    Info = 0,
    Warning,
    Error
};

struct REFLECTION_API DiagnosticIssue {
    DiagnosticSeverity severity = DiagnosticSeverity::Info;
    TypeId typeId = kInvalidTypeId;
    std::string code;    // stable machine code, e.g. "duplicate_property"
    std::string message;
};

struct REFLECTION_API ReflectionMemoryStats {
    std::size_t typeCount = 0;
    std::size_t propertyCount = 0;
    std::size_t functionCount = 0;
    std::size_t nameTableEntries = 0;
    std::size_t nameTableBytes = 0;
    std::size_t serializePlanBytes = 0;
    std::size_t estimatedHeapBytes = 0;
};

struct REFLECTION_API ReflectionTimingStats {
    double lastRegistrationMs = 0.0;
    double totalRegistrationMs = 0.0;
    double lastSealMs = 0.0;
    std::uint64_t registrationCount = 0;
    std::uint64_t lookupCount = 0;
    std::uint64_t propertyLookupCount = 0;
    std::uint64_t functionLookupCount = 0;
};

struct REFLECTION_API ReflectionDiagnosticsSnapshot {
    ReflectionMemoryStats memory;
    ReflectionTimingStats timing;
    std::vector<DiagnosticIssue> issues;
    std::uint64_t fingerprint = 0;
    bool sealed = false;
};

class ITypeRegistry;

/// Gather diagnostics for a registry (validates duplicates, conflicts, invalid metadata).
[[nodiscard]] REFLECTION_API ReflectionDiagnosticsSnapshot CaptureDiagnostics(
    const ITypeRegistry& registry);

/// Validate registry contents and append issues (does not throw).
/// Performs Full-level checks (inheritance, offsets, metadata, plans).
REFLECTION_API void ValidateRegistry(
    const ITypeRegistry& registry,
    std::vector<DiagnosticIssue>& outIssues);

/// Stable machine codes emitted by ValidateRegistry / ValidateRegistryEx.
namespace DiagnosticCode {
inline constexpr const char* MissingType = "missing_type";
inline constexpr const char* DuplicateType = "duplicate_type";
inline constexpr const char* DuplicateQualifiedName = "duplicate_qualified_name";
inline constexpr const char* ZeroSizeType = "zero_size_type";
inline constexpr const char* InvalidVersion = "invalid_version";
inline constexpr const char* InvalidProperty = "invalid_property";
inline constexpr const char* DuplicateProperty = "duplicate_property";
inline constexpr const char* ZeroSizeProperty = "zero_size_property";
inline constexpr const char* MissingBase = "missing_base";
inline constexpr const char* UnusedType = "unused_type";
inline constexpr const char* InheritanceCycle = "inheritance_cycle";
inline constexpr const char* SelfBase = "self_base";
inline constexpr const char* InvalidPropertyOffset = "invalid_property_offset";
inline constexpr const char* MisalignedProperty = "misaligned_property";
inline constexpr const char* PropertyOverlap = "property_overlap";
inline constexpr const char* MissingPropertyType = "missing_property_type";
inline constexpr const char* InvalidEnum = "invalid_enum";
inline constexpr const char* DuplicateEnumValue = "duplicate_enum_value";
inline constexpr const char* DuplicateFunction = "duplicate_function";
inline constexpr const char* InvalidAttribute = "invalid_attribute";
inline constexpr const char* AliasCycle = "alias_cycle";
inline constexpr const char* InvalidSerializePlan = "invalid_serialize_plan";
inline constexpr const char* CacheMismatch = "cache_mismatch";
inline constexpr const char* BinaryIncompatible = "binary_incompatible";
inline constexpr const char* InvalidAlignment = "invalid_alignment";
inline constexpr const char* MissingInterface = "missing_interface";
} // namespace DiagnosticCode

} // namespace we::runtime::reflection
