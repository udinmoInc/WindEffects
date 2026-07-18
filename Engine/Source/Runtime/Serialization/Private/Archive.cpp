#include "Serialization/IArchive.h"

#include <cstring>

namespace we::runtime::serialization {

MemoryArchive::MemoryArchive(Mode mode)
    : m_Mode(mode)
    , m_Binary(
          mode == Mode::Saving ? reflection::MemoryBinaryArchive::Mode::Saving
                               : reflection::MemoryBinaryArchive::Mode::Loading)
{
}

MemoryArchive::MemoryArchive(std::vector<std::uint8_t> existingBytes)
    : m_Mode(Mode::Loading)
    , m_Binary(std::move(existingBytes))
{
}

MemoryArchive::MemoryArchive(const std::uint8_t* bytes, std::size_t size)
    : m_Mode(Mode::Loading)
    , m_Binary(bytes, size)
{
}

bool MemoryArchive::IsLoading() const { return m_Mode == Mode::Loading; }
bool MemoryArchive::IsSaving() const { return m_Mode == Mode::Saving; }
bool MemoryArchive::Good() const { return m_Binary.Good(); }

reflection::IBinaryArchive& MemoryArchive::Binary() { return m_Binary; }
const reflection::IBinaryArchive& MemoryArchive::Binary() const { return m_Binary; }

bool MemoryArchive::SerializeObjectId(ObjectId& value) {
    return m_Binary.SerializeUInt64(value);
}

bool MemoryArchive::SerializeGuid(SerializationGuid& value) {
    return m_Binary.SerializeBytes(value.bytes.data(), value.bytes.size());
}

bool MemoryArchive::SerializeReference(ObjectReference& value) {
    std::uint8_t kind = static_cast<std::uint8_t>(value.kind);
    if (!m_Binary.SerializeUInt8(kind)) {
        return false;
    }
    if (IsLoading()) {
        value.kind = static_cast<ReferenceKind>(kind);
    }
    if (!SerializeObjectId(value.objectId)) {
        return false;
    }
    if (!SerializeTypeId(value.typeId)) {
        return false;
    }
    if (!SerializeGuid(value.guid)) {
        return false;
    }
    return SerializeString(value.softPath);
}

bool MemoryArchive::SerializeString(std::string& value) {
    return m_Binary.SerializeString(value);
}

bool MemoryArchive::SerializeBytes(void* data, std::size_t size) {
    return m_Binary.SerializeBytes(data, size);
}

bool MemoryArchive::SerializeTypeId(reflection::TypeId& value) {
    return m_Binary.SerializeTypeId(value);
}

bool MemoryArchive::SerializeUInt32(std::uint32_t& value) {
    return m_Binary.SerializeUInt32(value);
}

bool MemoryArchive::SerializeUInt64(std::uint64_t& value) {
    return m_Binary.SerializeUInt64(value);
}

const std::vector<std::uint8_t>& MemoryArchive::Bytes() const noexcept {
    return m_Binary.Bytes();
}

std::vector<std::uint8_t> MemoryArchive::TakeBytes() {
    return m_Binary.TakeBytes();
}

std::size_t MemoryArchive::Tell() const noexcept {
    return m_Binary.Tell();
}

BinaryWriter::BinaryWriter() = default;

bool BinaryWriter::WriteBytes(const void* data, std::size_t size) {
    if (!m_Good || !data || size == 0) {
        if (size == 0) {
            return m_Good;
        }
        m_Good = false;
        return false;
    }
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    m_Bytes.insert(m_Bytes.end(), bytes, bytes + size);
    return true;
}

bool BinaryWriter::WriteUInt32(std::uint32_t value) {
    return WriteBytes(&value, sizeof(value));
}

bool BinaryWriter::WriteUInt64(std::uint64_t value) {
    return WriteBytes(&value, sizeof(value));
}

bool BinaryWriter::WriteTypeId(reflection::TypeId value) {
    return WriteUInt64(value);
}

bool BinaryWriter::WriteObjectId(ObjectId value) {
    return WriteUInt64(value);
}

bool BinaryWriter::WriteGuid(const SerializationGuid& value) {
    return WriteBytes(value.bytes.data(), value.bytes.size());
}

bool BinaryWriter::WriteString(std::string_view value) {
    if (value.size() > 0xFFFFFFFFull) {
        m_Good = false;
        return false;
    }
    const std::uint32_t length = static_cast<std::uint32_t>(value.size());
    if (!WriteUInt32(length)) {
        return false;
    }
    if (length == 0) {
        return true;
    }
    return WriteBytes(value.data(), length);
}

bool BinaryWriter::WriteHeader(const DocumentHeader& header) {
    return WriteBytes(&header, sizeof(header));
}

BinaryReader::BinaryReader(const std::uint8_t* bytes, std::size_t size)
    : m_Data(bytes)
    , m_Size(size)
{
    if (!bytes && size > 0) {
        m_Good = false;
    }
}

BinaryReader::BinaryReader(const std::vector<std::uint8_t>& bytes)
    : BinaryReader(bytes.data(), bytes.size())
{
}

std::size_t BinaryReader::Remaining() const noexcept {
    return m_Offset <= m_Size ? m_Size - m_Offset : 0;
}

bool BinaryReader::ReadBytes(void* data, std::size_t size) {
    if (!m_Good || !data || size == 0) {
        if (size == 0) {
            return m_Good;
        }
        m_Good = false;
        return false;
    }
    if (m_Offset + size > m_Size) {
        m_Good = false;
        return false;
    }
    std::memcpy(data, m_Data + m_Offset, size);
    m_Offset += size;
    return true;
}

bool BinaryReader::ReadUInt32(std::uint32_t& value) {
    return ReadBytes(&value, sizeof(value));
}

bool BinaryReader::ReadUInt64(std::uint64_t& value) {
    return ReadBytes(&value, sizeof(value));
}

bool BinaryReader::ReadTypeId(reflection::TypeId& value) {
    return ReadUInt64(value);
}

bool BinaryReader::ReadObjectId(ObjectId& value) {
    return ReadUInt64(value);
}

bool BinaryReader::ReadGuid(SerializationGuid& value) {
    return ReadBytes(value.bytes.data(), value.bytes.size());
}

bool BinaryReader::ReadString(std::string& value) {
    std::uint32_t length = 0;
    if (!ReadUInt32(length)) {
        return false;
    }
    value.resize(length);
    if (length == 0) {
        return true;
    }
    return ReadBytes(value.data(), length);
}

bool BinaryReader::ReadHeader(DocumentHeader& header) {
    return ReadBytes(&header, sizeof(header));
}

std::unique_ptr<IArchive> CreateMemoryArchive(MemoryArchive::Mode mode) {
    return std::make_unique<MemoryArchive>(mode);
}

std::unique_ptr<IArchive> CreateMemoryArchive(std::vector<std::uint8_t> bytes) {
    return std::make_unique<MemoryArchive>(std::move(bytes));
}

} // namespace we::runtime::serialization
