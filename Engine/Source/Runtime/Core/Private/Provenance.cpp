#include "Core/Provenance.h"
#include "Core/ProductMetadata.h"
#include "Core/Logger.h"

#include <sstream>
#include <iomanip>
#include <cstring>
#include <array>

namespace we::core {

namespace {
constexpr uint32_t PROVENANCE_MAGIC = 0x57454447; // "WEDG"
constexpr uint32_t PROVENANCE_VERSION = 1;

struct ProvenanceHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t dataSize;
    uint32_t checksum;
};

uint32_t ComputeChecksum(const void* data, size_t size) {
    uint32_t hash = 5381;
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < size; ++i) {
        hash = ((hash << 5) + hash) + bytes[i];
    }
    return hash;
}
}

std::string ProvenanceData::Serialize() const {
    std::ostringstream oss;
    oss << engineGuid << "|" << signature << "|" << engineVersion << "|"
        << buildNumber << "|" << timestamp;
    return oss.str();
}

ProvenanceData ProvenanceData::Deserialize(const std::string& data) {
    ProvenanceData result;
    std::istringstream iss(data);
    std::string token;

    if (std::getline(iss, token, '|')) result.engineGuid = token;
    if (std::getline(iss, token, '|')) result.signature = token;
    if (std::getline(iss, token, '|')) result.engineVersion = token;
    if (std::getline(iss, token, '|')) result.buildNumber = token;
    if (std::getline(iss, token, '|')) result.timestamp = token;

    return result;
}

std::string ProvenanceData::ComputeHash() const {
    const std::string serialized = Serialize();
    const uint32_t checksum = ComputeChecksum(serialized.data(), serialized.size());

    std::ostringstream oss;
    oss << std::hex << std::setw(8) << std::setfill('0') << checksum;
    return oss.str();
}

ProvenanceService& ProvenanceService::Get() {
    static ProvenanceService instance;
    return instance;
}

ProvenanceData ProvenanceService::GetCurrentProvenance() const {
    ProvenanceData data;

    if (ProductMetadataService::Get().IsInitialized()) {
        const auto& metadata = ProductMetadataService::Get();
        data.engineGuid = metadata.GetEngine().guid;
        data.signature = metadata.GetEngine().provenanceSignature;
        data.engineVersion = metadata.GetProduct().version;
        data.buildNumber = metadata.GetProduct().build;
        data.timestamp = metadata.GetBuild().timestamp;
    } else {
        data.engineGuid = "A7B3C8D2-1E4F-4A5B-8C9D-0E1F2A3B4C5D";
        data.signature = "WE-PD-2026";
        data.engineVersion = "1.0.0";
        data.buildNumber = "1001";
        data.timestamp = "";
    }

    return data;
}

bool ProvenanceService::ValidateProvenance(const ProvenanceData& data) const {
    const ProvenanceData current = GetCurrentProvenance();

    if (data.engineGuid != current.engineGuid) {
        return false;
    }

    if (!data.signature.empty() && data.signature != current.signature) {
        return false;
    }

    return true;
}

void ProvenanceService::EmbedProvenance(void* buffer, size_t bufferSize) const {
    if (!buffer || bufferSize < sizeof(ProvenanceHeader)) {
        return;
    }

    const ProvenanceData data = GetCurrentProvenance();
    const std::string serialized = data.Serialize();

    ProvenanceHeader header{};
    header.magic = PROVENANCE_MAGIC;
    header.version = PROVENANCE_VERSION;
    header.dataSize = static_cast<uint32_t>(serialized.size());
    header.checksum = ComputeChecksum(serialized.data(), serialized.size());

    uint8_t* bytes = static_cast<uint8_t*>(buffer);
    std::memcpy(bytes, &header, sizeof(ProvenanceHeader));

    if (bufferSize >= sizeof(ProvenanceHeader) + serialized.size()) {
        std::memcpy(bytes + sizeof(ProvenanceHeader), serialized.data(), serialized.size());
    }
}

bool ProvenanceService::ExtractProvenance(const void* buffer, size_t bufferSize, ProvenanceData& outData) const {
    if (!buffer || bufferSize < sizeof(ProvenanceHeader)) {
        return false;
    }

    const uint8_t* bytes = static_cast<const uint8_t*>(buffer);
    const ProvenanceHeader* header = reinterpret_cast<const ProvenanceHeader*>(bytes);

    if (header->magic != PROVENANCE_MAGIC || header->version != PROVENANCE_VERSION) {
        return false;
    }

    if (bufferSize < sizeof(ProvenanceHeader) + header->dataSize) {
        return false;
    }

    const std::string serialized(
        reinterpret_cast<const char*>(bytes + sizeof(ProvenanceHeader)),
        header->dataSize);

    const uint32_t computedChecksum = ComputeChecksum(serialized.data(), serialized.size());
    if (computedChecksum != header->checksum) {
        return false;
    }

    outData = ProvenanceData::Deserialize(serialized);
    return true;
}

} // namespace we::core
