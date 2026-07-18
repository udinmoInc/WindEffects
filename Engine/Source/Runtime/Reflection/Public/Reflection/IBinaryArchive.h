#pragma once

#include "Reflection/Export.h"
#include "Reflection/PropertyInfo.h"
#include "Reflection/TypeId.h"
#include "Reflection/TypeInfo.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::reflection {

class ITypeRegistry;
class IPropertyAccessor;

/// Binary-only archive sink/source for reflection-driven persistence.
/// Intentionally has no text / JSON / XML methods — package and network formats
/// sit on top of these primitives.
class REFLECTION_API IBinaryArchive {
public:
    virtual ~IBinaryArchive() = default;

    [[nodiscard]] virtual bool IsLoading() const = 0;
    [[nodiscard]] virtual bool IsSaving() const = 0;

    virtual bool SerializeBytes(void* data, std::size_t size) = 0;
    virtual bool SerializeBool(bool& value) = 0;
    virtual bool SerializeInt8(std::int8_t& value) = 0;
    virtual bool SerializeUInt8(std::uint8_t& value) = 0;
    virtual bool SerializeInt16(std::int16_t& value) = 0;
    virtual bool SerializeUInt16(std::uint16_t& value) = 0;
    virtual bool SerializeInt32(std::int32_t& value) = 0;
    virtual bool SerializeUInt32(std::uint32_t& value) = 0;
    virtual bool SerializeInt64(std::int64_t& value) = 0;
    virtual bool SerializeUInt64(std::uint64_t& value) = 0;
    virtual bool SerializeFloat(float& value) = 0;
    virtual bool SerializeDouble(double& value) = 0;

    /// Length-prefixed UTF-8 string (uint32 length + bytes, no null terminator required).
    virtual bool SerializeString(std::string& value) = 0;

    /// TypeId as raw uint64 — consumers may remap across schema versions.
    virtual bool SerializeTypeId(TypeId& value) = 0;
};

/// Memory-backed binary archive for in-process / package blob work.
class REFLECTION_API MemoryBinaryArchive final : public IBinaryArchive {
public:
    enum class Mode : std::uint8_t { Saving, Loading };

    explicit MemoryBinaryArchive(Mode mode);
    explicit MemoryBinaryArchive(std::vector<std::uint8_t> existingBytes); // loading
    MemoryBinaryArchive(const std::uint8_t* bytes, std::size_t size);      // loading view copy

    [[nodiscard]] bool IsLoading() const override { return m_Mode == Mode::Loading; }
    [[nodiscard]] bool IsSaving() const override { return m_Mode == Mode::Saving; }

    bool SerializeBytes(void* data, std::size_t size) override;
    bool SerializeBool(bool& value) override;
    bool SerializeInt8(std::int8_t& value) override;
    bool SerializeUInt8(std::uint8_t& value) override;
    bool SerializeInt16(std::int16_t& value) override;
    bool SerializeUInt16(std::uint16_t& value) override;
    bool SerializeInt32(std::int32_t& value) override;
    bool SerializeUInt32(std::uint32_t& value) override;
    bool SerializeInt64(std::int64_t& value) override;
    bool SerializeUInt64(std::uint64_t& value) override;
    bool SerializeFloat(float& value) override;
    bool SerializeDouble(double& value) override;
    bool SerializeString(std::string& value) override;
    bool SerializeTypeId(TypeId& value) override;

    [[nodiscard]] const std::vector<std::uint8_t>& Bytes() const noexcept { return m_Bytes; }
    [[nodiscard]] std::vector<std::uint8_t> TakeBytes() { return std::move(m_Bytes); }
    [[nodiscard]] std::size_t Tell() const noexcept { return m_Offset; }
    [[nodiscard]] bool Good() const noexcept { return m_Good; }

private:
    template <typename T>
    bool SerializePod(T& value);

    Mode m_Mode = Mode::Saving;
    std::vector<std::uint8_t> m_Bytes;
    std::size_t m_Offset = 0;
    bool m_Good = true;
};

/// Options for reflection-driven binary object serialization.
struct REFLECTION_API BinarySerializeOptions {
    bool includeBases = true;
    bool writeTypeIdHeader = true;     // prefix with TypeId + schemaVersion
    bool skipTransient = true;
    bool requireSerializeFlag = true;  // only properties with Serialize flag
};

/// Serialize a reflected object instance to a binary archive using property metadata.
[[nodiscard]] REFLECTION_API bool SerializeObjectBinary(
    IBinaryArchive& archive,
    const ITypeRegistry& registry,
    TypeId typeId,
    void* instance,
    const BinarySerializeOptions& options = {});

/// Convenience: serialize to a contiguous byte buffer.
[[nodiscard]] REFLECTION_API std::vector<std::uint8_t> SerializeObjectToBytes(
    const ITypeRegistry& registry,
    TypeId typeId,
    const void* instance,
    const BinarySerializeOptions& options = {});

/// Convenience: deserialize from a contiguous byte buffer.
[[nodiscard]] REFLECTION_API bool DeserializeObjectFromBytes(
    const ITypeRegistry& registry,
    TypeId typeId,
    void* instance,
    const std::uint8_t* bytes,
    std::size_t byteCount,
    const BinarySerializeOptions& options = {});

} // namespace we::runtime::reflection
