#pragma once

#include "AssetProcessors/Export.h"
#include "AssetProcessors/ProcessorRegistry.h"
#include "AssetProcessors/ProcessTypes.h"

#include <memory>

namespace we::runtime::assetprocessors {

struct ASSETPROCESSORS_API AssetProcessorServiceDependencies {
    std::string engineVersion = "0.1.0";
};

/// Runs registered processors in stage order after import.
class ASSETPROCESSORS_API IAssetProcessorService {
public:
    virtual ~IAssetProcessorService() = default;

    [[nodiscard]] virtual ProcessorRegistry& GetRegistry() = 0;
    [[nodiscard]] virtual const ProcessorRegistry& GetRegistry() const = 0;

    [[nodiscard]] virtual ProcessResult Process(ProcessContext context) = 0;
};

[[nodiscard]] ASSETPROCESSORS_API std::unique_ptr<IAssetProcessorService> CreateAssetProcessorService(
    AssetProcessorServiceDependencies deps = {});

} // namespace we::runtime::assetprocessors
