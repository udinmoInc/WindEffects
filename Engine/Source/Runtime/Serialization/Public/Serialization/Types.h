#pragma once

#include "Serialization/Export.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string_view>

namespace we::runtime::serialization {

/// Binary document format version (package layout).
inline constexpr std::uint32_t kSerializationFormatVersion = 1;

/// Public ABI epoch for Serialization Runtime consumers.
inline constexpr std::uint32_t kSerializationAbiVersion = 1;

/// Magic: WindEffects Binary Native — 'WEBN'
inline constexpr char kSerializationMagic[4] = {'W', 'E', 'B', 'N'};

/// Stable object identity within a serialization document / object graph.
using ObjectId = std::uint64_t;
inline constexpr ObjectId kInvalidObjectId = 0;

/// 128-bit identity blob (assets, persistent GUIDs). Layout-compatible with AssetGuid bytes.
struct SERIALIZATION_API SerializationGuid {
    std::array<std::uint8_t, 16> bytes{};

    [[nodiscard]] static SerializationGuid Nil() noexcept { return {}; }
    [[nodiscard]] static SerializationGuid FromBytes(const std::uint8_t* data, std::size_t size) noexcept;
    [[nodiscard]] bool IsNil() const noexcept;
    [[nodiscard]] bool operator==(const SerializationGuid& other) const noexcept {
        return bytes == other.bytes;
    }
    [[nodiscard]] bool operator!=(const SerializationGuid& other) const noexcept {
        return !(*this == other);
    }
};

struct SerializationGuidHash {
    [[nodiscard]] std::size_t operator()(const SerializationGuid& guid) const noexcept;
};

enum class ReferenceKind : std::uint8_t {
    None = 0,
    Strong,   // owning / required object reference
    Shared,   // shared ownership (ref-counted at graph level)
    Weak,     // non-owning; may be null after load
    Soft,     // lazy / path-like; resolved on demand
    Asset     // asset identity (SerializationGuid)
};

enum class DocumentFlags : std::uint32_t {
    None              = 0,
    Compressed        = 1u << 0,
    Encrypted         = 1u << 1,
    HasChecksum       = 1u << 2,
    HasObjectTable    = 1u << 3,
    HasStringTable    = 1u << 4,
    HasReferenceTable = 1u << 5,
    DeltaDocument     = 1u << 6,
    SnapshotDocument  = 1u << 7,
    PartialDocument   = 1u << 8,
};

[[nodiscard]] constexpr DocumentFlags operator|(DocumentFlags a, DocumentFlags b) noexcept {
    return static_cast<DocumentFlags>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
}

[[nodiscard]] constexpr DocumentFlags operator&(DocumentFlags a, DocumentFlags b) noexcept {
    return static_cast<DocumentFlags>(static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b));
}

[[nodiscard]] constexpr bool HasFlag(DocumentFlags flags, DocumentFlags test) noexcept {
    return (static_cast<std::uint32_t>(flags) & static_cast<std::uint32_t>(test)) != 0;
}

enum class SerializeMode : std::uint8_t {
    Full = 0,
    Partial,
    Delta,
    Snapshot,
    Incremental
};

struct SERIALIZATION_API SerializeOptions {
    SerializeMode mode = SerializeMode::Full;
    bool writeTypeHeaders = true;
    bool skipTransient = true;
    bool includeBases = true;
    bool resolveReferences = true;
    bool detectCycles = true;
    bool computeChecksum = true;
    bool deterministic = true; // stable key ordering
    bool allowMissingTypesOnLoad = false;
    bool migrateOnVersionMismatch = true;
};

inline SerializationGuid SerializationGuid::FromBytes(const std::uint8_t* data, std::size_t size) noexcept {
    SerializationGuid guid{};
    if (data && size >= 16) {
        std::memcpy(guid.bytes.data(), data, 16);
    }
    return guid;
}

inline bool SerializationGuid::IsNil() const noexcept {
    for (std::uint8_t b : bytes) {
        if (b != 0) {
            return false;
        }
    }
    return true;
}

inline std::size_t SerializationGuidHash::operator()(const SerializationGuid& guid) const noexcept {
    std::uint64_t h = 14695981039346656037ull;
    for (std::uint8_t b : guid.bytes) {
        h ^= b;
        h *= 1099511628211ull;
    }
    return static_cast<std::size_t>(h);
}

} // namespace we::runtime::serialization
