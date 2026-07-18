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
REFLECTION_API void ValidateRegistry(
    const ITypeRegistry& registry,
    std::vector<DiagnosticIssue>& outIssues);

} // namespace we::runtime::reflection
