#pragma once

#include "AssetProcessors/Export.h"
#include "AssetProcessors/IAssetProcessor.h"
#include "AssetProcessors/ProcessStage.h"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace we::runtime::assetprocessors {

class ASSETPROCESSORS_API ProcessorRegistry {
public:
    void Register(std::shared_ptr<IAssetProcessor> processor);
    void Unregister(std::string_view processorId);
    void Clear();

    [[nodiscard]] std::shared_ptr<IAssetProcessor> FindById(std::string_view processorId) const;
    [[nodiscard]] std::vector<std::shared_ptr<IAssetProcessor>> GetForStage(ProcessStage stage) const;
    [[nodiscard]] std::vector<std::shared_ptr<IAssetProcessor>> GetAll() const;
    [[nodiscard]] std::string ComputeFingerprint() const;

private:
    mutable std::mutex m_Mutex;
    std::unordered_map<std::string, std::shared_ptr<IAssetProcessor>> m_ById;
};

} // namespace we::runtime::assetprocessors
