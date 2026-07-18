#pragma once

#include "Serialization/Export.h"
#include "Serialization/ObjectGraph.h"
#include "Serialization/Types.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::serialization {

enum class ValidationSeverity : std::uint8_t {
    Info = 0,
    Warning,
    Error
};

struct SERIALIZATION_API ValidationIssue {
    ValidationSeverity severity = ValidationSeverity::Info;
    std::string code;
    std::string message;
    ObjectId objectId = kInvalidObjectId;
};

struct SERIALIZATION_API SerializationMemoryStats {
    std::size_t documentBytes = 0;
    std::size_t payloadBytes = 0;
    std::size_t objectCount = 0;
    std::size_t referenceCount = 0;
    std::size_t stringTableBytes = 0;
    std::size_t estimatedHeapBytes = 0;
};

struct SERIALIZATION_API SerializationTimingStats {
    double lastSerializeMs = 0.0;
    double lastDeserializeMs = 0.0;
    double totalSerializeMs = 0.0;
    double totalDeserializeMs = 0.0;
    std::uint64_t serializeCount = 0;
    std::uint64_t deserializeCount = 0;
};

struct SERIALIZATION_API SerializationDiagnosticsSnapshot {
    SerializationMemoryStats memory{};
    SerializationTimingStats timing{};
    std::vector<ValidationIssue> issues;
    std::uint64_t lastChecksum = 0;
    bool checksumValid = true;
};

namespace ValidationCode {
inline constexpr const char* BadMagic = "bad_magic";
inline constexpr const char* BadFormatVersion = "bad_format_version";
inline constexpr const char* BadAbiVersion = "bad_abi_version";
inline constexpr const char* ChecksumMismatch = "checksum_mismatch";
inline constexpr const char* TruncatedDocument = "truncated_document";
inline constexpr const char* MissingType = "missing_type";
inline constexpr const char* MissingProperty = "missing_property";
inline constexpr const char* VersionMismatch = "version_mismatch";
inline constexpr const char* Corruption = "corruption";
inline constexpr const char* CircularReference = "circular_reference";
inline constexpr const char* UnresolvedReference = "unresolved_reference";
inline constexpr const char* MigrationFailed = "migration_failed";
} // namespace ValidationCode

/// Validate a raw document buffer (header, bounds, optional checksum).
SERIALIZATION_API void ValidateDocumentBytes(
    const std::uint8_t* bytes,
    std::size_t byteCount,
    std::vector<ValidationIssue>& outIssues,
    bool verifyChecksum = true);

[[nodiscard]] SERIALIZATION_API bool IsDocumentValid(
    const std::uint8_t* bytes,
    std::size_t byteCount,
    bool verifyChecksum = true);

[[nodiscard]] SERIALIZATION_API std::uint64_t ComputeChecksum(
    const std::uint8_t* data,
    std::size_t size);

[[nodiscard]] SERIALIZATION_API std::size_t CountValidationIssues(
    const std::vector<ValidationIssue>& issues,
    ValidationSeverity minimumSeverity);

} // namespace we::runtime::serialization
