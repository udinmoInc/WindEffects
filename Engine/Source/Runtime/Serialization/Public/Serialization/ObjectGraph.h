#pragma once

#include "Serialization/Export.h"
#include "Serialization/Types.h"
#include "Reflection/TypeId.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::serialization {

/// Soft / hard object reference stored in documents and live graphs.
struct SERIALIZATION_API ObjectReference {
    ObjectId objectId = kInvalidObjectId;
    reflection::TypeId typeId = reflection::kInvalidTypeId;
    ReferenceKind kind = ReferenceKind::Strong;
    SerializationGuid guid{}; // used when kind == Asset or Soft with GUID
    std::string softPath;     // optional human path for Soft refs

    [[nodiscard]] bool IsValid() const noexcept {
        return objectId != kInvalidObjectId || !guid.IsNil() || !softPath.empty();
    }
};

/// Entry in a shared-reference table (objectId → retain count at serialize time).
struct SERIALIZATION_API SharedReferenceEntry {
    ObjectId objectId = kInvalidObjectId;
    std::uint32_t retainCount = 1;
};

/// One object slot in a serialization document.
struct SERIALIZATION_API ObjectTableEntry {
    ObjectId objectId = kInvalidObjectId;
    reflection::TypeId typeId = reflection::kInvalidTypeId;
    std::uint32_t schemaVersion = 1;
    std::uint32_t flags = 0;
    std::uint64_t payloadOffset = 0;
    std::uint64_t payloadSize = 0;
};

/// Fixed-size binary document header (deterministic layout).
#pragma pack(push, 1)
struct SERIALIZATION_API DocumentHeader {
    char magic[4] = {kSerializationMagic[0], kSerializationMagic[1], kSerializationMagic[2], kSerializationMagic[3]};
    std::uint32_t formatVersion = kSerializationFormatVersion;
    std::uint32_t abiVersion = kSerializationAbiVersion;
    std::uint32_t flags = 0; // DocumentFlags
    std::uint64_t contentChecksum = 0;
    std::uint64_t objectCount = 0;
    std::uint64_t stringTableOffset = 0;
    std::uint64_t objectTableOffset = 0;
    std::uint64_t referenceTableOffset = 0;
    std::uint64_t payloadOffset = 0;
    std::uint64_t payloadSize = 0;
    std::uint64_t baseFingerprint = 0; // for delta documents
    std::uint32_t reserved0 = 0;
    std::uint32_t reserved1 = 0;
};
#pragma pack(pop)

static_assert(sizeof(DocumentHeader) == 88, "DocumentHeader must stay ABI-stable");

} // namespace we::runtime::serialization
