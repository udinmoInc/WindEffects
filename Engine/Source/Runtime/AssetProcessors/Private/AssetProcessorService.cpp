#include "AssetProcessors/IAssetProcessorService.h"

#include "BuiltinProcessors.h"

#include "Core/Logger.h"

namespace we::runtime::assetprocessors {
namespace {

class AssetProcessorService final : public IAssetProcessorService {
public:
    explicit AssetProcessorService(AssetProcessorServiceDependencies deps)
        : m_Deps(std::move(deps)) {
        RegisterBuiltinProcessors(m_Registry);
    }

    [[nodiscard]] ProcessorRegistry& GetRegistry() override { return m_Registry; }
    [[nodiscard]] const ProcessorRegistry& GetRegistry() const override { return m_Registry; }

    [[nodiscard]] ProcessResult Process(ProcessContext context) override {
        ProcessResult result{};
        result.asset = context.imported;
        result.processorFingerprint = m_Registry.ComputeFingerprint();
        result.success = true;

        if (context.imported.cookedPath.empty() || !std::filesystem::exists(context.imported.cookedPath)) {
            ProcessDiagnostic d{};
            d.severity = we::runtime::assetimporter::ImportSeverity::Fatal;
            d.code = "process.missing_cooked";
            d.message = "Imported cooked asset path is missing; processors require importer output.";
            result.diagnostics.push_back(d);
            result.success = false;
            return result;
        }

        if (!m_Deps.engineVersion.empty()) {
            context.engineVersion = m_Deps.engineVersion;
        }

        for (uint32_t i = 0; i < static_cast<uint32_t>(ProcessStage::Count); ++i) {
            const auto stage = static_cast<ProcessStage>(i);
            auto processors = m_Registry.GetForStage(stage);
            if (processors.empty()) {
                continue;
            }

            for (const auto& processor : processors) {
                if (!processor->CanProcess(context.imported.metadata.kind)) {
                    continue;
                }

                ProcessProgressCallback progress = [](float, std::string_view) {};

                auto stageResult = processor->Process(context, progress);
                for (auto& d : stageResult.diagnostics) {
                    if (d.processorId.empty()) {
                        d.processorId = std::string(processor->GetProcessorId());
                    }
                    result.diagnostics.push_back(std::move(d));
                }

                if (stageResult.HasErrors()) {
                    result.success = false;
                    HE_ERROR(std::string("[AssetProcessors] Stage failed: ")
                        + std::string(processor->GetProcessorId()));
                    return result;
                }

                if (!stageResult.skipped) {
                    context.imported = stageResult.asset;
                    result.asset = stageResult.asset;
                }
            }
        }

        return result;
    }

private:
    AssetProcessorServiceDependencies m_Deps{};
    ProcessorRegistry m_Registry{};
};

} // namespace

std::unique_ptr<IAssetProcessorService> CreateAssetProcessorService(
    AssetProcessorServiceDependencies deps) {
    return std::make_unique<AssetProcessorService>(std::move(deps));
}

} // namespace we::runtime::assetprocessors
