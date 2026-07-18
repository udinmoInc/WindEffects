#pragma once

#include "Serialization/Export.h"
#include "Serialization/ObjectGraph.h"
#include "Serialization/Types.h"
#include "Reflection/IBinaryArchive.h"
#include "Reflection/TypeId.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::serialization {

/// Higher-level archive facade over Reflection's IBinaryArchive.
/// Adds ObjectId / Guid / reference primitives used by document and graph serializers.
class SERIALIZATION_API IArchive {
public:
    virtual ~IArchive() = default;

    [[nodiscard]] virtual bool IsLoading() const = 0;
    [[nodiscard]] virtual bool IsSaving() const = 0;
    [[nodiscard]] virtual bool Good() const = 0;

    [[nodiscard]] virtual reflection::IBinaryArchive& Binary() = 0;
    [[nodiscard]] virtual const reflection::IBinaryArchive& Binary() const = 0;

    virtual bool SerializeObjectId(ObjectId& value) = 0;
    virtual bool SerializeGuid(SerializationGuid& value) = 0;
    virtual bool SerializeReference(ObjectReference& value) = 0;
    virtual bool SerializeString(std::string& value) = 0;
    virtual bool SerializeBytes(void* data, std::size_t size) = 0;
    virtual bool SerializeTypeId(reflection::TypeId& value) = 0;
    virtual bool SerializeUInt32(std::uint32_t& value) = 0;
    virtual bool SerializeUInt64(std::uint64_t& value) = 0;
};

/// Memory-backed archive for in-process / package blob work.
class SERIALIZATION_API MemoryArchive final : public IArchive {
public:
    enum class Mode : std::uint8_t { Saving, Loading };

    explicit MemoryArchive(Mode mode);
    explicit MemoryArchive(std::vector<std::uint8_t> existingBytes);
    MemoryArchive(const std::uint8_t* bytes, std::size_t size);

    [[nodiscard]] bool IsLoading() const override;
    [[nodiscard]] bool IsSaving() const override;
    [[nodiscard]] bool Good() const override;

    [[nodiscard]] reflection::IBinaryArchive& Binary() override;
    [[nodiscard]] const reflection::IBinaryArchive& Binary() const override;

    bool SerializeObjectId(ObjectId& value) override;
    bool SerializeGuid(SerializationGuid& value) override;
    bool SerializeReference(ObjectReference& value) override;
    bool SerializeString(std::string& value) override;
    bool SerializeBytes(void* data, std::size_t size) override;
    bool SerializeTypeId(reflection::TypeId& value) override;
    bool SerializeUInt32(std::uint32_t& value) override;
    bool SerializeUInt64(std::uint64_t& value) override;

    [[nodiscard]] const std::vector<std::uint8_t>& Bytes() const noexcept;
    [[nodiscard]] std::vector<std::uint8_t> TakeBytes();
    [[nodiscard]] std::size_t Tell() const noexcept;

private:
    Mode m_Mode = Mode::Saving;
    reflection::MemoryBinaryArchive m_Binary;
};

/// Streaming writer that appends deterministic binary sections.
class SERIALIZATION_API BinaryWriter {
public:
    BinaryWriter();

    bool WriteBytes(const void* data, std::size_t size);
    bool WriteUInt32(std::uint32_t value);
    bool WriteUInt64(std::uint64_t value);
    bool WriteTypeId(reflection::TypeId value);
    bool WriteObjectId(ObjectId value);
    bool WriteGuid(const SerializationGuid& value);
    bool WriteString(std::string_view value);
    bool WriteHeader(const DocumentHeader& header);

    [[nodiscard]] const std::vector<std::uint8_t>& Bytes() const noexcept { return m_Bytes; }
    [[nodiscard]] std::vector<std::uint8_t> TakeBytes() { return std::move(m_Bytes); }
    [[nodiscard]] std::size_t Size() const noexcept { return m_Bytes.size(); }
    [[nodiscard]] bool Good() const noexcept { return m_Good; }

private:
    std::vector<std::uint8_t> m_Bytes;
    bool m_Good = true;
};

/// Streaming reader over a contiguous byte buffer.
class SERIALIZATION_API BinaryReader {
public:
    BinaryReader(const std::uint8_t* bytes, std::size_t size);
    explicit BinaryReader(const std::vector<std::uint8_t>& bytes);

    bool ReadBytes(void* data, std::size_t size);
    bool ReadUInt32(std::uint32_t& value);
    bool ReadUInt64(std::uint64_t& value);
    bool ReadTypeId(reflection::TypeId& value);
    bool ReadObjectId(ObjectId& value);
    bool ReadGuid(SerializationGuid& value);
    bool ReadString(std::string& value);
    bool ReadHeader(DocumentHeader& header);

    [[nodiscard]] std::size_t Tell() const noexcept { return m_Offset; }
    [[nodiscard]] std::size_t Remaining() const noexcept;
    [[nodiscard]] bool Good() const noexcept { return m_Good; }
    [[nodiscard]] const std::uint8_t* Data() const noexcept { return m_Data; }
    [[nodiscard]] std::size_t Size() const noexcept { return m_Size; }

private:
    const std::uint8_t* m_Data = nullptr;
    std::size_t m_Size = 0;
    std::size_t m_Offset = 0;
    bool m_Good = true;
};

[[nodiscard]] SERIALIZATION_API std::unique_ptr<IArchive> CreateMemoryArchive(MemoryArchive::Mode mode);
[[nodiscard]] SERIALIZATION_API std::unique_ptr<IArchive> CreateMemoryArchive(
    std::vector<std::uint8_t> bytes);

} // namespace we::runtime::serialization
