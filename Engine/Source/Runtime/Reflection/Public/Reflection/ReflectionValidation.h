#pragma once

#include "Reflection/Export.h"
#include "Reflection/IReflectionDiagnostics.h"
#include "Reflection/TypeId.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::reflection {

class ITypeRegistry;

/// Controls depth of registry integrity checks.
enum class ValidationLevel : std::uint8_t {
    Basic = 0,   // duplicates, missing names, version fields
    Full,        // + inheritance, offsets, metadata, plans
    Strict       // + overlap, ABI, unused as warnings
};

struct REFLECTION_API ValidationOptions {
    ValidationLevel level = ValidationLevel::Full;
    bool checkInheritance = true;
    bool checkPropertyOffsets = true;
    bool checkPropertyOverlap = true;
    bool checkMissingPropertyTypes = true;
    bool checkMetadata = true;
    bool checkSerializePlans = true;
    bool checkCaches = true;
    bool checkBinaryCompatibility = true;
    bool checkEnumIntegrity = true;
    bool checkAttributes = true;
    bool reportUnusedTypes = false;
};

struct REFLECTION_API BinaryCompatibilityReport {
    bool compatible = false;
    std::uint64_t currentFingerprint = 0;
    std::uint64_t expectedFingerprint = 0;
    std::uint32_t schemaVersion = 0;
    std::uint32_t expectedSchemaVersion = 0;
    std::vector<DiagnosticIssue> issues;
};

struct REFLECTION_API CacheVerificationReport {
    bool valid = true;
    std::size_t typesChecked = 0;
    std::size_t plansChecked = 0;
    std::size_t ancestorCachesChecked = 0;
    std::vector<DiagnosticIssue> issues;
};

/// Extended validation — appends issues; never throws.
REFLECTION_API void ValidateRegistryEx(
    const ITypeRegistry& registry,
    std::vector<DiagnosticIssue>& outIssues,
    const ValidationOptions& options = {});

/// Verify precomputed caches (ancestors, derived, serialize plans, name maps).
[[nodiscard]] REFLECTION_API CacheVerificationReport VerifyReflectionCaches(
    const ITypeRegistry& registry);

/// Compare live registry fingerprint / schema against an expected ABI epoch.
[[nodiscard]] REFLECTION_API BinaryCompatibilityReport ValidateBinaryCompatibility(
    const ITypeRegistry& registry,
    std::uint64_t expectedFingerprint,
    std::uint32_t expectedSchemaVersion = 0);

/// True when ValidateRegistryEx reports zero Error-severity issues at Full level.
[[nodiscard]] REFLECTION_API bool IsReflectionIntegrityOk(const ITypeRegistry& registry);

/// Count issues at or above a severity threshold.
[[nodiscard]] REFLECTION_API std::size_t CountDiagnostics(
    const std::vector<DiagnosticIssue>& issues,
    DiagnosticSeverity minimumSeverity);

} // namespace we::runtime::reflection
