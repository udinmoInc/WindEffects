#include "Serialization/Diagnostics.h"
#include "HashUtils.h"

#include <cstring>

namespace we::runtime::serialization {

std::uint64_t ComputeChecksum(const std::uint8_t* data, std::size_t size) {
    if (!data || size == 0) {
        return 1;
    }
    return detail::Fnv1a64Bytes(data, size);
}

std::size_t CountValidationIssues(
    const std::vector<ValidationIssue>& issues,
    ValidationSeverity minimumSeverity)
{
    std::size_t count = 0;
    for (const ValidationIssue& issue : issues) {
        if (static_cast<std::uint8_t>(issue.severity) >= static_cast<std::uint8_t>(minimumSeverity)) {
            ++count;
        }
    }
    return count;
}

void ValidateDocumentBytes(
    const std::uint8_t* bytes,
    std::size_t byteCount,
    std::vector<ValidationIssue>& outIssues,
    bool verifyChecksum)
{
    if (!bytes || byteCount < sizeof(DocumentHeader)) {
        outIssues.push_back({
            ValidationSeverity::Error,
            ValidationCode::TruncatedDocument,
            "Document smaller than header",
            kInvalidObjectId});
        return;
    }

    DocumentHeader header{};
    std::memcpy(&header, bytes, sizeof(header));

    if (std::memcmp(header.magic, kSerializationMagic, 4) != 0) {
        outIssues.push_back({
            ValidationSeverity::Error,
            ValidationCode::BadMagic,
            "Invalid document magic (expected WEBN)",
            kInvalidObjectId});
    }
    if (header.formatVersion == 0 || header.formatVersion > kSerializationFormatVersion + 8) {
        // Allow limited forward window; still report.
        outIssues.push_back({
            header.formatVersion > kSerializationFormatVersion ? ValidationSeverity::Warning
                                                               : ValidationSeverity::Error,
            ValidationCode::BadFormatVersion,
            "Unsupported or invalid format version",
            kInvalidObjectId});
    }
    if (header.formatVersion > kSerializationFormatVersion) {
        outIssues.push_back({
            ValidationSeverity::Warning,
            ValidationCode::BadFormatVersion,
            "Document format is newer than this runtime (forward compatibility path)",
            kInvalidObjectId});
    }
    if (header.abiVersion == 0) {
        outIssues.push_back({
            ValidationSeverity::Error,
            ValidationCode::BadAbiVersion,
            "ABI version is zero",
            kInvalidObjectId});
    }
    if (header.abiVersion > kSerializationAbiVersion) {
        outIssues.push_back({
            ValidationSeverity::Warning,
            ValidationCode::BadAbiVersion,
            "Document ABI is newer than this runtime",
            kInvalidObjectId});
    }

    if (byteCount < sizeof(DocumentHeader) + 8) {
        outIssues.push_back({
            ValidationSeverity::Error,
            ValidationCode::TruncatedDocument,
            "Document body truncated",
            kInvalidObjectId});
        return;
    }

    if (verifyChecksum && HasFlag(static_cast<DocumentFlags>(header.flags), DocumentFlags::HasChecksum)
        && header.contentChecksum != 0) {
        const std::uint64_t actual =
            ComputeChecksum(bytes + sizeof(DocumentHeader), byteCount - sizeof(DocumentHeader));
        if (actual != header.contentChecksum) {
            outIssues.push_back({
                ValidationSeverity::Error,
                ValidationCode::ChecksumMismatch,
                "Content checksum mismatch — possible corruption",
                kInvalidObjectId});
        }
    }
}

bool IsDocumentValid(const std::uint8_t* bytes, std::size_t byteCount, bool verifyChecksum) {
    std::vector<ValidationIssue> issues;
    ValidateDocumentBytes(bytes, byteCount, issues, verifyChecksum);
    return CountValidationIssues(issues, ValidationSeverity::Error) == 0;
}

} // namespace we::runtime::serialization
