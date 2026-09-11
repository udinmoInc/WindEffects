#pragma once

#include "Core/Export.h"

#include <string>
#include <cstdint>

namespace we::core {

struct ProvenanceData {
    std::string engineGuid;
    std::string signature;
    std::string engineVersion;
    std::string buildNumber;
    std::string timestamp;

    [[nodiscard]] std::string Serialize() const;
    [[nodiscard]] static ProvenanceData Deserialize(const std::string& data);
    [[nodiscard]] std::string ComputeHash() const;
};

class CORE_API ProvenanceService {
public:
    static ProvenanceService& Get();

    [[nodiscard]] ProvenanceData GetCurrentProvenance() const;
    [[nodiscard]] bool ValidateProvenance(const ProvenanceData& data) const;
    
    void EmbedProvenance(void* buffer, size_t bufferSize) const;
    [[nodiscard]] bool ExtractProvenance(const void* buffer, size_t bufferSize, ProvenanceData& outData) const;

private:
    ProvenanceService() = default;
    ~ProvenanceService() = default;
};

} // namespace we::core
