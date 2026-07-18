#pragma once

#include "AssetImporter/Types.h"
#include "AssetProcessors/Export.h"
#include "AssetProcessors/ProcessStage.h"
#include "AssetProcessors/ProcessTypes.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::assetprocessors {

/// Plugin-replaceable processing stage. Does NOT import source files.
class ASSETPROCESSORS_API IAssetProcessor {
public:
    virtual ~IAssetProcessor() = default;

    [[nodiscard]] virtual std::string_view GetProcessorId() const = 0;
    [[nodiscard]] virtual std::string_view GetDisplayName() const = 0;
    [[nodiscard]] virtual std::string_view GetVersion() const { return "1.0.0"; }
    [[nodiscard]] virtual ProcessStage GetStage() const = 0;
    [[nodiscard]] virtual int GetPriority() const { return 100; }

    [[nodiscard]] virtual bool CanProcess(we::runtime::assetimporter::AssetKind kind) const = 0;

    [[nodiscard]] virtual ProcessResult Process(
        ProcessContext& context,
        const ProcessProgressCallback& progress) const = 0;
};

} // namespace we::runtime::assetprocessors
